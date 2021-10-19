#ifndef CORRECTION_TASK_H
#define CORRECTION_TASK_H

#include <array>
#include <random>
#include <string>
#include <vector>

#include <TFile.h>
#include <TTree.h>
#include <TTreeReader.h>
#include <TChain.h>

#include <QnTools/CorrectionManager.hpp>

#include <AnalysisTree/DataHeader.hpp>
#include <QnAnalysisBase/AnalysisTree.hpp>


#include <QnAnalysisBase/AnalysisSetup.hpp>
#include <QnAnalysisBase/QVector.hpp>

#include <at_task/Task.h>

namespace Qn::Analysis::Correction {
/**
 * Qn vector analysis TestTask. It is to be configured by the user.
 * @brief TestTask for analysing qn vectors
 */

class QnCorrectionTask : public UserFillTask {
 public:
  QnCorrectionTask() = default;
  explicit QnCorrectionTask(Base::AnalysisSetup* global_config) : analysis_setup_(global_config) {}

  void AddQAHistogram(const std::string& qvec_name, const std::vector<AxisD>& axis) {
    qa_histos_.emplace_back(qvec_name, axis);
  }
  boost::program_options::options_description GetBoostOptions() override;
  void PreInit() override;
 protected:
  bool UseATI2() const override { return false; }
 public:
  void UserInit(std::map<std::string, void*>&) override;
  void UserExec() override;
  void UserFinish() override;

  void SetPointerToVarManager(ATVarManager* ptr) { var_manager_ = ptr; }

  Base::AnalysisSetup* GetConfig() { return analysis_setup_; }

 protected:
  void FillTracksQvectors();
  void SetCorrectionSteps(const Base::QVector& qvec);
  void InitVariables();
  void AddQAHisto();

  std::string yaml_config_file_;
  std::string yaml_config_node_;

  std::string qa_file_name_;
  std::shared_ptr<TFile> out_file_;
  std::string in_calibration_file_name_{"correction_in.root"};

  TTree* out_tree_{nullptr};
  Qn::CorrectionManager manager_;

  Base::AnalysisSetup* analysis_setup_{nullptr};
  ATVarManager* var_manager_{nullptr};
  std::vector<std::tuple<std::string, std::vector<AxisD>>> qa_histos_;
  std::map<int, int> is_filled_{};

  TASK_DEF(QnCorrectionTask, 2)
};
}// namespace Qn
#endif//CORRECTION_TASK_H
