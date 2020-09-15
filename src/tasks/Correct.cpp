#include "CbmCuts.h"
#include "QnTools/Axis.hpp"
#include "correct/TaskManager.h"

#include "AnalysisSetup.h"
#include "AnalysisTree/DataHeader.hpp"
#include "Axis.h"
#include "QVector.h"

#include <config/Config.h>

using namespace AnalysisTree;

int main(int argc, char** argv) {

  std::string config_file = "analysis-config.yml";
  std::string config_node_name = "test";

  if (argc <= 1) {
    std::cout << "Not enough arguments! Please use:" << std::endl;
    std::cout << "   ./correct filelist1 treename1 .. filelistN treenameN" << std::endl;
    return -1;
  }

  std::vector<std::string> filelists{}, treenames{};
  for (int i = 1; i < argc; i += 2) {
    filelists.emplace_back(argv[i]);
    treenames.emplace_back(argv[i + 1]);
  }

  // main configuration object
  auto* global_config = new Flow::Base::AnalysisSetup(Flow::Config::ReadSetupFromFile(config_file, config_node_name));

  Qn::CorrectTaskManager man(filelists, treenames);
  //  man.SetEventCuts(GetCbmEventCuts("RecEventHeader"));
  //  man.AddBranchCut( GetCbmEventCuts("RecEventHeader"));
  //  man.AddBranchCut( GetCbmTrackCuts("VtxTracks"));
  man.AddBranchCut(GetCbmMcTracksCuts("SimParticles"));

  auto* task = new Qn::CorrectionTask(global_config);
  man.AddTask(task);

  man.Init();
  man.Run(-1);
  man.Finish();

  return 0;
}
