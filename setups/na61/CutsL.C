#include "AnalysisTree/Cuts.hpp"
#include "CutsRegistry.hpp"


int CutsL() {
    using namespace AnalysisTree;

    SimpleCut vtx_z(Variable("RecEventHeaderProc","vtx_z"), -594., -590.);
    SimpleCut geant4_Epsd_bug({"RecEventHeaderProc/M", "RecEventHeaderProc/Epsd"}, [] (std::vector<double> &args) -> bool {
      if (args[0] < 70 && args[1] < 1800) {
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
