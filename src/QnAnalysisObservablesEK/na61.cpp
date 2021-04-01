//
// Created by eugene on 24/11/2020.
//

#include <DataContainer.hpp>
#include <DataContainerHelper.hpp>
#include <StatCalculate.hpp>

#include <any>
#include <filesystem>
#include <tuple>
#include <utility>

#include <TClass.h>
#include <TDirectory.h>
#include <TFile.h>
#include <TKey.h>
#include <TSystem.h>

#include <QnAnalysisTools/QnAnalysisTools.hpp>

#include "Observables.hpp"

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <TCanvas.h>
#include <TLegend.h>

namespace Details {

std::vector<std::string> FindTDirectory(const TDirectory &dir, const std::string &cwd = "") {
  std::vector<std::string> result;

  for (auto o : *dir.GetListOfKeys()) {
    auto key_ptr = dynamic_cast<TKey *>(o);
    if (TClass::GetClass(key_ptr->GetClassName())->InheritsFrom(TDirectory::Class())) {
      auto nested_dir = dynamic_cast<TDirectory *>(key_ptr->ReadObj());
      auto nested_contents = FindTDirectory(*nested_dir, cwd + "/" + nested_dir->GetName());
      std::move(std::begin(nested_contents), std::end(nested_contents), std::back_inserter(result));
    } else {
      result.emplace_back(cwd + "/" + key_ptr->GetName());
    }
  }
  return result;
}

}// namespace Details

template<typename T>
void LoadROOTFile(const std::string &file_name, const std::string &manager_prefix = "") {
  TFile f(file_name.c_str(), "READ");

  for (const auto &path : Details::FindTDirectory(f)) {
    auto ptr = f.Get<T>(path.c_str());
    if (ptr) {
      auto manager_path = manager_prefix.empty() ? path : "/" + manager_prefix + path;
      std::cout << "Adding path '" << manager_path << "'" << std::endl;
      AddResource(manager_path, ptr);
    }
  }
}

inline
std::string
PlotTitle(const ResourceManager::Resource &r) {
  using Predicates::Resource::META;
  using ::Tools::Format;

  const std::map<std::string, std::string> map_particle{
      {"proton", "p"},
      {"protons", "p"},
      {"pion_neg", "#pi-"},
  };
  const std::map<std::string, std::string> map_component{
      {"x1x1", "X"},
      {"y1y1", "Y"},
      {"combined", "X+Y"},
  };

  auto src = META["v1.src"](r);
  auto particle = META["v1.particle"](r);
  auto component = META["v1.component"](r);
  if (src == "mc") {
    return (Format("v_{1,%2%}^{%1%} (#Psi_{RP})")
        % map_particle.at(particle)
        % map_component.at(component)).str();
  } else if (src == "reco") {
    auto reference = META["v1.ref"](r);
    auto part_1 = (Format("v_{1,%2%}^{%1%} (%3%)")
        % map_particle.at(particle)
        % map_component.at(component)
        % reference).str();
    std::string part_2 = META["v1.resolution.title"](r);
    auto part_3 = "setup:" + META["v1.set"](r);
    return part_1.append(" ").append(part_2).append(" ").append(part_3);
  }
  return "";
}

void SaveCanvas(TCanvas &c, const std::string &base_name) {
//  c.Print((base_name + ".png").c_str(), "png");
//  c.Print((base_name + ".pdf").c_str(), "pdf");
//  c.Print((base_name + ".C").c_str());
}

TGraph *CalcSignificance(const TGraphErrors *case_graph, const TGraphErrors *ref_graph) {
  assert(case_graph->GetN() == ref_graph->GetN());
  auto signi_graph = new TGraph(ref_graph->GetN());
  for (int i = 0; i < ref_graph->GetN(); ++i) {
    auto ref_value = ref_graph->GetPointY(i);
    auto ref_err = ref_graph->GetErrorY(i);
    auto case_value = case_graph->GetPointY(i);
    auto case_err = case_graph->GetErrorY(i);
    auto significance = (case_value - ref_value) / TMath::Sqrt(case_err * case_err + ref_err * ref_err);
    if (std::isnan(significance) || std::isinf(significance)) {
      significance = 0.;
    }
    signi_graph->SetPoint(i, ref_graph->GetPointX(i), significance);
  }
  return signi_graph;
}

int main() {

  using std::get;
  using std::string;
  using ::Tools::Define;
  using ::Tools::Define1;
  using ::Tools::Format;
  namespace Tools = Qn::Analysis::Tools;

  using Predicates::Resource::META;
  using Predicates::Resource::KEY;
  using Predicates::Resource::BASE_OF;

  using DTCalc = Qn::DataContainerStatCalculate;
  using DTColl = Qn::DataContainerStatCollect;
  using Meta = ResourceManager::MetaType;

  using Tmpltor = ::Predicates::MetaTemplateGenerator;



  TFile f("correlation.root", "READ");
  LoadROOTFile<DTColl>(f.GetName(), "raw");

  /* Convert everything to Qn::DataContainerStatCalculate */
  gResourceManager.ForEach([](VectorKey key, DTColl &collect) {
    /* replacing /raw with /calc */
    key[0] = "calc";
    AddResource(key, Qn::DataContainerStatCalculate(collect));
  });


  /* Remap axis */
  gResourceManager.ForEach([](const StringKey &, DTCalc &dt) {
    const std::map<std::string, std::string> axis_name_map{
        {"RecEventHeaderProc_Centrality_Epsd", "Centrality"},
        {"Centrality_Centrality_Epsd", "Centrality"},
        {"SimTracksProc_y_cm", "y_cm"},
        {"SimTracksProc_pT", "pT"},
        {"RecParticles_y_cm", "y_cm"},
        {"RecParticles_pT", "pT"},
    };

    for (auto &ax : dt.GetAxes()) {
      auto name = ax.Name();

      auto map_it = axis_name_map.find(name);
      if (map_it != axis_name_map.end()) {
        ax.SetName(map_it->second);
      } else {
        assert(false);
      }
    } // axis

  });


  /* label correlations */
  gResourceManager.ForEach([](const StringKey &key, ResourceManager::Resource &r) {
    using std::filesystem::path;
    using boost::algorithm::split;
    path key_path(key);

    auto obj_name = key_path.filename().string();

    std::vector<std::string> tokens;
    split(tokens, obj_name, boost::is_any_of("."));

    /* args are all tokens but last */
    for (size_t iarg = 0; iarg < tokens.size() - 1; ++iarg) {
      const std::regex arg_re("^(\\w+)_(PLAIN|RECENTERED|RESCALED)$");
      std::smatch match_results;
      std::regex_search(tokens[iarg], match_results, arg_re);
      auto arg_name_raw = match_results.str(0);
      auto arg_name = match_results.str(1);
      auto arg_cstep = match_results.str(2);
      r.meta.put("arg" + std::to_string(iarg) + ".raw-name", arg_name_raw);
      r.meta.put("arg" + std::to_string(iarg) + ".name", arg_name);
      r.meta.put("arg" + std::to_string(iarg) + ".c-step", arg_cstep);
    }
    r.meta.put("component", tokens.back());
  });

  {
    const std::string
        uQ_reco_expr("^/calc/uQ/(pion_neg|pion_pos|protons)_(pt|y)_set_(\\w+)_(PLAIN|RECENTERED|TWIST|RESCALED).*$");
    gResourceManager.ForEach([&uQ_reco_expr](const StringKey &key, ResourceManager::Resource &r) {
      auto particle = KEY.MatchGroup(1, uQ_reco_expr)(r);
      auto axis = KEY.MatchGroup(2, uQ_reco_expr)(r);
      auto set = KEY.MatchGroup(3, uQ_reco_expr)(r);
      auto correction_step = KEY.MatchGroup(4, uQ_reco_expr)(r);
      r.meta.put("type", "uQ");
      r.meta.put("u.particle", particle);
      r.meta.put("u.axis", axis);
      r.meta.put("u.set", set);
      r.meta.put("u.src", "reco");
      r.meta.put("u.correction-step", correction_step);
    }, KEY.Matches(uQ_reco_expr));
  }
  {
    const std::string u_mc_expr("^/calc/uQ/mc_(pion_neg|pion_pos|protons)_(pt|y)_PLAIN.*$");
    gResourceManager.ForEach([&u_mc_expr](const StringKey &key, ResourceManager::Resource &r) {
      auto particle = KEY.MatchGroup(1, u_mc_expr)(r);
      auto axis = KEY.MatchGroup(2, u_mc_expr)(r);
      r.meta.put("type", "uQ");
      r.meta.put("u.particle", particle);
      r.meta.put("u.axis", axis);
      r.meta.put("u.src", "mc");
    }, KEY.Matches(u_mc_expr));
  }


  /* To check compatibility with old stuff, multiply raw correlations by factor of 2 */
  gResourceManager.ForEach([](VectorKey key, const ResourceManager::Resource &r) {
    auto meta = r.meta;
    key[0] = "x2";
    meta.put("type", "x2");
    auto result = 2 * r.As<DTCalc>();
    AddResource(key, ResourceManager::Resource(result, meta));
  });

  {
    /* Projection _y correlations to pT axis  */
    gResourceManager.ForEach([](const StringKey &name, DTCalc &calc) {
                               calc = calc.Projection({"Centrality", "y_cm"});
                             },
                             META["u.axis"] == "y");
    /* Projection _pT correlations to 'y' axis  */
    gResourceManager.ForEach([](const StringKey &name, DTCalc &calc) {
                               calc = calc.Projection({"Centrality", "pT"});
                             },
                             META["u.axis"] == "pt");
  }
  {
//    /* Rebin y  */
//    gResourceManager.ForEach([](const StringKey &name, DTCalc &calc) {
//                               calc = calc.Rebin(Qn::AxisD("y_cm",
//                                                           {-0.6, -0.4, -0.2, 0.0, 0.2, 0.4, 0.6, 0.8, 1.0, 1.2, 1.4, 1.6, 2.0, 2.4}));
//                             },
//                             META["u.axis"] == "y");
//    /* Rebin pT  */
//    gResourceManager.ForEach([](const StringKey &name, DTCalc &calc) {
//                               calc = calc.Rebin(Qn::AxisD("pT",
//                                                           {0., 0.2, 0.4, 0.6, 0.8, 1.0, 1.2, 1.4, 1.8, 2.4, 3.0}));
//                             },
//                             META["u.axis"] == "pt");
//    /* Rebin centrality  */
    gResourceManager.ForEach([](const StringKey &name, DTCalc &calc) {
      calc = calc.Rebin(Qn::AxisD("Centrality",
                                  {0., 10., 25., 45., 80.}));
    });
  }
  {
    /***************** RESOLUTION 3-sub ******************/
    const auto build_3sub_resolution = [](
        const std::string &meta_key,
        const std::array<std::string, 3> &base_q_vectors,
        const std::map<std::string, std::string> &ref_alias_map = {},
        const std::string title = "") {

      const std::vector<std::tuple<string, string, string>> q_permutations = {
          {base_q_vectors[0], base_q_vectors[1], base_q_vectors[2]},
          {base_q_vectors[1], base_q_vectors[0], base_q_vectors[2]},
          {base_q_vectors[2], base_q_vectors[0], base_q_vectors[1]},
      };
      const std::vector<std::string> q_components = {"x1x1", "y1y1"};

      for (auto&&[q_component, ref_args] : Tools::Combination(q_components, q_permutations)) {
        std::string subA, subB, subC;
        std::tie(subA, subB, subC) = ref_args;

        auto ref_alias = [&ref_alias_map, subA]() {
          auto remap_ref_it = ref_alias_map.find(subA);
          if (remap_ref_it != ref_alias_map.end()) {
            return remap_ref_it->second;
          }
          return subA;
        }();
        auto component = q_component == "x1x1" ? "X" : "Y";

        auto arg1_name = (Format("/calc/QQ/%1%_RECENTERED.%2%_RECENTERED.%3%") % subA % subB % q_component).str();
        auto arg2_name = (Format("/calc/QQ/%1%_RECENTERED.%2%_RECENTERED.%3%") % subA % subC % q_component).str();
        auto arg3_name = (Format("/calc/QQ/%1%_RECENTERED.%2%_RECENTERED.%3%") % subB % subC % q_component).str();
        auto resolution = (Format("/resolution/%3%/RES_%1%_%2%") % ref_alias % q_component % meta_key).str();

        Meta meta;
        meta.put("resolution.ref", subA);
        meta.put("resolution.ref_alias", ref_alias);
        meta.put("resolution.component", component);
        meta.put("resolution.meta_key", meta_key);
        meta.put("resolution.title", (Format("R_{1,%1%} (%2%) - %3%")
            % component
            % ref_alias
            % (title.empty() ? meta_key : title)).str());
        Define(resolution, Methods::Resolution3S, {arg1_name, arg2_name, arg3_name}, meta);
      }
    };

    build_3sub_resolution("3sub_standard",
                          {"psd1", "psd2", "psd3"});
//    build_3sub_resolution("3sub_psd90",
//                          {"psd1_90", "psd2_90", "psd3_90"},
//                          {{"psd1_90", "psd1"}, {"psd2_90", "psd2"}, {"psd3_90", "psd3"}});

  }

  {
    /***************** RESOLUTION MC ******************/
    auto build_psd_mc_resolution = [](
        const std::string &meta_key,
        const std::vector<std::string> &base_q_vectors,
        const std::map<std::string, std::string> &ref_alias_map = {}
    ) {
      std::vector<std::string> components = {"x1x1", "y1y1"};
      for (auto&&[q_component, base_q_vector] : Tools::Combination(components, base_q_vectors)) {

        auto ref_alias = [&ref_alias_map, &base_q_vector]() {
          auto remap_ref_it = ref_alias_map.find(base_q_vector);
          if (remap_ref_it != ref_alias_map.end()) {
            return remap_ref_it->second;
          }
          return base_q_vector;
        }();

        auto resolution_component = q_component == "x1x1" ? "X" : "Y";

        Meta meta;
        meta.put("type", "resolution");
        meta.put("resolution.ref", base_q_vector);
        meta.put("resolution.ref_alias", ref_alias);
        meta.put("resolution.component", resolution_component);
        meta.put("resolution.method", "mc");
        meta.put("resolution.meta_key", meta_key);
        meta.put("resolution.title", (Format("R_{1,%1%} (%2%) from MC")
            % resolution_component
            % ref_alias).str());

        auto name = (Format("/resolution/%3%/RES_%1%_%2%") % ref_alias % resolution_component % meta_key).str();
        auto arg_name = (Format("/calc/QQ/%1%_RECENTERED.psi_rp_PLAIN.%2%") % base_q_vector % q_component).str();
        Define(name, [](const DTCalc &calc) { return 2 * calc; }, {arg_name}, meta);
      } // component, base_q_vector
    };

    build_psd_mc_resolution("psd_mc", {"psd1", "psd2", "psd3"});
//    build_psd_mc_resolution("psd90_mc", {"psd1_90", "psd2_90", "psd3_90"},
//                            {{"psd1_90", "psd1"}, {"psd2_90", "psd2"}, {"psd3_90", "psd3"}});

  }

  {
    /***************** RESOLUTION 4-sub ******************/


    auto build_4sub_resolution = [](
        const std::string &meta_key,
        const std::string &tpc_ref_base,
        const std::vector<std::string> &psd_subs_base) {
      const std::vector<std::string> components = {"x1x1", "y1y1"};
      const std::string tpc_ref_cstep = "RESCALED";
      const std::string psd_subs_cstep = "RECENTERED";
      for (const auto &component : components) {
        const auto subA = psd_subs_base[0];
        const auto subB = psd_subs_base[1];
        const auto subC = psd_subs_base[2];
        const auto resolution_component = component == "x1x1" ? "X" : "Y";
        auto tpc_ref_subA = gResourceManager.GetMatching(KEY == (Format("/calc/uQ/%1%_%2%.%3%_%4%.%5%") % tpc_ref_base
            % tpc_ref_cstep % subA % psd_subs_cstep % component).str()).at(0);
        auto tpc_ref_subB = gResourceManager.GetMatching(KEY == (Format("/calc/uQ/%1%_%2%.%3%_%4%.%5%") % tpc_ref_base
            % tpc_ref_cstep % subB % psd_subs_cstep % component).str()).at(0);
        auto tpc_ref_subC = gResourceManager.GetMatching(KEY == (Format("/calc/uQ/%1%_%2%.%3%_%4%.%5%") % tpc_ref_base
            % tpc_ref_cstep % subC % psd_subs_cstep % component).str()).at(0);
        auto subA_subC = gResourceManager.GetMatching(KEY == (Format("/calc/QQ/%1%_%2%.%3%_%4%.%5%") % subA
            % psd_subs_cstep % subC % psd_subs_cstep % component).str()).at(0);

        auto apply_abs = [](const std::string &key) {
          auto &calc = gResourceManager.Get(key, ResourceManager::ResTag<DTCalc>());
          calc = Qn::Sqrt(Qn::Pow(calc, 2.));
        };
        apply_abs(tpc_ref_subA);
        apply_abs(tpc_ref_subB);
        apply_abs(tpc_ref_subC);

        Tmpltor resolution_key_generator
            ("/resolution/{{resolution.meta_key}}/RES_{{resolution.ref}}_{{resolution.component}}");

        Meta meta_r_rtpc;
        meta_r_rtpc.put("type", "resolution_4sub_aux");
        meta_r_rtpc.put("resolution.meta_key", meta_key);
        meta_r_rtpc.put("resolution.ref", "TPC");
        meta_r_rtpc.put("resolution.component", resolution_component);
        auto r_tpc_key = Define(resolution_key_generator,
                                Methods::Resolution3S,
                                {tpc_ref_subA, tpc_ref_subC, subA_subC},
                                meta_r_rtpc)->name;

        auto make_meta = [meta_key, resolution_component](const std::string &ref) {
          Meta meta;
          meta.put("resolution.meta_key", meta_key);
          meta.put("resolution.ref", ref);
          meta.put("resolution.ref_alias", ref);
          meta.put("resolution.component", resolution_component);
          return meta;
        };

        Define(resolution_key_generator, Methods::Resolution4S, {subA_subC, r_tpc_key, tpc_ref_subC}, make_meta(subA));
        Define(resolution_key_generator, Methods::Resolution4S_1, {tpc_ref_subB, r_tpc_key}, make_meta(subB));
        Define(resolution_key_generator, Methods::Resolution4S, {subA_subC, r_tpc_key, tpc_ref_subA}, make_meta(subC));
      }// components
    };

    build_4sub_resolution("4sub_preliminary", "4sub_ref_preliminary", {"psd1", "psd2", "psd3"});
    build_4sub_resolution("4sub_opt1", "4sub_ref_opt1", {"psd1", "psd2", "psd3"});
    build_4sub_resolution("4sub_opt2", "4sub_ref_opt2", {"psd1", "psd2", "psd3"});

  }

  const Tmpltor v1_reco_key_generator(
      "/{{type}}/{{v1.src}}/{{v1.particle}}/AX_{{v1.axis}}/systematics/"
      "SET_{{v1.set}}/RES_{{v1.resolution.meta_key}}/REF_{{v1.ref}}/{{v1.component}}");
  const Tmpltor v1_reco_centrality_key_generator(
      "/{{type}}/{{v1.src}}/{{v1.particle}}/AX_{{v1.axis}}/centrality_{{centrality.key}}/systematics/"
      "SET_{{v1.set}}/RES_{{v1.resolution.meta_key}}/REF_{{v1.ref}}/{{v1.component}}");

  const ::Predicates::MetaFeatureSet v1_reco_full_feature_set{
      "v1.particle",
      "v1.axis",
      // systematics
      "v1.set",
      "v1.resolution.meta_key",
      "v1.ref",
      "v1.component"
  };

  {
    /***************** Directed flow ******************/
    // folder structure /v1/<particle>/<axis>

    const auto uQ_filter = (
        META["type"] == "uQ" &&
            META["u.src"] == "reco" &&
            META["u.correction-step"] == "RESCALED") &&
        META["arg1.c-step"] == "RECENTERED";
    const auto resolution_filter = (META["type"] == "resolution");

    /* Lookup uQ */
    gResourceManager.ForEach([v1_reco_key_generator](const StringKey &uq_key, const ResourceManager::Resource &uQ_res) {
      auto reference = META["arg1.name"](uQ_res);
      auto correlation_component = META["component"](uQ_res);
      if (!(correlation_component == "x1x1" || correlation_component == "y1y1")) {
        return;
      }
      auto resolution_component = correlation_component == "x1x1" ? "X" : "Y";

      const auto resolution_filter = (META["type"] == "resolution" &&
          META["resolution.component"] == resolution_component &&
          META["resolution.ref"] == reference);
      /* Lookup resolution */
      gResourceManager.ForEach([uq_key, correlation_component, v1_reco_key_generator](const StringKey &resolution_key,
                                                                                      const ResourceManager::Resource &res) {
        std::cout << resolution_key << std::endl;
        Meta meta;
        meta.put("v1.ref", META["resolution.ref"](res));
        meta.put("v1.component", correlation_component);
        meta.put("v1.src", "reco");
        Define(v1_reco_key_generator, Methods::v1, {uq_key, resolution_key}, meta);
      }, resolution_filter);
    }, uQ_filter);

  } // directed flow

  {
    /***************** Coeffiecient c1 ******************/
    gResourceManager.GroupBy(
        META["u.particle"] + "__" +
            META["u.axis"] + "__" +
            META["arg1.name"],
        [v1_reco_key_generator](const std::string &meta_key,
                                const std::vector<ResourceManager::ResourcePtr> &list_resources) {
          for (auto &uQ_resource : list_resources) {
            auto reference = META["arg1.name"](*uQ_resource);
            auto uQ_component = META["component"](*uQ_resource);
            auto resolution_component = (uQ_component == "x1y1" ? "Y" : "X");
            auto c1_component = (uQ_component == "x1y1" ? "X" : "Y");

            auto resolution_keys = gResourceManager
                .SelectUniq(KEY, META["type"] == "resolution" &&
                    META["resolution.method"] == "mc" &&
                    META["resolution.ref"] == reference &&
                    META["resolution.component"] == resolution_component);
            if (resolution_keys.size() != 1) {
              return;
            }

            Meta meta;
            meta.put("type", "c1");
            meta.put("v1.ref", reference);
            meta.put("v1.component", c1_component);
            meta.put("v1.src", "reco");
            Define(v1_reco_key_generator, Methods::v1, {KEY(*uQ_resource), resolution_keys[0]}, meta);
          }
        },
        META["type"] == "uQ" &&
            META["component"].Matches("(x1y1|y1x1)") &&
            META["arg0.c-step"] == "RESCALED"
    );

  }

  const Tmpltor v1_mc_key_generator{
      "/{{type}}/{{v1.src}}/{{v1.particle}}/AX_{{v1.axis}}/systematics/{{v1.component}}"};
  const Tmpltor v1_mc_centrality_key_generator{
      "/{{type}}/{{v1.src}}/{{v1.particle}}/AX_{{v1.axis}}/centrality_{{centrality.key}}/systematics/{{v1.component}}"};
  {
    /***************** Directed flow (MC) ******************/
    const std::string re_expr(R"(^/calc/uQ/(mc_\w+)_PLAIN\.psi_rp_PLAIN\.(x1x1|y1y1)$)");
    gResourceManager.ForEach([re_expr, v1_mc_key_generator](const StringKey &key, ResourceManager::Resource &r) {
      std::string component = KEY.MatchGroup(2, re_expr)(r);
      std::string u_vector = KEY.MatchGroup(1, re_expr)(r);

      auto result = 2. * r.As<DTCalc>();

      Meta meta;
      meta.put("type", "v1");
      meta.put("v1.ref", "psi_rp");
      meta.put("v1.component", component);
      meta.put("v1.particle", r.meta.get<std::string>("u.particle"));
      meta.put("v1.axis", r.meta.get<std::string>("u.axis"));
      meta.put("v1.resolution.meta_key", "NA");
      meta.put("v1.src", "mc");

      ResourceManager::Resource new_res(result, meta);

      auto new_key = v1_mc_key_generator(new_res);

      AddResource(new_key, new_res);
    }, KEY.Matches(re_expr));
  }
  const ::Predicates::MetaFeatureSet v1_mc_full_feature_set{
      "v1.particle",
      "v1.axis",
      // systematics
      "v1.component"
  };

  {
    /// Inverse backward
    const auto v1_backward_filter =
        META["type"] == "v1" &&
            META["v1.set"] == "backward";

    gResourceManager.ForEach([](const StringKey &key, ResourceManager::Resource &res) {
      auto calc_inv = (-1) * res.As<DTCalc>();
      res.obj = calc_inv;
    }, v1_backward_filter);

  }

  /******************** v1 vs Centrality *********************/
  {
    auto key_generator = v1_reco_centrality_key_generator;
    auto expand_centrality_projection = [&key_generator](const VectorKey &key, ResourceManager::Resource &res) {
      auto calc = res.As<DTCalc>();
      auto meta = res.meta;

      auto centrality_axis = res.As<DTCalc>().GetAxes()[0];
      for (size_t ic = 0; ic < centrality_axis.size(); ++ic) {
        auto c_lo = centrality_axis.GetLowerBinEdge(ic);
        auto c_hi = centrality_axis.GetUpperBinEdge(ic);
        auto centrality_range_str = (Format("centrality_%1%-%2%") % c_lo % c_hi).str();
        auto centrality_key = (Format("%1%-%2%") % c_lo % c_hi).str();

        auto selected = calc.Select(Qn::AxisD(centrality_axis.Name(), 1, c_lo, c_hi));

        meta.put("type", META["type"](res) + "_centrality");
        meta.put("centrality.no", std::to_string(ic));
        meta.put("centrality.lo", c_lo);
        meta.put("centrality.hi", c_hi);
        meta.put("centrality.key", centrality_key);

        auto new_resource = ResourceManager::Resource(selected, meta);
        auto new_key = key_generator(new_resource);

        AddResource(new_key, new_resource);
      } // centrality bin
    };
    gResourceManager.ForEach(expand_centrality_projection,
                             (META["type"] == "v1" || META["type"] == "c1") && META["v1.src"] == "reco");
    key_generator = v1_mc_centrality_key_generator;
    gResourceManager.ForEach(expand_centrality_projection,
                             META["type"] == "v1" && META["v1.src"] == "mc");
  }

  const auto v1_reco_centrality_feature_set = v1_reco_full_feature_set + "centrality.key";
  const auto v1_mc_centrality_feature_set = v1_mc_full_feature_set + "centrality.key";

  /* Combine x1x1 and y1y1 */
  {
    auto v1_key_generator = v1_reco_centrality_key_generator;
    auto
        combine_components_function = [&v1_key_generator](auto f, std::vector<ResourceManager::ResourcePtr> &objs) {
      assert(objs.size() == 2);
      auto x1x1 = objs[0]->As<DTCalc>();
      auto y1y1 = objs[1]->As<DTCalc>();
      auto combined = x1x1.Apply(y1y1, [](const Qn::StatCalculate &a, const Qn::StatCalculate &b) {
        return Qn::Merge(a, b);
      });
      combined.SetErrors(Qn::StatCalculate::ErrorType::BOOTSTRAP);
      auto combined_meta = objs[0]->meta;
      combined_meta.put("v1.component", "combined");
      ResourceManager::Resource new_resource(combined, combined_meta);
      auto new_name = v1_key_generator(new_resource);
      AddResource(new_name, std::move(new_resource));
    };

    gResourceManager.GroupBy(
        v1_reco_centrality_feature_set - "v1.component",
        combine_components_function,
        META["type"] == "v1_centrality" &&
            META["v1.src"] == "reco" &&
            META["v1.component"].Matches("^(x1x1|y1y1)$"));

    v1_key_generator = v1_mc_centrality_key_generator;
    gResourceManager.GroupBy(
        v1_mc_centrality_feature_set - "v1.component",
        combine_components_function,
        META["type"] == "v1_centrality" &&
            META["v1.src"] == "mc" &&
            META["v1.component"].Matches("^(x1x1|y1y1)$"));



    /* Combine all references */
    auto combine_reference_function =
        [v1_reco_centrality_key_generator](auto feature, std::vector<ResourceManager::ResourcePtr> &objs) {
          assert(!objs.empty());
          auto combined = objs[0]->As<DTCalc>(); // taking copy of first object
          for (int iobjs = 1; iobjs < objs.size(); ++iobjs) {
            combined =
                combined.Apply(objs[iobjs]->As<DTCalc>(), [](const Qn::StatCalculate &a, const Qn::StatCalculate &b) {
                  return Qn::Merge(a, b);
                });
          }
          combined.SetErrors(Qn::StatCalculate::ErrorType::BOOTSTRAP);
          auto combined_meta = objs[0]->meta;
          combined_meta.put("v1.ref", "combined");
          combined_meta.put("v1.resolution.ref", "combined");
          auto new_resource = ResourceManager::Resource(combined, combined_meta);
          auto new_name = v1_reco_centrality_key_generator(new_resource);
          AddResource(new_name, new_resource);
        };
    gResourceManager.GroupBy(
        v1_reco_centrality_feature_set - "v1.ref",
        combine_reference_function,
        META["type"] == "v1_centrality" &&
            META["v1.src"] == "reco");

  }

/****************** DRAWING *********************/

  // All to TGraph
  gResourceManager
      .ForEach([](const StringKey &name, DTCalc calc) {
                 auto cwd = gDirectory;
                 gDirectory = nullptr;
                 auto graph = Qn::ToTGraph(calc);
                 if (graph)
                   AddResource("/profiles" + name, graph);
                 gDirectory = cwd;
               },
               KEY.Matches("^/x2/QQ/.*$"));

  {
    const auto resolution_filter = META["type"] == "resolution";

    ::Predicates::MetaFeatureSet resolution_feature_list{
        "resolution.meta_key",
        "resolution.ref_alias",
        "resolution.component",
    };

    const std::string save_dir = "resolution";
    gSystem->mkdir(save_dir.c_str());

    gResourceManager.GroupBy(META["resolution.meta_key"], [&save_dir](
        auto feature,
        const std::vector<ResourceManager::ResourcePtr> &resolutions_list) {

      const std::map<std::string, int> colors = {
          {"psd1", kRed + 1},
          {"psd2", kGreen + 2},
          {"psd3", kBlue}
      };
      const std::map<std::string, int> markers = {
          {"X", kFullSquare},
          {"Y", kOpenSquare},
      };

      TMultiGraph mg;
      for (auto &r : resolutions_list) {
        auto resolution_calc = r->template As<DTCalc>();
        auto resolution_graph = Qn::ToTGraph(resolution_calc);
        resolution_graph->SetLineColor(colors.at(META["resolution.ref_alias"](*r)));
        resolution_graph->SetMarkerColor(colors.at(META["resolution.ref_alias"](*r)));
        resolution_graph->SetMarkerStyle(markers.at(META["resolution.component"](*r)));
        resolution_graph->SetTitle((Format("R_{1,%1%} (%2%)")
            % META["resolution.component"](r)
            % META["resolution.ref_alias"](r)).str().c_str());
        mg.Add(resolution_graph, "pl");

        {
          auto cwd = gDirectory;
          gDirectory = nullptr;
          /// place resolution to the resource manager
          auto res_profile_key = Tmpltor("/profiles/resolution/"
                                         "{{resolution.meta_key}}/"
                                         "RES_{{resolution.ref_alias}}_{{resolution.component}}")(r);
          auto meta = r->meta;
          meta.put("type", "profile_resolution");
          auto obj = (TGraphErrors *) resolution_graph->Clone("");
          AddResource(res_profile_key, ResourceManager::Resource(*obj, meta));
          gDirectory = cwd;
        }

      }

      TCanvas c;
      c.SetCanvasSize(1280, 1024);
      mg.Draw("A");
      mg.GetYaxis()->SetRangeUser(0., .4);
      mg.GetYaxis()->SetTitle("R_{1}");
      mg.GetXaxis()->SetTitle("Centrality (%)");
      auto legend = c.BuildLegend(0.15, 0.7, 0.55, 0.9);
      legend->SetHeader(feature.c_str());
      SaveCanvas(c, save_dir + "/" + feature);

      TFile f((save_dir + "/multigraphs.root").c_str(), "update");
      f.WriteTObject(&mg, ("mg_" + feature).c_str(), "Overwrite");
    }, resolution_filter);

    gResourceManager
        .GroupBy(
            resolution_feature_list - "resolution.meta_key",
            [&save_dir](auto feature, const std::vector<ResourceManager::ResourcePtr> &resolution_ptrs) {
              const std::map<std::string, int> colors = {
                  {"psd_mc", kBlack},
                  {"psd90_mc", kOrange + 1},
                  {"3sub_standard", kRed},
                  {"3sub_psd90", kGreen + 2},
                  {"4sub_preliminary", kBlue},
                  {"4sub_opt1", kMagenta},
                  {"4sub_opt2", kCyan + 1},
              };

              TMultiGraph mg;
              for (auto &r : resolution_ptrs) {
                auto resolution_calc = r->template As<DTCalc>();
                auto resolution_graph = Qn::ToTGraph(resolution_calc);
                resolution_graph->SetLineColor(colors.at(META["resolution.meta_key"](*r)));
                resolution_graph->SetMarkerColor(colors.at(META["resolution.meta_key"](*r)));
                resolution_graph->SetLineWidth(2.);
                resolution_graph->SetTitle(META["resolution.meta_key"](r).c_str());
                mg.Add(resolution_graph, "pZ");
              }

              TCanvas c;
              c.SetCanvasSize(1280, 1024);
              mg.Draw("A");
              mg.GetYaxis()->SetRangeUser(0., .3);
              mg.GetYaxis()->SetTitle("R_{1}");
              mg.GetXaxis()->SetTitle("Centrality (%)");
              auto legend = c.BuildLegend(0.15, 0.2, 0.65, 0.35);
              legend->SetHeader(feature.c_str());
              SaveCanvas(c, save_dir + "/" + "comp_" + feature);

              TFile f_multigraphs((save_dir + "/" + "multigraphs.root").c_str(), "update");
              f_multigraphs.WriteTObject(&mg, ("comp__" + feature).c_str(), "Overwrite");
            }, resolution_filter);

  }

  /* v1 profiles */
  // /profiles/v1/<particle>/<axis>/<centrality range>/
  gResourceManager
      .ForEach(
          [](VectorKey key, ResourceManager::Resource &resource) {
            auto cwd = gDirectory;
            gDirectory = nullptr;
            const std::map<std::string, std::string> remap_axis_name = {
                {"pT", "p_{T} (GeV/#it{c})"},
                {"y_cm", "#it{y}_{CM}"},
            };
            const std::map<std::string, int> colors_map = {
                {"psd1", kRed},
                {"psd2", kGreen + 2},
                {"psd3", kBlue},
                {"combined", kBlack},
                {"psi_rp", kBlack}
            };

            auto selected_graph = Qn::ToTGraph(resource.As<DTCalc>());
            if (!selected_graph) {
              return;
            }
            selected_graph->SetName(key.back().c_str());
            selected_graph->GetXaxis()->SetTitle(remap_axis_name.at(resource.As<DTCalc>().GetAxes()[0].Name()).c_str());
            VectorKey new_key(key.begin(), key.end());
            new_key.insert(new_key.begin(), "profiles");
            auto meta = resource.meta;
            if (META["type"](resource) == "v1_centrality") {
              meta.put("type", "profile_v1");
            } else if (META["type"](resource) == "ratio_v1") {
              meta.put("type", "profile_ratio");
            }
            AddResource(new_key, ResourceManager::Resource(*selected_graph, meta));
            gDirectory = cwd;
          },
          META["type"] == "v1_centrality" ||
              META["type"] == "c1_centrality");


  /*********** v1 systematics (component) ********************/
  {
    std::string base_dir = "v1_systematics/component";

    gResourceManager
        .GroupBy(
            v1_reco_centrality_feature_set - "v1.component",
            [&base_dir](auto feature, const std::vector<ResourceManager::ResourcePtr> &resources) {
              const std::map<std::string, int> colors = {
                  {"x1x1", kRed},
                  {"y1y1", kBlue + 1},
                  {"combined", kBlack},
              };
              const std::map<std::string, std::pair<double, double>> y_ranges{
                  {"protons__pt", {-0.1, 0.25}},
                  {"protons__y", {-0.05, 0.4}},
                  {"pion_neg__pt", {-0.15, 0.15}},
                  {"pion_neg__y", {-0.15, 0.15}},
                  {"pion_pos__pt", {-0.05, 0.1}},
                  {"pion_pos__y", {-0.1, 0.1}},
              };
              const Tmpltor save_dir_tmpl("{{v1.particle}}__{{v1.axis}}/"
                                          "{{centrality.key}}/{{v1.set}}");
              const Tmpltor file_name_tmpl(
                  "RES_{{v1.resolution.meta_key}}");

              TMultiGraph mg;
              for (auto &r : resources) {
                auto &dt_calc = r->template As<DTCalc>();
                auto graph = Qn::ToTGraph(dt_calc);
                graph->SetLineColor(colors.at(META["v1.component"](*r)));
                graph->SetMarkerColor(colors.at(META["v1.component"](*r)));
                graph->GetXaxis()->SetTitle(dt_calc.GetAxes()[0].Name().c_str());
                graph->SetTitle(META["v1.component"](r).c_str());

                mg.Add(graph, META["v1.component"](*r) == "combined" ? "lpZ" : "pZ");
              } // resources

              /*********** reference from MC **********/
              auto mc_ref_keys = gResourceManager.template GetMatching(
                  META["type"] == "v1_centrality" &&
                      META["centrality.no"] == META["centrality.no"](resources.front()) &&
                      META["v1.src"] == "mc" &&
                      META["v1.particle"] == META["v1.particle"](resources.front()) &&
                      META["v1.axis"] == META["v1.axis"](resources.front()) &&
                      META["v1.component"] == "combined");
              if (!mc_ref_keys.empty()) {
                auto mc_ref = gResourceManager.template Get(mc_ref_keys.front(),
                                                            ResourceManager::ResTag<ResourceManager::Resource>());
                auto mc_ref_graph = Qn::ToTGraph(mc_ref.template As<DTCalc>());
                mc_ref_graph->SetLineStyle(kDashed);
                mc_ref_graph->SetLineColor(kBlack);
                mc_ref_graph->SetTitle("MC");
                mc_ref_graph->SetMarkerStyle(kOpenSquare);
                mg.Add(mc_ref_graph, "lpZ");
              }

              auto save_dir = base_dir + "/" + save_dir_tmpl(resources.front());
              auto filename = file_name_tmpl(resources.front());

              gSystem->mkdir(save_dir.c_str(), true);
              TCanvas c;
              c.SetCanvasSize(1280, 1024);
              mg.Draw("A");
              auto ranges = y_ranges.at(
                  META["v1.particle"](*resources.front()) + "__" +
                      META["v1.axis"](*resources.front()));
              mg.GetYaxis()->SetRangeUser(ranges.first, ranges.second);
              mg.GetXaxis()->SetTitle(resources.front()->template As<DTCalc>().GetAxes()[0].Name().c_str());
              c.BuildLegend();
              SaveCanvas(c, save_dir + "/" + filename);

              TFile f_multigraphs((save_dir + "/multigraphs.root").c_str(), "update");
              f_multigraphs.WriteTObject(&mg, filename.c_str(), "Overwrite");


              /************* ratios & significance plot *************/
              auto ref_it = std::find_if(resources.begin(), resources.end(),
                                         META["v1.component"] == "combined");

              if (ref_it != resources.end()) {
                auto ref_v1 = (*ref_it)->template As<DTCalc>();

                TMultiGraph ratio_mg;
                for (const auto &r : resources) {
                  auto ratio = r->template As<DTCalc>() / ref_v1;
                  auto ratio_graph = Qn::ToTGraph(ratio);
                  ratio_graph->SetLineColor(colors.at(META["v1.component"](r)));
                  ratio_graph->SetMarkerColor(colors.at(META["v1.component"](r)));
                  ratio_graph->SetLineWidth(2.);
                  ratio_graph->SetTitle(META["v1.component"](r).c_str());
                  ratio_mg.Add(ratio_graph, META["v1.component"](*r) == "combined" ? "lpZ" : "pZ");
                }
                c.Clear();
                ratio_mg.Draw("A");
                ratio_mg.GetYaxis()->SetRangeUser(0., 2.);
                ratio_mg.GetXaxis()->SetTitle(resources.front()->template As<DTCalc>().GetAxes()[0].Name().c_str());
                c.BuildLegend();
                SaveCanvas(c, save_dir + "/" + "ratio_" + filename);
                f_multigraphs.WriteTObject(&ratio_mg, ("ratio_" + filename).c_str(), "Overwrite");

                TMultiGraph signi_mg;
                for (const auto &r: resources) {
                  /* calculate significance */
                  auto ref_graph = Qn::ToTGraph(ref_v1);
                  auto case_graph = Qn::ToTGraph(r->template As<DTCalc>());
                  auto signi_graph = CalcSignificance(case_graph, ref_graph);
                  signi_graph->SetLineColor(colors.at(META["v1.component"](r)));
                  signi_graph->SetMarkerColor(colors.at(META["v1.component"](r)));
                  signi_graph->SetFillColor(colors.at(META["v1.component"](r)));
                  signi_graph->SetLineWidth(2.);
                  signi_graph->SetTitle(META["v1.component"](r).c_str());
                  signi_mg.Add(signi_graph, META["v1.component"](*r) == "combined" ? "l" : "lp");
                }
                f_multigraphs.WriteTObject(&signi_mg, ("signi_" + filename).c_str(), "Overwrite");
              } // ref exists

            },
            META["type"] == "v1_centrality" &&
                META["centrality.no"] == "1" &&
                META["v1.ref"] == "combined");
  }

  {
    std::string base_dir = "v1_systematics/reference";

    gResourceManager
        .GroupBy(
            v1_reco_centrality_feature_set - "v1.ref",
            [&base_dir](auto feature, const std::vector<ResourceManager::ResourcePtr> &resources) {
              const std::map<std::string, int> colors = {
                  {"psd1", kRed},
                  {"psd2", kGreen + 2},
                  {"psd3", kBlue},
                  {"combined", kBlack},
              };
              const std::map<std::string, std::pair<double, double>> y_ranges{
                  {"protons__pt", {-0.1, 0.25}},
                  {"protons__y", {-0.05, 0.4}},
                  {"pion_neg__pt", {-0.15, 0.15}},
                  {"pion_neg__y", {-0.15, 0.15}},
                  {"pion_pos__pt", {-0.05, 0.1}},
                  {"pion_pos__y", {-0.1, 0.1}},
              };
              const Tmpltor save_dir_tmpl("{{v1.particle}}__{{v1.axis}}/"
                                          "{{centrality.key}}/{{v1.set}}");
              const Tmpltor file_name_tmpl(
                  "RES_{{v1.resolution.meta_key}}");

              TMultiGraph mg;

              for (auto &r : resources) {
                auto &dt_calc = r->template As<DTCalc>();
                auto graph = Qn::ToTGraph(dt_calc);
                graph->SetLineColor(colors.at(META["v1.ref"](r)));
                graph->SetMarkerColor(colors.at(META["v1.ref"](r)));
                graph->SetTitle(META["v1.ref"](r).c_str());
                graph->GetXaxis()->SetTitle(dt_calc.GetAxes()[0].Name().c_str());
                mg.Add(graph, META["v1.ref"](r) == "combined" ? "lpZ" : "pZ");
              }

              auto save_dir = base_dir + "/" + save_dir_tmpl(resources.front().operator*());
              auto filename = file_name_tmpl(resources.front().operator*());

              /*********** reference from MC **********/
              auto mc_ref_keys = gResourceManager.template GetMatching(
                  META["type"] == "v1_centrality" &&
                      META["centrality.no"] == META["centrality.no"](resources.front()) &&
                      META["v1.src"] == "mc" &&
                      META["v1.particle"] == META["v1.particle"](resources.front()) &&
                      META["v1.axis"] == META["v1.axis"](resources.front()) &&
                      META["v1.component"] == "combined");
              if (!mc_ref_keys.empty()) {
                auto mc_ref = gResourceManager.template Get(mc_ref_keys.front(),
                                                            ResourceManager::ResTag<ResourceManager::Resource>());
                auto mc_ref_graph = Qn::ToTGraph(mc_ref.template As<DTCalc>());
                mc_ref_graph->SetLineStyle(kDashed);
                mc_ref_graph->SetLineColor(kBlack);
                mc_ref_graph->SetTitle("MC");
                mc_ref_graph->SetMarkerStyle(kOpenSquare);
                mg.Add(mc_ref_graph, "lpZ");
              }

              gSystem->mkdir(save_dir.c_str(), true);
              TCanvas c;
              c.SetCanvasSize(1280, 1024);
              mg.Draw("A");
              auto ranges = y_ranges.at(
                  META["v1.particle"](*resources.front()) + "__" +
                      META["v1.axis"](*resources.front()));
              mg.GetYaxis()->SetRangeUser(ranges.first, ranges.second);
              auto legend = c.BuildLegend();
              SaveCanvas(c, save_dir + "/" + filename);
              TFile f_multigraphs((save_dir + "/multigraphs.root").c_str(), "update");
              f_multigraphs.WriteTObject(&mg, filename.c_str(), "Overwrite");

              /************* ratios & significance plot *************/
              auto ref_it = std::find_if(resources.begin(), resources.end(),
                                         META["v1.ref"] == "combined");
              if (ref_it != resources.end()) {
                auto ref_v1 = (*ref_it)->template As<DTCalc>();

                TMultiGraph ratio_mg;
                for (auto &r : resources) {
                  auto ratio = r->template As<DTCalc>() / ref_v1;
                  auto ratio_graph = Qn::ToTGraph(ratio);
                  ratio_graph->SetLineColor(colors.at(META["v1.ref"](*r)));
                  ratio_graph->SetMarkerColor(colors.at(META["v1.ref"](*r)));
                  ratio_graph->SetTitle(META["v1.ref"](*r).c_str());
                  ratio_graph->GetXaxis()->SetTitle(ratio.GetAxes()[0].Name().c_str());
                  ratio_graph->SetLineWidth(2.0);
                  ratio_mg.Add(ratio_graph, META["v1.ref"](*r) == "combined" ? "lpZ" : "pZ");
                }

                c.Clear();
                ratio_mg.Draw("A");
                ratio_mg.GetYaxis()->SetRangeUser(0.0, 2.0);
                auto legend = c.BuildLegend();
                Tmpltor header_tmpl{"RES: {{v1.resolution.meta_key}}"};
                legend->SetHeader(header_tmpl(resources.front()).c_str());
                SaveCanvas(c, save_dir + "/" + "ratio_" + filename);
                f_multigraphs.WriteTObject(&ratio_mg, ("ratio_" + filename).c_str(), "Overwrite");

                TMultiGraph signi_mg;
                for (auto &r : resources) {
                  auto ref_graph = Qn::ToTGraph(ref_v1);
                  auto case_graph = Qn::ToTGraph(r->template As<DTCalc>());
                  auto signi_graph = CalcSignificance(case_graph, ref_graph);
                  signi_graph->SetLineColor(colors.at(META["v1.ref"](r)));
                  signi_graph->SetMarkerColor(colors.at(META["v1.ref"](r)));
                  signi_graph->SetLineWidth(2.);
                  signi_graph->SetTitle(META["v1.ref"](r).c_str());
                  signi_mg.Add(signi_graph, META["v1.ref"](*r) == "combined" ? "l" : "lp");
                }
                f_multigraphs.WriteTObject(&signi_mg, ("signi_" + filename).c_str(), "Overwrite");
              } // reference exists
            },
            META["type"] == "v1_centrality" &&
                META["centrality.no"] == "1" &&
                META["v1.src"] == "reco" &&
                META["v1.component"] == "combined");
  }


  /* compare forward/backward */
  {
    const auto v1_forward_backward_filter =
        META["type"] == "v1_centrality" &&
            META["v1.src"] == "reco" &&
            META["v1.resolution.meta_key"] == "psd_mc" &&
            (META["v1.set"] == "forward" || META["v1.set"] == "backward") &&
            META["v1.ref"] == "combined" &&
            META["v1.component"] == "combined";

    const std::string save_dir = "v1_forward_backward";
    gSystem->mkdir(save_dir.c_str());

    gResourceManager.GroupBy(
        ::Predicates::MetaFeatureSet({"centrality.key"}),
        [&save_dir](auto feature, const std::vector<ResourceManager::ResourcePtr> &v1_list) {
          assert(v1_list.size() == 2);
          TMultiGraph mg;
          const std::map<std::string, int> colors = {
              {"backward", kRed},
              {"forward", kBlack},
          };
          const std::map<std::string, int> markers = {
              {"backward", kOpenCircle},
              {"forward", kFullCircle},
          };
          for (const auto &v1_ptr : v1_list) {
            auto v1_set = META["v1.set"](*v1_ptr);
            auto graph = Qn::ToTGraph(v1_ptr->As<DTCalc>());
            graph->SetMarkerColor(colors.at(v1_set));
            graph->SetLineColor(colors.at(v1_set));
            graph->SetMarkerStyle(markers.at(v1_set));
            graph->SetTitle(v1_set.c_str());
            mg.Add(graph);
          }

          TCanvas c;
          c.SetCanvasSize(1280, 1024);
          mg.Draw("A lp");
          mg.GetYaxis()->SetRangeUser(-0.05, 0.15);
          mg.GetYaxis()->SetTitle("v_{1}");
          mg.GetXaxis()->SetTitle("p_{T} GeV/c");
          auto legend = c.BuildLegend(0.15, 0.75, 0.45, 0.9);
          legend->SetHeader(("Centrality: " + feature + "%").c_str());
          SaveCanvas(c, save_dir + "/" + feature);
        }, v1_forward_backward_filter);

  }

  /* Compare MC vs Data */
  {
    auto meta_key_gen = META["v1.particle"] + "__" + META["v1.axis"];
    auto centrality_predicate = META["centrality.key"] == "10-25";

    gResourceManager
        .GroupBy(
            meta_key_gen,
            [=](
                const std::string &meta_key,
                const std::vector<ResourceManager::ResourcePtr> &mc_correlations) {
              auto c_overview = new TCanvas;
              c_overview->SetCanvasSize(1280, 1024);

              std::map<std::string, TCanvas *> c_ratio_map{
                  {"x1x1", new TCanvas()},
                  {"y1y1", new TCanvas()},
                  {"combined", new TCanvas()},
              };
              std::map<std::string, std::pair<double, double>> ranges_map{
                  {"protons__pt", {-0.1, 0.25}},
                  {"protons__y", {-0.05, 0.4}},
                  {"pion_neg__pt", {-0.15, 0.15}},
                  {"pion_neg__y", {-0.15, 0.15}},
              };
              for (auto &&[component, canv_ptr] : c_ratio_map) {
                canv_ptr->SetCanvasSize(1280, 1024);
              }

              bool overview_first = true;
              for (auto &mc_ref : mc_correlations) {
                auto mc_ref_calc = mc_ref->As<DTCalc>();

                /* plot to overview */
                auto mc_ref_graph = Qn::ToTGraph(mc_ref_calc);

                const auto component = META["v1.component"](*mc_ref);

                auto line_style = kSolid;
                auto draw_opts = overview_first ? "Al" : "l";

                mc_ref_graph->GetXaxis()->SetTitle(mc_ref_calc.GetAxes()[0].Name().c_str());
                mc_ref_graph->GetYaxis()->SetTitle("v_{1}");
                mc_ref_graph->GetYaxis()->SetRangeUser(ranges_map.at(meta_key).first, ranges_map.at(meta_key).second);
                mc_ref_graph->SetLineStyle(line_style);
                mc_ref_graph->SetTitle(PlotTitle(*mc_ref).c_str());
                mc_ref_graph->SetLineWidth(2.);

                c_overview->cd();
                mc_ref_graph->DrawClone(draw_opts);
                overview_first = false;

                auto ratio_self_calc = mc_ref_calc / mc_ref_calc;
                auto ratio_self_graph = Qn::ToTGraph(ratio_self_calc);
                ratio_self_graph->SetTitle(PlotTitle(*mc_ref).c_str());
                ratio_self_graph->SetLineStyle(kSolid);
                ratio_self_graph->SetLineWidth(2.);
                ratio_self_graph->SetLineColor(kBlack);
                for (auto&&[component, canvas_ptr] : c_ratio_map) {
                  canvas_ptr->cd();
                  ratio_self_graph->Draw("Al");
                  ratio_self_graph->GetXaxis()->SetTitle(mc_ref_calc.GetAxes()[0].Name().c_str());
                  ratio_self_graph->GetYaxis()->SetRangeUser(0.0, 2.0);
                }

                gResourceManager.ForEach(
                    [=](const StringKey &key, ResourceManager::Resource &reco) {
                      const std::map<std::string, int> ref_rgb_colors{
                          {"psd1", kRed + 1},
                          {"psd1_90", kRed + 1},
                          {"psd2", kGreen + 2},
                          {"psd2_90", kGreen + 2},
                          {"psd3", kBlue},
                          {"psd3_90", kBlue},
                          {"combined", kBlack}
                      };

                      const auto reco_ref = META["v1.ref"](reco);
                      const auto reco_component = META["v1.component"](reco);
                      const auto setup = META["v1.set"](reco);
                      const auto resolution_key = META["v1.resolution.meta_key"](reco);

                      const auto draw_color = ref_rgb_colors.at(reco_ref);
                      const auto draw_marker_style = [reco_component, reco_ref]() {
                        if (reco_component == "x1x1")
                          return kFullCircle;
                        else if (reco_component == "y1y1")
                          return kOpenCircle;
                        else if (reco_component == "combined")
                          return kFullSquare;

                        assert(false);
                      }();

                      auto reco_calc = reco.As<DTCalc>();
                      auto reco_graph = Qn::ToTGraph(reco_calc);

                      auto ratio_calc = reco_calc / mc_ref_calc;
                      ratio_calc.SetErrors(Qn::StatCalculate::ErrorType::BOOTSTRAP);

                      auto ratio_graph = Qn::ToTGraph(ratio_calc);

                      ratio_graph->SetTitle(PlotTitle(reco).c_str());
                      ratio_graph->SetLineColor(draw_color);
                      ratio_graph->SetMarkerColor(draw_color);
                      ratio_graph->SetMarkerStyle(draw_marker_style);

                      reco_graph->SetTitle(PlotTitle(reco).c_str());
                      reco_graph->SetLineColor(draw_color);
                      reco_graph->SetMarkerColor(draw_color);
                      reco_graph->SetMarkerStyle(draw_marker_style);


                      /* ratio */
                      c_ratio_map.at(reco_component)->cd();
                      ratio_graph->DrawClone("pZ");

                      c_overview->cd();
                      reco_graph->DrawClone("pZ");
                      delete ratio_graph;
                      delete reco_graph;
                    },
                    meta_key_gen == meta_key &&
                        META["type"] == "v1_centrality" &&
                        META["v1.src"] == "reco" &&
                        centrality_predicate &&
                        (
//                            (META["v1.set"] == "standard") ||
                        (META["v1.set"] == "standard" && META["v1.resolution.meta_key"] == "psd_mc")
                    ) &&
                        (
                            META["v1.component"].Matches("^(x1x1|y1y1)$") && META["v1.ref"].Matches("psd[0-9]") ||
                                META["v1.component"] == "combined" && META["v1.ref"] == "combined"
                        ));
              }

              c_overview->BuildLegend();
              c_overview->Print((meta_key + ".png").c_str(), "png");
              c_overview->Print((meta_key + ".C").c_str(), "C");
              for (auto &ratio: c_ratio_map) {
                ratio.second->BuildLegend();
                ratio.second->Print((meta_key + "__" + ratio.first + ".png").c_str(), "png");
                ratio.second->Print((meta_key + "__" + ratio.first + ".C").c_str(), "C");
                delete ratio.second;
              }
              delete c_overview;
            },
            META["type"] == "v1_centrality" &&
                META["v1.src"] == "mc" &&
                centrality_predicate &&
                META["v1.component"] == "combined");
  }

  /***************** SAVING OUTPUT *****************/
  {
    using ::Tools::ToRoot;
    gResourceManager.ForEach(ToRoot<Qn::DataContainerStatCalculate>("correlation_proc.root"));
    gResourceManager.ForEach(ToRoot<TGraphErrors>("prof.root"));
    gResourceManager.ForEach(ToRoot<TGraphAsymmErrors>("prof.root", "UPDATE"));
  }
  return 0;
}
