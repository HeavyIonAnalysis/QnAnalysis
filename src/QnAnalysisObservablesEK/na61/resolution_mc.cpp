//
// Created by eugene on 06/05/2021.
//

#include "resolution_mc.hpp"
#include "using.hpp"

#include "Observables.hpp"
#include "QnAnalysisTools/CombinePermute.hpp"


void resolution_mc() {
  auto build_psd_mc_resolution = [](
      const std::string &meta_key,
      const std::vector<std::string> &base_q_vectors,
      const std::map<std::string, std::string> &ref_alias_map = {}
  ) {
    std::vector<std::string> components = {"x1x1", "y1y1"};
    for (auto&&[q_component, base_q_vector] : Qn::Analysis::Tools::Combination(components, base_q_vectors)) {

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
      ::Tools::Define(name, [](const DTCalc &calc) { return 2 * calc; }, {arg_name}, meta);
    } // component, base_q_vector
  };

  build_psd_mc_resolution("psd_mc", {"psd1", "psd2", "psd3"});
}
