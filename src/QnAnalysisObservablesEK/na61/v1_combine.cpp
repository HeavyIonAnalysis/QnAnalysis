//
// Created by eugene on 06/05/2021.
//

#include "v1_combine.hpp"
#include "v1_mc.hpp"
#include "v1_reco.hpp"
#include "v1_centrality.hpp"
#include "using.hpp"
#include "Observables.hpp"

void v1_combine() {
  auto v1_key_generator = v1_centrality_reco_key();
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

  v1_key_generator = v1_key();
  gResourceManager.GroupBy(
      v1_reco_feature_set() - "v1.component",
      combine_components_function,
      META["type"] == "v1" && META["v1.src"] == "reco" && META["v1.component"].Matches("^(x1x1|y1y1)$"));

  v1_key_generator = v1_mc_key();
  gResourceManager.GroupBy(
      v1_mc_feature_set() - "v1.component",
      combine_components_function,
      META["type"] == "v1" && META["v1.src"] == "mc" && META["v1.component"].Matches("^(x1x1|y1y1)$"));

  v1_key_generator = v1_centrality_mc_key();
  gResourceManager.GroupBy(
      v1_centrality_mc_feature_set() - "v1.component",
      combine_components_function,
      META["type"] == "v1_centrality" && META["v1.src"] == "mc" && META["v1.component"].Matches("^(x1x1|y1y1)$"));

  v1_key_generator = v1_centrality_reco_key();
  gResourceManager.GroupBy(
      v1_centrality_reco_feature_set() - "v1.component",
      combine_components_function,
      META["type"] == "v1_centrality" && META["v1.src"] == "reco" && META["v1.component"].Matches("^(x1x1|y1y1)$"));

  v1_key_generator = v1_centrality_mc_key();
  gResourceManager.GroupBy(
      v1_centrality_mc_feature_set() - "v1.component",
      combine_components_function,
      META["type"] == "v1_centrality" && META["v1.src"] == "mc" && META["v1.component"].Matches("^(x1x1|y1y1)$"));



  /* Combine all references */
  auto combine_reference_function =
      [&v1_key_generator](auto feature, std::vector<ResourceManager::ResourcePtr> &objs) {
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
        auto new_name = v1_key_generator(new_resource);
        AddResource(new_name, new_resource);
      };

  v1_key_generator = v1_centrality_reco_key();
  gResourceManager.GroupBy(
      v1_centrality_reco_feature_set() - "v1.ref",
      combine_reference_function,
      META["type"] == "v1_centrality" && META["v1.src"] == "reco");

  v1_key_generator = v1_key();
  gResourceManager.GroupBy(
      v1_reco_feature_set() - "v1.ref",
      combine_reference_function,
      META["type"] == "v1" && META["v1.src"] == "reco");
}

