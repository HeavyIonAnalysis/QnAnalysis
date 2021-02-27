//
// Created by eugene on 24/11/2020.
//

#include <DataContainer.hpp>
#include <DataContainerHelper.hpp>

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

  /* label correlations */
  gResourceManager.ForEach([](StringKey key, ResourceManager::Resource &r) {
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





  /* To check compatibility with old stuff, multiply raw correlations by factor of 2 */
  gResourceManager.ForEach([](VectorKey key, ResourceManager::Resource calc) {
    key[0] = "x2";
    calc.obj = 2 * calc.As<DTCalc>();
    AddResource(key, calc);
  });

  {
    /* Projection _y correlations to pT axis  */
    gResourceManager.ForEach([](StringKey name, DTCalc &calc) {
                               calc = calc.Projection({"Centrality_Centrality_Epsd", "RecParticles_y_cm"});
                             },
                             KEY.Matches(R"(/calc/uQ/(\w+)_y_(\w+)\..*)"));
    /* Projection _pT correlations to 'y' axis  */
    gResourceManager.ForEach([](StringKey name, DTCalc &calc) {
                               calc = calc.Projection({"Centrality_Centrality_Epsd", "RecParticles_pT"});
                             },
                             KEY.Matches(R"(/calc/uQ/(\w+)_pt_(\w+)\..*)"));
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
    /***************** RESOLUTION 4-sub ******************/

    /*
     * Prepare different references from TPC to use in 4-subevents method
     */
    gResourceManager.ForEach([](VectorKey key, const ResourceManager::Resource &r) {
      using Resource = ResourceManager::Resource;

      auto calc = r.As<DTCalc>();

      auto meta = r.meta;
      meta.put("type", "4sub_tpc_ref");

      {
        auto selected = calc.Select(Qn::AxisD("RecParticles_y_cm", 1, 0.8, 1.2));
        VectorKey new_key = {"resolution", "4sub_protons_08_12", key.back()};
        meta.put("4sub_meta_key", "4sub_protons_08_12");
        AddResource(new_key, Resource(selected, meta));
      }
      {
        auto selected = calc.Select(Qn::AxisD("RecParticles_y_cm", 1, 0.4, 0.8));
        VectorKey new_key = {"resolution", "4sub_protons_04_08", key.back()};
        meta.put("4sub_meta_key", "4sub_protons_04_08");
        AddResource(new_key, Resource(selected, meta));
      }
      {
        auto selected = calc.Select(Qn::AxisD("RecParticles_y_cm", 1, 0.0, 0.4));
        VectorKey new_key = {"resolution", "4sub_protons_00_04", key.back()};
        meta.put("4sub_meta_key", "4sub_protons_00_04");
        AddResource(new_key, Resource(selected, meta));
      }
    }, KEY.Matches("/calc/uQ/protons_y_RESCALED\\.(psd[1-3])_RECENTERED\\.(x1x1|y1y1)$"));

    for (auto &&[resolution_meta_key, component] : Tools::Combination(gResourceManager.SelectUniq(META["4sub_meta_key"],
                                                                                                  META["type"]
                                                                                                      == "4sub_tpc_ref"),
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
//        meta.put("v1.particle", particle);
        meta.put("v1.component", projection);
//        meta.put("v1.axis", axis);

        VectorKey key = {"v1", u_vector_base, "systematics", resolution_method, reference, projection};
        Define(key, Methods::v1, {u_vector, resolution}, meta);
      }
    }
  }

  /* Combine x1x1 and y1y1 */
  {
    gResourceManager.GroupBy(
        BASE_OF(KEY),
        [](const std::string &feature, std::vector<ResourceManager::ResourcePtr> &objs) {
          std::cout << "feature" << std::endl;
        },
        META["type"] == "v1");
  }
/****************** DRAWING *********************/

/* export everything to TGraph */
  gResourceManager.ForEach([](StringKey name, DTCalc calc) {
                             auto graph = Qn::ToTGraph(calc);
                             AddResource("/profiles" + name, graph);
                           },
                           KEY.Matches("^/x2/QQ/.*$"));

  /* export ALL resolution to TGraph */
  gResourceManager.ForEach([](StringKey name, ResourceManager::Resource r) {
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
  gResourceManager.ForEach([](VectorKey key, Qn::DataContainerStatCalculate &calc) {
    auto centrality_axis = calc.GetAxes()[0];
    for (size_t ic = 0; ic < centrality_axis.size(); ++ic) {
      auto c_lo = centrality_axis.GetLowerBinEdge(ic);
      auto c_hi = centrality_axis.GetUpperBinEdge(ic);
      auto centrality_range_str = (Format("%1%-%2%") % c_lo % c_hi).str();

      auto selected = calc.Select(Qn::AxisD(centrality_axis.Name(), 1, c_lo, c_hi));
      auto selected_graph = Qn::ToTGraph(selected);
      selected_graph->SetTitle(centrality_range_str.c_str());
      selected_graph->GetXaxis()->SetTitle(calc.GetAxes()[1].Name().c_str());
      selected_graph->GetYaxis()->SetRangeUser(-0.2, 0.2);

      VectorKey new_key(key.begin(), key.end());
      new_key.insert(new_key.begin(), "profiles");
      new_key.insert(std::find(new_key.begin(), new_key.end(), "systematics"), centrality_range_str);
      AddResource(new_key, selected_graph);
    }
  }, META["type"] == "v1");

  gResourceManager.Print();

  /***************** SAVING OUTPUT *****************/
  {
    using ::Tools::ToRoot;
    gResourceManager.ForEach(ToRoot<Qn::DataContainerStatCalculate>("correlation_proc.root"));
    gResourceManager.ForEach(ToRoot<TGraphErrors>("prof.root"));
    gResourceManager.ForEach(ToRoot<TGraphAsymmErrors>("prof.root", "UPDATE"));
  }
  return 0;
}
