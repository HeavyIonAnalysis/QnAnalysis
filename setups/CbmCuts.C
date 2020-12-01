#include "AnalysisTree/SimpleCut.hpp"
#include "AnalysisTree/Cuts.hpp"
#include "CutsRegistry.hpp"

#include <TMath.h>

int CbmCuts() {
  using namespace AnalysisTree;

  {
    const char *branch = "AnaEventHeader";
    const char *name = "default";

    std::vector<SimpleCut> cuts;
    SimpleCut vtx_x_cut({branch, "vtx_x"}, -0.5, 0.5);
    SimpleCut vtx_y_cut({branch, "vtx_y"}, -0.5, 0.5);
    SimpleCut vtx_z_cut({branch, "vtx_z"}, -0.03, 0.03);
    SimpleCut vtx_chi2_cut({branch, "vtx_chi2"}, 0.8, 1.7);

//     const char *cuts_name = "hades/auau/1.23/event_cuts/standard";
    const char *cuts_name = "default";
    std::string branch_name = "AnaEventHeader";
    RegisterCuts(cuts_name, Cuts(branch_name, {
        vtx_x_cut,
        vtx_y_cut,
        vtx_z_cut,
        vtx_chi2_cut}));
  }

  return 0;
}