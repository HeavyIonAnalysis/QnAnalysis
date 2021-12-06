//
// Created by eugene on 16/09/2020.
//

#ifndef QNANALYSIS_SRC_QNANALYSISCORRECT_ATVARMANAGERTASK_H
#define QNANALYSIS_SRC_QNANALYSISCORRECT_ATVARMANAGERTASK_H

#include <at_task/Task.h>
#include <QnAnalysisBase/AnalysisTree.hpp>

namespace Qn::Analysis::Correction {

class
ATVarManagerTask :
    public ATVarManager,
    public UserTask {

public:
  ANALYSISTREE_FILLTASK *FillTaskPtr() final { return this; }
  void Init(std::map<std::string, void *> &Map) override;
  void Exec() override;
  void Finish() override;

TASK_DEF(ATVarManagerTask, 1)
};

}

#endif //QNANALYSIS_SRC_QNANALYSISCORRECT_ATVARMANAGERTASK_H
