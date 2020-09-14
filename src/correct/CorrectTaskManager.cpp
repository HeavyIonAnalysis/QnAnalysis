#include "CorrectTaskManager.h"

void Qn::CorrectTaskManager::AddTask(Qn::CorrectionTask* task) {
  assert(tasks_.empty()); // For the moment one task per Manager

  auto* var_task = new AnalysisTree::VarManager();

  // Variables used by tracking Q-vectors
  for (auto& qvec : task->GetConfig()->QvectorsConfig()) {
    const auto& vars = qvec.GetListOfVariables();
    qvec.SetVarEntryId( var_task->AddEntry(AnalysisTree::VarManagerEntry(vars)).first );
  }
  // Variables used by channelized Q-vectors
  for (auto& channel : task->GetConfig()->ChannelConfig()) {
    channel.SetVarEntryId( var_task->AddEntry(AnalysisTree::VarManagerEntry({channel.GetWeightVar()})).first );
  }
  // Psi variable
  if(task->GetConfig()->IsSimulation()){
    auto& qvec = task->GetConfig()->PsiQvector();
    qvec.SetVarEntryId( var_task->AddEntry(AnalysisTree::VarManagerEntry({qvec.GetPhiVar()})).first );
  }
  auto event_var_id = var_task->AddEntry(AnalysisTree::VarManagerEntry(task->GetConfig()->GetEventVars())).first;

  var_task->FillBranchNames();
  var_task->SetCutsMap(cuts_map_);

  task->SetPointerToVarManager(var_task);

  tasks_.emplace_back(var_task); //Needs to be executed first
  tasks_.emplace_back(task);
}
