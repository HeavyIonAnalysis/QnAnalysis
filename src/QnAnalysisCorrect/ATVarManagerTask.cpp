//
// Created by eugene on 16/09/2020.
//

#include "ATVarManagerTask.h"

using namespace Qn::Analysis::Correction;

TASK_IMPL(ATVarManagerTask)

void ATVarManagerTask::Init(std::map<std::string, void *> &Map) {
  AnalysisTree::VarManager::Init(Map);
}
void ATVarManagerTask::Exec() {
  AnalysisTree::VarManager::Exec();
}
void ATVarManagerTask::Finish() {
  AnalysisTree::VarManager::Finish();
};
