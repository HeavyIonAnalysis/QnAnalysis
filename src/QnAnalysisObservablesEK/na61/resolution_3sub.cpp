//
// Created by eugene on 06/05/2021.
//

#include "resolution_3sub.hpp"
#include "using.hpp"

#include "Observables.hpp"
#include "Format.hpp"
#include "QnAnalysisTools/CombinePermute.hpp"

#include <TSystem.h>

#include <string>

using std::string;



void resolution_3sub() {
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

    for (auto&&[q_component, ref_args] : Qn::Analysis::Tools::Combination(q_components, q_permutations)) {
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
      Tools::Define(resolution, Methods::Resolution3S, {arg1_name, arg2_name, arg3_name}, meta);
    }
  };
  build_3sub_resolution("3sub_standard",{"psd1", "psd2", "psd3"});

}
