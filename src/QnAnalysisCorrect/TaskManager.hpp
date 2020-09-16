#ifndef FLOW_SRC_CORRECT_CORRECTTASKMANAGER_H_
#define FLOW_SRC_CORRECT_CORRECTTASKMANAGER_H_

#include "QnCorrectionTask.hpp"
#include <AnalysisTree/TaskManager.hpp>

namespace Qn::Analysis::Correction {

class TaskManager : public AnalysisTree::TaskManager {

 public:
  TaskManager(const std::vector<std::string>& filelists,
              const std::vector<std::string>& in_trees) : AnalysisTree::TaskManager(filelists, in_trees) {}

  void AddTask(QnCorrectionTask* task);
};

}// namespace Qn
#endif//FLOW_SRC_CORRECT_CORRECTTASKMANAGER_H_
