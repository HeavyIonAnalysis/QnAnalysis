//
// Created by eugene on 24/11/2020.
//

#include <DataContainer.hpp>
#include <DataContainerHelper.hpp>

#include <any>
#include <filesystem>
#include <map>
#include <regex>
#include <tuple>
#include <utility>

#include <TClass.h>
#include <TDirectory.h>
#include <TFile.h>
#include <TKey.h>

#include <QnAnalysisTools/QnAnalysisTools.hpp>

#include "Observables.hpp"
#include "Tools.hpp"

namespace Tools {

namespace Details {

template<typename Tuple, std::size_t IArg>
void SetArgI(const std::vector<std::string> &arg_names, Tuple &tuple) {
  using ArgT = std::tuple_element_t<IArg, Tuple>;
  auto &manager = ResourceManager::Instance();
  try {
    std::get<IArg>(tuple) = manager.Get(arg_names[IArg], ResourceManager::ResTag<ArgT>());
  } catch (std::bad_any_cast &e) {
    throw ResourceManager::NoSuchResource(arg_names[IArg]);
  }
}
template<typename Tuple, std::size_t... IArg>
void SetArgTupleImpl(const std::vector<std::string> &arg_names, Tuple &tuple, std::index_sequence<IArg...>) {
  (SetArgI<Tuple, IArg>(arg_names, tuple), ...);
}
template<typename... Args>
void SetArgTuple(const std::vector<std::string> &arg_names, std::tuple<Args...> &tuple) {
  SetArgTupleImpl(arg_names, tuple, std::make_index_sequence<sizeof...(Args)>());
}
}// namespace Details

template<typename KeyRepr, typename Function>
auto Define(const KeyRepr &key, Function &&fct, std::vector<std::string> arg_names, bool warn_at_missing = true) {
  using ArgsTuple = typename ::Details::FunctionTraits<decltype(std::function{fct})>::ArgumentsTuple;
  ArgsTuple args;
  try {
    Details::SetArgTuple(arg_names, args);
    return AddResource(key, std::apply(fct, args));
  } catch (ResourceManager::NoSuchResource &e) {
    if (warn_at_missing) {
      Warning(__func__, "Resource '%s' required for '%s' is missing, new resource won't be added",
              e.what(), ::Details::Convert<KeyRepr>::ToString(key).c_str());
      return ResourceManager::ResourcePtr();
    } else {
      throw e; /* rethrow */
    }
  }
}

template<typename KeyRepr, typename Function>
void Define1(const KeyRepr &key, Function &&fct, std::vector<std::string> arg_names, bool warn_at_missing = true) {
  using Traits = ::Details::FunctionTraits<decltype(std::function{fct})>;
  using Container = std::decay_t<typename Traits::template ArgType<0>>;
  using Entity = typename Container::value_type;
  Container args_container;
  /* lookup arguments */
  auto args_back_inserter = std::back_inserter(args_container);
  for (auto &arg : arg_names) {
    try {
      *args_back_inserter = ResourceManager::Instance().template Get<std::string, Entity>(arg);
    } catch (ResourceManager::NoSuchResource &e) {
      if (warn_at_missing) {
        Warning(__func__, "Resource '%s' required for '%s' is missing, new resource won't be added",
                e.what(), key.c_str());
        return;
      } else {
        throw e;
      }
    }
  }
  AddResource(key, fct(args_container));
}

}// namespace Tools

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
  using Predicates::RegexMatch;

  using DTCalc = Qn::DataContainerStatCalculate;
  using DTColl = Qn::DataContainerStatCollect;



  TFile f("correlation.root", "READ");
  LoadROOTFile<Qn::DataContainerStatCollect>(f.GetName(), "raw");

  /* Convert everything to Qn::DataContainerStatCalculate */
  gResourceManager.ForEach([](VectorKey key, DTColl &collect) {
    /* replacing /raw with /calc */
    key[0] = "calc";
    AddResource(key, Qn::DataContainerStatCalculate(collect));
  });
  /* To check compatibility with old stuff, multiply raw correlations by factor of 2 */
  gResourceManager.ForEach([](VectorKey key, DTCalc calc) {
    key[0] = "x2";
    auto x2 = 2 * calc;
    AddResource(key, x2);
  });

  {
    /* Projection _y correlations to pT axis  */
    gResourceManager.ForEach([](StringKey name, DTCalc &calc) {
                               calc = calc.Projection({"Centrality_Centrality_Epsd", "RecParticles_y_cm"});
                             },
                             RegexMatch(R"(/calc/uQ/(\w+)_y_(\w+)\..*)"));
    /* Projection _pT correlations to 'y' axis  */
    gResourceManager.ForEach([](StringKey name, DTCalc &calc) {
                               calc = calc.Projection({"Centrality_Centrality_Epsd", "RecParticles_pT"});
                             },
                             RegexMatch(R"(/calc/uQ/(\w+)_pt_(\w+)\..*)"));
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
      auto resolution = (Format("/resolution/3sub/RES_%1%_%2%") % subA % component).str();
      auto result = ::Tools::Define(resolution, Methods::Resolution3S, {arg1_name, arg2_name, arg3_name});
      if (result) {
        result->meta.put("resolution.ref", subA);
        result->meta.put("resolution.projection", component);
      }
    }
  }

  {
    /***************** RESOLUTION 4-sub ******************/
    gResourceManager.ForEach([](VectorKey key, DTCalc &calc) {
      auto selected = calc.Select(Qn::AxisD("RecParticles_y_cm", 1, 0.8, 1.2));
      VectorKey new_key = {"resolution", "4sub_protons", key.back()};
      AddResource(new_key, selected);
    }, RegexMatch("/calc/uQ/protons_y_RESCALED\\.(psd[1-3])_RECENTERED\\.(x1x1|y1y1)$"));

    Define(StringKey("/resolution/4sub_protons/RES_TPC.x1x1"), Methods::Resolution3S,
           {"/resolution/4sub_protons/protons_y_RESCALED.psd1_RECENTERED.x1x1",
            "/resolution/4sub_protons/protons_y_RESCALED.psd3_RECENTERED.x1x1",
            "/calc/QQ/psd1_RECENTERED.psd3_RECENTERED.x1x1"});
    Define(StringKey("/resolution/4sub_protons/RES_TPC.y1y1"), Methods::Resolution3S,
           {"/resolution/4sub_protons/protons_y_RESCALED.psd1_RECENTERED.y1y1",
            "/resolution/4sub_protons/protons_y_RESCALED.psd3_RECENTERED.y1y1",
            "/calc/QQ/psd1_RECENTERED.psd3_RECENTERED.y1y1"});
    Define(StringKey("/resolution/4sub_protons/RES_psd1_x1x1"), Methods::Resolution4S,
           {"/calc/QQ/psd1_RECENTERED.psd3_RECENTERED.x1x1",
            "/resolution/4sub_protons/RES_TPC.x1x1",
            "/resolution/4sub_protons/protons_y_RESCALED.psd3_RECENTERED.x1x1"});
    Define(StringKey("/resolution/4sub_protons/RES_psd2_x1x1"), Methods::Resolution4S_1,
           {"/resolution/4sub_protons/protons_y_RESCALED.psd2_RECENTERED.x1x1",
            "/resolution/4sub_protons/RES_TPC.x1x1"});
    Define(StringKey("/resolution/4sub_protons/RES_psd3_x1x1"), Methods::Resolution4S,
           {"/calc/QQ/psd1_RECENTERED.psd3_RECENTERED.x1x1",
            "/resolution/4sub_protons/RES_TPC.x1x1",
            "/resolution/4sub_protons/protons_y_RESCALED.psd1_RECENTERED.x1x1"});
    Define(StringKey("/resolution/4sub_protons/RES_psd1_y1y1"), Methods::Resolution4S,
           {"/calc/QQ/psd1_RECENTERED.psd3_RECENTERED.y1y1",
            "/resolution/4sub_protons/RES_TPC.y1y1",
            "/resolution/4sub_protons/protons_y_RESCALED.psd3_RECENTERED.y1y1"});
    Define(StringKey("/resolution/4sub_protons/RES_psd2_y1y1"), Methods::Resolution4S_1,
           {"/resolution/4sub_protons/protons_y_RESCALED.psd2_RECENTERED.y1y1",
            "/resolution/4sub_protons/RES_TPC.y1y1"});
    Define(StringKey("/resolution/4sub_protons/RES_psd3_y1y1"), Methods::Resolution4S,
           {"/calc/QQ/psd1_RECENTERED.psd3_RECENTERED.y1y1",
            "/resolution/4sub_protons/RES_TPC.y1y1",
            "/resolution/4sub_protons/protons_y_RESCALED.psd1_RECENTERED.y1y1"});
  }

  {
    std::vector<std::string> resolution_methods{"3sub", "4sub_protons", "4sub_pion_neg"};
    std::vector<std::string> references{"psd1", "psd2", "psd3"};
    std::vector<std::string> u_correction_step{"RECENTERED", "RESCALED"};
    std::vector<std::string> axes{"pt", "y"};
    std::vector<std::string> particles{"protons", "pion_neg"};
    std::vector<std::string> projections{"x1x1", "y1y1"};

    for (auto&&[resolution_method, reference, u_cstep, projection, axis, particle] :
        Tools::Combination(resolution_methods, references, u_correction_step, projections, axes, particles)) {
      auto u_query =
          RegexMatch((Format("/calc/uQ/%4%_%5%_%1%\\.%2%_RECENTERED.%3%") % u_cstep % reference % projection % particle
              % axis).str());
      auto res_query =
          RegexMatch((Format("/resolution/%3%/RES_%1%_%2%") % reference % projection % resolution_method).str());
      for (auto &&[u_vector, resolution] : Tools::Combination(gResourceManager.GetMatching(u_query),
                                                              gResourceManager.GetMatching(res_query))) {
        VectorKey key = {"v1", resolution_method, "u-" + u_cstep,
                         (Format("v1_%1%_%2%_%4%_%3%") % particle % axis % projection
                             % reference).str()};
        auto result = Define(key, Methods::v1, {u_vector, resolution});
        if (result) {
          result->meta.put("v1.ref", reference);
          result->meta.put("v1.particle", particle);
          result->meta.put("v1.component", projection);
          result->meta.put("v1.axis", axis);
        }
      }
    }
  }
/****************** DRAWING *********************/

  {
    auto result = gResourceManager.SelectUniq(META["type"]);
    std::cout << std::endl;

  }

/* export everything to TGraph */
  gResourceManager.ForEach([](StringKey name, DTCalc calc) {
                             auto graph = Qn::ToTGraph(calc);
                             AddResource("/profiles" + name, graph);
                           },
                           RegexMatch("^/x2/QQ/.*$"));

  /* export everything to TGraph */
  gResourceManager.ForEach([](StringKey name, DTCalc calc) {
    auto graph = Qn::ToTGraph(calc);
    graph->GetYaxis()->SetRangeUser(-0.1, 1.0);
    AddResource("/profiles" + name, graph);
  }, META["type"] == "resolution");

  /* v1 vs Centrality */
  gResourceManager.ForEach([](StringKey name, Qn::DataContainerStatCalculate &calc) {
    auto centrality_axis = calc.GetAxes()[0];
    for (size_t ic = 0; ic < centrality_axis.size(); ++ic) {
      auto c_lo = centrality_axis.GetLowerBinEdge(ic);
      auto c_hi = centrality_axis.GetUpperBinEdge(ic);
      auto selected = calc.Select(Qn::AxisD(centrality_axis.Name(), 1, c_lo, c_hi));
      auto selected_graph = Qn::ToTGraph(selected);
      selected_graph->SetTitle((Format("%1%-%2%") % c_lo % c_hi).str().c_str());
      selected_graph->GetXaxis()->SetTitle(calc.GetAxes()[1].Name().c_str());
      selected_graph->GetYaxis()->SetRangeUser(-0.2, 0.2);
      AddResource((Format("/profiles%1%/%2%_%3%") % name % centrality_axis.Name() % ic).str(),
                  selected_graph);
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
