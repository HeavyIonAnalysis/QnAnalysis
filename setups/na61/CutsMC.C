#include "AnalysisTree/Cuts.hpp"
#include "CutsRegistry.hpp"


int CutsL() {
    using namespace AnalysisTree;

    SimpleCut vtx_z(Variable("RecEventHeaderProc","vtx_z"), -594., -590.);
    SimpleCut geant4_Epsd_bug({"RecEventHeaderProc/M", "RecEventHeaderProc/Epsd"}, [] (std::vector<double> &args) -> bool {
      float x = args[0];
      float y = args[1];
      if (y < 2356 - (2356.0 - 380.0)/430 * x) {
        return false;
      }
      return true;
    });

    RegisterCuts("na61/pbpb/13agev/mc/standard/event", Cuts("RecEventHeaderProc", {
                vtx_z,
                geant4_Epsd_bug
    }));


    return 0;
}
