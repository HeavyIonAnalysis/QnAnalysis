#ifndef CORRECTION_TASK_H
#define CORRECTION_TASK_H

#include <vector>
#include <array>
#include <random>
#include <string>

#include <TChain.h>
#include "TFile.h"
#include "TTree.h"
#include "TTreeReader.h"

#include "QnTools/CorrectionManager.hpp"

#include "AnalysisTree/VarManager.hpp"
#include <AnalysisTree/DataHeader.hpp>

#include "QVector.h"
#include "AnalysisSetup.h"

namespace Qn {
/**
 * Qn vector analysis TestTask. It is to be configured by the user.
 * @brief TestTask for analysing qn vectors
 */

class CorrectionTask : public AnalysisTree::FillTask {
 public:

  CorrectionTask() = delete;
  explicit CorrectionTask(Flow::Base::AnalysisSetup* global_config) : global_config_(global_config) {}

  void AddQAHistogram(const std::string& qvec_name, const std::vector<AxisD>& axis){
    qa_histos_.emplace_back(qvec_name, axis );
  }

  void Init(std::map<std::string, void*>&) override;
  void Exec() override;
  void Finish() override;

  void SetPointerToVarManager(AnalysisTree::VarManager* ptr) { var_manager_ = ptr; }

  Flow::Base::AnalysisSetup* GetConfig() { return global_config_; }
 protected:

  void FillTracksQvectors();
  void SetCorrectionSteps(const Flow::Base::QVector& qvec);
  void InitVariables();
  void AddQAHisto();

  std::shared_ptr<TFile> out_file_{nullptr};
  std::string in_calibration_file_name_{"correction_in.root"};

  TTree *out_tree_{nullptr};
  Qn::CorrectionManager manager_;

  Flow::Base::AnalysisSetup* global_config_{nullptr};
  AnalysisTree::VarManager* var_manager_{nullptr};
  std::vector<std::tuple<std::string, std::vector<AxisD>>> qa_histos_;
  std::map<int, int> is_filled_{};

};
}
#endif //CORRECTION_TASK_H
