//
// Created by eugene on 06/05/2021.
//

#include "v1_mc.hpp"
#include "using.hpp"
#include "Observables.hpp"

Tmpltor v1_mc_key() {
  return Tmpltor("/{{type}}/{{v1.src}}/{{v1.particle}}/AX_{{v1.axis}}/systematics/{{v1.component}}");
}
MetaFeatureSet v1_mc_feature_set() {
  return {{"v1.particle",
                        "v1.axis",
      // systematics
                        "v1.component"}};
}

void v1_mc() {
  /***************** Directed flow (MC) ******************/
  const std::string re_expr(R"(^/calc/uQ/(mc_\w+)_PLAIN\.psi_rp_PLAIN\.(x1x1|y1y1)$)");
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

    auto new_key = v1_mc_key()(new_res);

    AddResource(new_key, new_res);
  }, KEY.Matches(re_expr));
}


