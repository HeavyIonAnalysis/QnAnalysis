//
// Created by eugene on 16/09/2020.
//

#include "ATVarManagerTask.hpp"

using namespace Qn::Analysis::Correction;

TASK_IMPL(ATVarManagerTask)

void ATVarManagerTask::UserInit(std::map<std::string, void *> &Map) {
  ATVarManager::Init(Map);
}
void ATVarManagerTask::UserExec() {
  ATVarManager::Exec();
}
void ATVarManagerTask::UserFinish() {
  ATVarManager::Finish();
};
