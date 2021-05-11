//
// Created by eugene on 11/05/2021.
//

#include "plot_v1_centrality.hpp"
#include "Observables.hpp"
#include "using.hpp"



void plot_v1_centrality() {
  ::Tools::ToRoot<TMultiGraph> root_saver("v1_centrality.root", "RECREATE");


  gResourceManager.ForEach([=,&root_saver] (const StringKey &key, ResourceManager::Resource& data) {


  }, META["type"] == "v1" &&
      META["v1.axis"] == "2d" &&
      META["v1.component"] == "combined" &&
      META["v1.ref"] == "combined");
}
