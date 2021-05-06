//
// Created by eugene on 06/05/2021.
//

#include "resolution_4sub.hpp"
#include "using.hpp"

#include "Observables.hpp"

void resolution_4sub() {
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
        for (auto &bin : calc) {
          bin.SetWeightType(Qn::StatCalculate::WeightType::REFERENCE);
        }
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
