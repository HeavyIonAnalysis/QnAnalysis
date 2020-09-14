#ifndef FLOW_SRC_CORRECT_CORRECTTASKMANAGER_H_
#define FLOW_SRC_CORRECT_CORRECTTASKMANAGER_H_

#include "CorrectionTask.h"
#include <AnalysisTree/TaskManager.hpp>

namespace Qn {

class CorrectTaskManager : public AnalysisTree::TaskManager {

 public:
  CorrectTaskManager(const std::vector<std::string>& filelists,
                     const std::vector<std::string>& in_trees) : TaskManager(filelists, in_trees) {}

  void AddTask(Qn::CorrectionTask* task);
};

}// namespace Qn
#endif//FLOW_SRC_CORRECT_CORRECTTASKMANAGER_H_
