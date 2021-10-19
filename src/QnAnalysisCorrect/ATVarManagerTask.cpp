//
// Created by eugene on 16/09/2020.
//

#include "ATVarManagerTask.hpp"

using namespace Qn::Analysis::Correction;

TASK_IMPL(ATVarManagerTask)

void ATVarManagerTask::Init(std::map<std::string, void *> &Map) {
  ATVarManager::Init(Map);
}
void ATVarManagerTask::Exec() {
  ATVarManager::Exec();
}
void ATVarManagerTask::Finish() {
  ATVarManager::Finish();
};
