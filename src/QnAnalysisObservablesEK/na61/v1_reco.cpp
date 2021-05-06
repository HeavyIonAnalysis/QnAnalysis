//
// Created by eugene on 06/05/2021.
//

#include "v1_reco.hpp"
#include "using.hpp"
#include "Observables.hpp"


Tmpltor v1_key() {
  return Tmpltor("/{{type}}/{{v1.src}}/{{v1.particle}}/AX_{{v1.axis}}/systematics/"
          "SET_{{v1.set}}/RES_{{v1.resolution.meta_key}}/REF_{{v1.ref}}/{{v1.component}}");
}

MetaFeatureSet v1_reco_feature_set() {
  return {{"v1.particle", "v1.axis",
              // systematics
              "v1.set", "v1.resolution.meta_key", "v1.ref", "v1.component"}};
}


void v1() {
  const auto uQ_filter = (
      META["type"] == "uQ" &&
          META["u.src"] == "reco" &&
          META["u.correction-step"] == "RESCALED") &&
      META["arg1.c-step"] == "RECENTERED";
  const auto resolution_filter = (META["type"] == "resolution");


  /* Lookup uQ */
  gResourceManager.ForEach([](const StringKey &uq_key, const ResourceManager::Resource &uQ_res) {
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
    gResourceManager.ForEach([uq_key, correlation_component](const StringKey &resolution_key,
                                                                                    const ResourceManager::Resource &res) {
      std::cout << resolution_key << std::endl;
      Meta meta;
      meta.put("v1.ref", META["resolution.ref"](res));
      meta.put("v1.component", correlation_component);
      meta.put("v1.src", "reco");
      Define(v1_key(), Methods::v1, {uq_key, resolution_key}, meta);
    }, resolution_filter);
  }, uQ_filter);

  /***************** Coeffiecient c1 ******************/
  gResourceManager.GroupBy(
      META["u.particle"] + "__" +
          META["u.axis"] + "__" +
          META["arg1.name"],
      [](const std::string &meta_key,
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
          Define(v1_key(), Methods::v1, {KEY(*uQ_resource), resolution_keys[0]}, meta);
        }
      },
      META["type"] == "uQ" &&
          META["component"].Matches("(x1y1|y1x1)") &&
          META["arg0.c-step"] == "RESCALED"
  );

}

