//
// Created by eugene on 06/05/2021.
//

#include "v1_centrality.hpp"
#include "using.hpp"
#include "Observables.hpp"
#include "v1_reco.hpp"
#include "v1_mc.hpp"

Tmpltor v1_centrality_reco_key() {
  return Tmpltor("/{{type}}/{{v1.src}}/{{v1.particle}}/AX_{{v1.axis}}/centrality_{{centrality.key}}/systematics/"
                 "SET_{{v1.set}}/RES_{{v1.resolution.meta_key}}/REF_{{v1.ref}}/{{v1.component}}");
}
MetaFeatureSet v1_centrality_reco_feature_set() {
  return v1_reco_feature_set() + "centrality.no";
}

Tmpltor v1_centrality_mc_key() {
  return Tmpltor("/{{type}}/{{v1.src}}/{{v1.particle}}/AX_{{v1.axis}}/centrality_{{centrality.key}}/systematics/{{v1.component}}");
}

MetaFeatureSet v1_centrality_mc_feature_set() {
  return v1_mc_feature_set() + "centrality.no";
}


void v1_centrality() {
  auto key_generator = v1_centrality_reco_key();
  auto expand_centrality_projection = [&key_generator](const VectorKey &key, ResourceManager::Resource &res) {
    auto calc = res.As<DTCalc>();
    auto meta = res.meta;

    auto centrality_axis = res.As<DTCalc>().GetAxes()[0];
    for (size_t ic = 0; ic < centrality_axis.size(); ++ic) {
      auto c_lo = centrality_axis.GetLowerBinEdge(ic);
      auto c_hi = centrality_axis.GetUpperBinEdge(ic);
      auto centrality_range_str = (Format("centrality_%05.1f-%05.1f") % c_lo % c_hi).str();
      auto centrality_key = (Format("%05.1f-%05.1f") % c_lo % c_hi).str();

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
  key_generator = v1_centrality_mc_key();
  gResourceManager.ForEach(expand_centrality_projection,
                           META["type"] == "v1" && META["v1.src"] == "mc");

}

