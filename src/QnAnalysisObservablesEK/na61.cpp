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
      std::string arg_name = match_results.str(1);
      std::string arg_cstep = match_results.str(2);
      r.meta.put("arg" + std::to_string(iarg) + ".name", arg_name);
      r.meta.put("arg" + std::to_string(iarg) + ".c-step", arg_cstep);
    }
    r.meta.put("component", tokens.back());
  });

  {
    const std::regex u_reco_expr("^/calc/uQ/(pion_neg|protons)_(pt|y)_set_(\\w+)_(PLAIN|RECENTERED|TWIST|RESCALED).*$");
    gResourceManager.ForEach([&u_reco_expr](const StringKey &key, ResourceManager::Resource &r) {
      auto particle = KEY.MatchGroup(1, u_reco_expr)(r);
      auto axis = KEY.MatchGroup(2, u_reco_expr)(r);
      auto set = KEY.MatchGroup(3, u_reco_expr)(r);
      r.meta.put("type", "uQ");
      r.meta.put("u.particle", particle);
      r.meta.put("u.axis", axis);
      r.meta.put("u.set", set);
    }, KEY.Matches(u_reco_expr));
  }
  {
    const std::regex u_mc_expr("^/calc/uQ/mc_(pion_neg|protons)_(pt|y)_PLAIN.*$");
    gResourceManager.ForEach([&u_mc_expr](const StringKey &key, ResourceManager::Resource &r) {
      auto particle = KEY.MatchGroup(1, u_mc_expr)(r);
      auto axis = KEY.MatchGroup(2, u_mc_expr)(r);
      r.meta.put("type", "uQ");
      r.meta.put("u.particle", particle);
      r.meta.put("u.axis", axis);
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
    /***************** RESOLUTION 3-sub ******************/
    std::vector<std::tuple<string, string, string>> args_list = {
        {"psd1", "psd2", "psd3"},
        {"psd2", "psd1", "psd3"},
        {"psd3", "psd1", "psd2"},
    };
    std::vector<std::string> components = {"x1x1", "y1y1"};

    for (auto&&[component, ref_args] : Tools::Combination(components, args_list)) {
      std::string subA, subB, subC;
      std::tie(subA, subB, subC) = ref_args;
      auto arg1_name = (Format("/calc/QQ/%1%_RECENTERED.%2%_RECENTERED.%3%") % subA % subB % component).str();
      auto arg2_name = (Format("/calc/QQ/%1%_RECENTERED.%2%_RECENTERED.%3%") % subA % subC % component).str();
      auto arg3_name = (Format("/calc/QQ/%1%_RECENTERED.%2%_RECENTERED.%3%") % subB % subC % component).str();
      auto resolution = (Format("/resolution/3sub_standard/RES_%1%_%2%") % subA % component).str();

      Meta meta;
      meta.put("resolution.ref", subA);
      meta.put("resolution.component", component == "x1x1" ? "X" : "Y");
      meta.put("resolution.meta_key", "3sub_standard");

      Define(resolution, Methods::Resolution3S, {arg1_name, arg2_name, arg3_name}, meta);
    }
  }

  {
    /***************** RESOLUTION MC ******************/
    std::vector<std::string> reference = {
        "psd1", "psd2", "psd3",
        "psd1_90", "psd2_90", "psd3_90"};
    std::vector<std::string> components = {"x1x1", "y1y1"};

    for (auto&&[component, reference] : Tools::Combination(components, reference)) {

      Meta meta;
      meta.put("type", "resolution");
      meta.put("resolution.ref", reference);
      meta.put("resolution.component", component == "x1x1" ? "X" : "Y");
      meta.put("resolution.method", "mc");
      meta.put("resolution.meta_key", "mc");

      auto name = (Format("/resolution/mc/RES_%1%_%2%") % reference % component).str();
      auto arg_name = (Format("/calc/QQ/%1%_RECENTERED.psi_rp_PLAIN.%2%") % reference % component).str();
      Define(name, [](const DTCalc &calc) { return 2 * calc; }, {arg_name}, meta);
    }

  }

  {
    /***************** RESOLUTION 4-sub ******************/

    /*
     * Prepare different references from TPC to use in 4-subevents method
     */
    gResourceManager.ForEach([](VectorKey key, const ResourceManager::Resource &r) {
      using Resource = ResourceManager::Resource;

      auto calc = r.As<DTCalc>();

      auto meta = r.meta;
      meta.put("type", "4sub_tpc_ref");

      auto reference_name = "reference_" + META["arg1.name"](r);
      auto component = META["component"](r);

      {
        auto selected = calc.Select(Qn::AxisD("RecParticles_y_cm", 1, 0.8, 1.2));
        VectorKey new_key = {"resolution", "4sub_protons_08_12", "4sub", reference_name, component};
        meta.put("4sub_meta_key", "4sub_protons_08_12");
        AddResource(new_key, Resource(selected, meta));
      }
      {
        auto selected = calc.Select(Qn::AxisD("RecParticles_y_cm", 1, 0.4, 0.8));
        VectorKey new_key = {"resolution", "4sub_protons_04_08", "4sub", reference_name, component};
        meta.put("4sub_meta_key", "4sub_protons_04_08");
        AddResource(new_key, Resource(selected, meta));
      }
      {
        auto selected = calc.Select(Qn::AxisD("RecParticles_y_cm", 1, 0.0, 0.4));
        VectorKey new_key = {"resolution", "4sub_protons_00_04", "4sub", reference_name, component};
        meta.put("4sub_meta_key", "4sub_protons_00_04");
        AddResource(new_key, Resource(selected, meta));
      }
    }, KEY.Matches("/calc/uQ/protons_y_standard_RESCALED\\.(psd[1-3])_RECENTERED\\.(x1x1|y1y1)$"));

    for (auto &&[resolution_meta_key, component] : Tools::Combination(
        gResourceManager.SelectUniq(META["4sub_meta_key"], META["type"] == "4sub_tpc_ref"),
        std::vector<std::string>{"x1x1", "y1y1"})) {

      auto key_generator =
          "/resolution/" + META["resolution.meta_key"] +
              "/RES_" + META["resolution.ref"] + "_" + META["resolution.component"];

      Meta meta;
      meta.put("resolution.meta_key", resolution_meta_key);
      meta.put("resolution.component", component);

      meta.put("resolution.ref", "TPC");
      Define(key_generator, Methods::Resolution3S,
             {"/resolution/" + resolution_meta_key + "/protons_y_RESCALED.psd1_RECENTERED." + component,
              "/resolution/" + resolution_meta_key + "/protons_y_RESCALED.psd3_RECENTERED." + component,
              "/calc/QQ/psd1_RECENTERED.psd3_RECENTERED." + component}, meta);
      meta.put("resolution.ref", "psd1");
      Define(key_generator, Methods::Resolution4S,
             {"/calc/QQ/psd1_RECENTERED.psd3_RECENTERED." + component,
              "/resolution/" + resolution_meta_key + "/RES_TPC_" + component,
              "/resolution/" + resolution_meta_key + "/protons_y_RESCALED.psd3_RECENTERED." + component}, meta);
      meta.put("resolution.ref", "psd2");
      Define(key_generator, Methods::Resolution4S_1,
             {"/resolution/" + resolution_meta_key + "/protons_y_RESCALED.psd2_RECENTERED." + component,
              "/resolution/" + resolution_meta_key + "/RES_TPC_" + component}, meta);
      meta.put("resolution.ref", "psd3");
      Define(key_generator, Methods::Resolution4S,
             {"/calc/QQ/psd1_RECENTERED.psd3_RECENTERED." + component,
              "/resolution/" + resolution_meta_key + "/RES_TPC_" + component,
              "/resolution/" + resolution_meta_key + "/protons_y_RESCALED.psd1_RECENTERED." + component}, meta);
    }

  }
  const auto v1_key_generator =
      "/" + META["type"] + "/reco/" +
          META["v1.particle"] + "/" +
          "AX_" + META["v1.axis"] + "/" +
          "systematics" + "/" +
          "SET_" + META["v1.set"] + "/"
                                    "RES_" + META["v1.resolution.meta_key"] + "/" +
          "REF_" + META["v1.ref"] + "/" +
          META["v1.component"];

  {
    /***************** Directed flow ******************/
    // folder structure /v1/<particle>/<axis>
    const auto resolution_predicate = (META["type"] == "resolution");
    auto resolution_methods = gResourceManager.SelectUniq(META["resolution.meta_key"], resolution_predicate);
    auto references = gResourceManager.SelectUniq(META["resolution.ref"], resolution_predicate);
    std::vector<std::string> projections{"x1x1", "y1y1"};

    auto u_vectors = gResourceManager.SelectUniq(KEY.MatchGroup(1, "/calc/uQ/(\\w+)_RESCALED.*$"),
                                                 KEY.Matches("^/calc/uQ/.*"));

    for (auto&&[resolution_method, reference, projection, u_vector_base] :
        Tools::Combination(resolution_methods, references, projections, u_vectors)) {
      auto u_query =
          KEY.Matches((Format("/calc/uQ/%1%_RESCALED\\.%2%_RECENTERED.%3%") % u_vector_base % reference
              % projection).str());
      auto res_query =
          KEY.Matches((Format("/resolution/%3%/RES_%1%_%2%") % reference % projection % resolution_method).str());
      for (auto &&[u_vector, resolution] : Tools::Combination(gResourceManager.GetMatching(u_query),
                                                              gResourceManager.GetMatching(res_query))) {
        Meta meta;
        meta.put("v1.ref", reference);
        meta.put("v1.component", projection);
        meta.put("v1.src", "reco");
        Define(v1_key_generator, Methods::v1, {u_vector, resolution}, meta);
      }
    }
  } // directed flow

  {
    /***************** Coeffiecient c1 ******************/
    gResourceManager.GroupBy(
        META["u.particle"] + "__" +
            META["u.axis"] + "__" +
            META["arg1.name"],
        [v1_key_generator](const std::string &meta_key,
                           const std::vector<ResourceManager::ResourcePtr> &list_resources) {
          for (auto &uQ_resource : list_resources) {
            auto reference = META["arg1.name"](*uQ_resource);
            auto uQ_component = META["component"](*uQ_resource);
            auto resolution_component = (uQ_component == "x1y1") ? "Y" : "X";

            auto resolution_keys = gResourceManager
                .SelectUniq(KEY, META["type"] == "resolution" &&
                    META["resolution.method"] == "mc" &&
                    META["resolution.ref"] == reference &&
                    META["resolution.component"] == resolution_component);
            assert(resolution_keys.size() == 1);

            Meta meta;
            meta.put("type", "c1");
            meta.put("v1.ref", reference);
            meta.put("v1.component", uQ_component);
            meta.put("v1.src", "reco");
            Define(v1_key_generator, Methods::v1, {KEY(*uQ_resource), resolution_keys[0]}, meta);
          }
        },
        META["type"] == "uQ" &&
            META["component"].Matches("(x1y1|y1x1)") &&
            META["arg0.c-step"] == "RESCALED"
    );

  }

  {
    /***************** Directed flow (MC) ******************/
    const std::regex re_expr(R"(^/calc/uQ/(mc_\w+)_PLAIN\.psi_rp_PLAIN\.(x1x1|y1y1)$)");
    gResourceManager.ForEach([re_expr](const StringKey &key, ResourceManager::Resource &r) {
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

      auto new_key = ("/v1/mc/" + META["v1.particle"] + "/" + "AX_" + META["v1.axis"] + "/" + "systematics" + "/"
          + META["v1.component"])(new_res);

      AddResource(new_key, new_res);
    }, KEY.Matches(re_expr));
  }



  /******************** v1 vs Centrality *********************/
  {

    gResourceManager.ForEach([](VectorKey key, ResourceManager::Resource &res) {
      auto calc = res.As<DTCalc>();
      auto meta = res.meta;

      auto centrality_axis = res.As<DTCalc>().GetAxes()[0];
      for (size_t ic = 0; ic < centrality_axis.size(); ++ic) {
        auto c_lo = centrality_axis.GetLowerBinEdge(ic);
        auto c_hi = centrality_axis.GetUpperBinEdge(ic);
        auto centrality_range_str = (Format("centrality_%1%-%2%") % c_lo % c_hi).str();

        auto selected = calc.Select(Qn::AxisD(centrality_axis.Name(), 1, c_lo, c_hi));

        meta.put("type", META["type"](res) + "_centrality");
        meta.put("centrality.no", std::to_string(ic));
        meta.put("centrality.lo", c_lo);
        meta.put("centrality.hi", c_hi);

        VectorKey new_key(key.begin(), key.end());
        new_key[0] = META["type"](res) + "_centrality";
        new_key.insert(std::find(new_key.begin(), new_key.end(), "systematics"), centrality_range_str);

        AddResource(new_key, ResourceManager::Resource(selected, meta));
      }
    }, META["type"] == "v1" || META["type"] == "c1");
  }

  /* Combine x1x1 and y1y1 */
  {
    auto
        combine_components_function = [](const std::string &base_dir, std::vector<ResourceManager::ResourcePtr> &objs) {
      auto x1x1 = objs[0]->As<DTCalc>();
      auto y1y1 = objs[1]->As<DTCalc>();
      auto combined_name = base_dir + "/" + "combined";
      auto combined = x1x1.Apply(y1y1, [](const Qn::StatCalculate &a, const Qn::StatCalculate &b) {
        return Qn::Merge(a, b);
      });
      auto combined_meta = objs[0]->meta;
      combined_meta.put("v1.component", "combined");
      auto result = AddResource(combined_name, ResourceManager::Resource(combined, combined_meta));

      for (auto &obj : objs) {
        auto ratio = obj->As<DTCalc>() / combined;
        auto ratio_name = base_dir + "/" + "ratio_" + META["v1.component"](*obj);
        auto ratio_meta = obj->meta;
        ratio_meta.put("type", "ratio_v1");
        AddResource(ratio_name, ResourceManager::Resource(ratio, ratio_meta));
      }
    };

    gResourceManager.GroupBy(
        BASE_OF(KEY),
        combine_components_function,
        META["type"] == "v1_centrality" && KEY.Matches(".*(x1x1|y1y1)$"));
    /* Combine all references */
    std::string component = "x1x1";
    auto combine_reference_function =
        [&component](const std::string &base_dir, std::vector<ResourceManager::ResourcePtr> &objs) {
          if (base_dir == "NOT-FOUND") {
            return;
          }
          assert(!objs.empty());
          auto combined_psd_name = base_dir + "/" + "REF_combined" + "/" + component;
          auto combined = objs[0]->As<DTCalc>(); // taking copy of first object
          for (int iobjs = 1; iobjs < objs.size(); ++iobjs) {
            combined =
                combined.Apply(objs[iobjs]->As<DTCalc>(), [](const Qn::StatCalculate &a, const Qn::StatCalculate &b) {
                  return Qn::Merge(a, b);
                });
          }
          auto combined_meta = objs[0]->meta;
          combined_meta.put("v1.ref", "combined");
          combined_meta.put("v1.resolution.ref", "combined");
          AddResource(combined_psd_name, ResourceManager::Resource(combined, combined_meta));

          for (auto &ref : objs) {
            auto ratio_ref_combined = ref->As<DTCalc>() / combined;
            auto ratio_meta = ref->meta;
            ratio_meta.put("type", "ratio_v1");
            auto ratio_key_name =
                base_dir + "/" + "REF_combined" + "/" + "ratio_" + META["v1.ref"](*ref) + "_" + component;
            AddResource(ratio_key_name, ResourceManager::Resource(ratio_ref_combined, ratio_meta));
          }
        };
    const std::regex re_ref("^(.*)/REF_psd\\d/.*$");
    gResourceManager.GroupBy(
        KEY.MatchGroup(1, re_ref),
        combine_reference_function,
        META["type"] == "v1_centrality" && META["v1.component"] == component);
    component = "y1y1";
    gResourceManager.GroupBy(
        KEY.MatchGroup(1, re_ref),
        combine_reference_function,
        META["type"] == "v1_centrality" && META["v1.component"] == component);
    /* Combine x1x1 and y1y1 in combined reference path */
    gResourceManager.GroupBy(
        BASE_OF(KEY),
        combine_components_function,
        META["type"] == "v1_centrality" && META["v1.ref"] == "combined");
  }

/****************** DRAWING *********************/

/* export everything to TGraph */
  gResourceManager.ForEach([](const StringKey &name, DTCalc calc) {
                             auto graph = Qn::ToTGraph(calc);
                             if (graph)
                               AddResource("/profiles" + name, graph);
                           },
                           KEY.Matches("^/x2/QQ/.*$"));

  /* export ALL resolution to TGraph */
  gResourceManager.ForEach([](const StringKey &name, ResourceManager::Resource r) {

    auto graph = Qn::ToTGraph(r.As<DTCalc>());

    auto component = r.meta.get("resolution.component", "??");
    auto reference = r.meta.get("resolution.ref", "??");
    auto method = r.meta.get("resolution.method", "??");

    graph->SetTitle((Format("R_{1,%1%} (%2%) %3%")
        % component
        % reference
        % method).str().c_str());

    graph->GetXaxis()->SetTitle("Centrality (%)");

    graph->GetYaxis()->SetRangeUser(-0.1, 1.0);
    graph->GetYaxis()->SetTitle("R_{1}");

    AddResource("/profiles" + name, ResourceManager::Resource(*graph, r.meta));
  }, META["type"] == "resolution");

  /* v1 profiles */
  // /profiles/v1/<particle>/<axis>/<centrality range>/
  gResourceManager.ForEach([](VectorKey key, ResourceManager::Resource &resource) {
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
    const std::map<std::pair<std::string, std::string>, int> markers_map = {
        {{"psd1", "x1x1"}, kFullSquare},
        {{"psd1", "y1y1"}, kOpenSquare},
        {{"psd1", "combined"}, kFullStar},
        {{"psd2", "x1x1"}, kFullCircle},
        {{"psd2", "y1y1"}, kOpenCircle},
        {{"psd2", "combined"}, kFullStar},
        {{"psd3", "x1x1"}, kFullTriangleDown},
        {{"psd3", "y1y1"}, kOpenTriangleDown},
        {{"psd3", "combined"}, kFullStar},
        {{"combined", "x1x1"}, kFullDiamond},
        {{"combined", "y1y1"}, kOpenDiamond},
        {{"combined", "combined"}, kOpenDiamond},
        {{"psi_rp", "x1x1"}, kFullSquare},
        {{"psi_rp", "y1y1"}, kOpenSquare},
        {{"psi_rp", "combined"}, kFullStar}
    };

    auto selected_graph = Qn::ToTGraph(resource.As<DTCalc>());
    if (!selected_graph) {
      return;
    }
    selected_graph->SetName(key.back().c_str());
    selected_graph->GetXaxis()->SetTitle(remap_axis_name.at(resource.As<DTCalc>().GetAxes()[0].Name()).c_str());
//    selected_graph->SetLineColor(colors_map.at(META["v1.ref"](resource)));
//    selected_graph->SetMarkerColor(colors_map.at(META["v1.ref"](resource)));

    VectorKey new_key(key.begin(), key.end());
    new_key.insert(new_key.begin(), "profiles");
    auto meta = resource.meta;
    if (META["type"](resource) == "v1_centrality") {
      meta.put("type", "profile_v1");
    } else if (META["type"](resource) == "ratio_v1") {
      meta.put("type", "profile_ratio");
    }
    AddResource(new_key, ResourceManager::Resource(*selected_graph, meta));
  },
                           META["type"] == "v1_centrality" ||
                           META["type"] == "c1_centrality");

  gResourceManager.ForEach([](const StringKey &, TGraphErrors &graph) {
    graph.GetYaxis()->SetRangeUser(-0.1, 0.1);
  }, META["type"] == "profile_v1" && META["v1.particle"] == "pion_neg");

  gResourceManager.ForEach([](const StringKey &, TGraphErrors &graph) {
    graph.GetYaxis()->SetRangeUser(-0.1, 0.3);
  }, META["type"] == "profile_v1" && META["v1.particle"] == "proton");

  gResourceManager.ForEach([](const StringKey &, TGraphErrors &graph) {
    graph.GetYaxis()->SetRangeUser(0., 2.);
  }, META["type"] == "profile_ratio");

  /* Compare MC vs Data */
  {
    auto meta_key_gen = META["v1.particle"] + "__" + META["v1.axis"];
    auto centrality_predicate = META["centrality.no"] == "3";

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
                  {"protons__y", {-0.05, 0.3}},
                  {"pion_neg__pt", {-0.15, 0.15}},
                  {"pion_neg__y", {-0.15, 0.15}},
              };
              for (auto &&[component, canv_ptr] : c_ratio_map) {
                canv_ptr->SetCanvasSize(1280, 1024);
              }

              bool overview_first = true;
              for (auto &mc_ref : mc_correlations) {
                auto mc_ref_calc = mc_ref->As<DTCalc>();

                auto get_plot_title = [](const ResourceManager::Resource &r) -> std::string {
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
                    std::string part_2;
                    if (META["v1.resolution.meta_key"](r) == "mc") {
                      part_2 = "R_{1} (MC)";
                    } else if (META["v1.resolution.method"](r) == "3sub") {
                      part_2 = "R_{1} (3-sub)";
                    }
                    auto part_3 = "setup:" + META["v1.set"](r);
                    return part_1.append(" ").append(part_2).append(" ").append(part_3);
                  }
                  return "";
                };

                /* plot to overview */
                auto mc_ref_graph = Qn::ToTGraph(mc_ref_calc);

                const auto component = META["v1.component"](*mc_ref);

                auto line_style = kSolid;
                auto draw_opts = overview_first ? "Al" : "l";

                mc_ref_graph->GetXaxis()->SetTitle(mc_ref_calc.GetAxes()[0].Name().c_str());
                mc_ref_graph->GetYaxis()->SetTitle("v_{1}");
                mc_ref_graph->GetYaxis()->SetRangeUser(ranges_map.at(meta_key).first, ranges_map.at(meta_key).second);
                mc_ref_graph->SetLineStyle(line_style);
                mc_ref_graph->SetTitle(get_plot_title(*mc_ref).c_str());
                mc_ref_graph->SetLineWidth(2.);

                c_overview->cd();
                mc_ref_graph->DrawClone(draw_opts);
                overview_first = false;

                auto ratio_self_calc = mc_ref_calc / mc_ref_calc;
                auto ratio_self_graph = Qn::ToTGraph(ratio_self_calc);
                ratio_self_graph->SetTitle(get_plot_title(*mc_ref).c_str());
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
                      const auto draw_marker_style = [reco_component, reco_ref] () {
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

                      ratio_graph->SetTitle(get_plot_title(reco).c_str());
                      ratio_graph->SetLineColor(draw_color);
                      ratio_graph->SetMarkerColor(draw_color);
                      ratio_graph->SetMarkerStyle(draw_marker_style);

                      reco_graph->SetTitle(get_plot_title(reco).c_str());
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
                        (META["v1.set"] == "primaries" && META["v1.resolution.meta_key"] == "mc")
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

  gResourceManager.GroupBy(
      META["v1.particle"] + "__" +
          META["v1.axis"] + "__" +
          META["centrality.lo"] + "_" + META["centrality.hi"] + "__" +
          META["v1.component"],
      [](const std::string &f, std::vector<ResourceManager::ResourcePtr> &resources) {
        if (resources.empty())
          return;

        const std::map<std::string, int> map_colors{
            {"3sub_standard", kBlue},
            {"mc", kRed},
        };
        const std::map<std::string, int> map_linestyle{
            {"standard", kSolid},
            {"weighted", kDotted},
            {"primaries", kSolid},
        };

        TCanvas c(("c__" + f).c_str(), "");
        c.SetCanvasSize(1280, 1024);
        c.SetBatch(false);

        bool is_first = true;
        for (auto &res : resources) {
          auto draw_opts = is_first ? "Apl" : "pl";
          is_first = false;

          auto graph = (TGraphErrors *) res->As<TGraphErrors>().Clone();
          if (META["v1.src"](*res) == "mc") {
            graph->SetLineColor(kBlack);
            graph->SetMarkerColor(kBlack);
            graph->SetTitle("True MC");
          } else {
            auto resolution_meta_key = META["v1.resolution.meta_key"](*res);
            auto v1_set = META["v1.set"](*res);
            graph->SetLineColor(map_colors.at(resolution_meta_key));
            graph->SetLineStyle(map_linestyle.at(v1_set));
            graph->SetMarkerColor(map_colors.at(resolution_meta_key));
            graph->SetTitle((v1_set + " " + resolution_meta_key).c_str());
          }
          graph->Draw(draw_opts);
        }
        auto legend = c.BuildLegend(0.1, 0.7, 0.4, 0.9);
        legend->SetHeader(f.c_str());
        c.SaveAs(("plots/resolution_methods/" + f + ".png").c_str());
        c.SaveAs(("plots/resolution_methods/" + f + ".C").c_str());
      }, META["type"] == "profile_v1" &&
          META["centrality.no"] == "3" &&
          (META["v1.src"] == "mc" || (META["v1.src"] == "reco" && META["v1.ref"] == "combined")));

  gResourceManager.GroupBy(
      META["v1.particle"] + "__" +
          META["v1.axis"],
      [](const std::string &f, const std::vector<ResourceManager::ResourcePtr> &resources) {
        if (resources.empty())
          return;
        TCanvas c(("c__" + f).c_str(), "");
        c.SetCanvasSize(1280, 1024);
        c.SetBatch(false);

        bool is_first = true;
        for (auto &res : resources) {
          auto draw_opts = is_first ? "Apl" : "pl";
          is_first = false;

          auto graph = (TGraphErrors *) res->As<TGraphErrors>().Clone();
          graph->Draw(draw_opts);
        }
        c.SaveAs(("plots/v1_centrality/" + f + ".png").c_str());
        c.SaveAs(("plots/v1_centrality/" + f + ".C").c_str());
        c.SaveAs(("plots/v1_centrality/" + f + ".eps").c_str());
      },
      META["type"] == "profile_v1" &&
          META["v1.src"] == "reco" &&
          META["v1.ref"] == "combined" &&
          META["v1.component"] == "combined" &&
          META["v1.resolution.meta_key"] == "3sub_standard");

  /***************** SAVING OUTPUT *****************/

  using ::Tools::ToRoot;
  /// Save individual cases to separate ROOT file
  for (const auto &v1_case : gResourceManager.SelectUniq(KEY.MatchGroup(1, "^/profiles/v1_centrality/(\\w+)/.*$"))) {
    if (v1_case == "NOT-FOUND") continue;
    gResourceManager.ForEach(ToRoot<TGraphErrors>("v1_" + v1_case + ".root", "RECREATE",
                                                  "/profiles/v1_centrality/" + v1_case),
                             META["type"] == "profile_v1" &&
                                 META["v1.component"] == "combined" &&
                                 META["v1.ref"] == "combined");
  }

  {
    gResourceManager.ForEach(ToRoot<Qn::DataContainerStatCalculate>("correlation_proc.root"));
    gResourceManager.ForEach(ToRoot<TGraphErrors>("prof.root"));
    gResourceManager.ForEach(ToRoot<TGraphAsymmErrors>("prof.root", "UPDATE"));
  }
  return 0;
}
