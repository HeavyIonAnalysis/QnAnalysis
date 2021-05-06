//
// Created by eugene on 06/05/2021.
//

#include "remap_axis.hpp"
#include "using.hpp"

#include "Observables.hpp"



void remap_axes() {
/* Remap axis */
  gResourceManager.ForEach([](const StringKey &, DTCalc &dt) {
    const std::map<std::string, std::string> axis_name_map{
        {"RecEventHeaderProc_Centrality_Epsd", "Centrality"},
        {"Centrality_Centrality_Epsd", "Centrality"},
        {"SimTracksProc_y_cm", "y_cm"},
        {"SimTracksProc_pT", "pT"},
        {"RecParticles_y_cm", "y_cm"},
        {"RecParticles_pT", "pT"},
    };

    for (auto &ax : dt.GetAxes()) {
      auto name = ax.Name();

      auto map_it = axis_name_map.find(name);
      if (map_it != axis_name_map.end()) {
        ax.SetName(map_it->second);
      } else {
        assert(false);
      }
    } // axis

  });
}
