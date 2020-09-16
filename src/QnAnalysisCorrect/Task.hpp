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

#include <AnalysisTree/VarManager.hpp>
#include <AnalysisTree/DataHeader.hpp>

#include <QnAnalysisBase/AnalysisSetup.hpp>
#include <QnAnalysisBase/QVector.hpp>

namespace Qn::Analysis::Correction {
/**
 * Qn vector analysis TestTask. It is to be configured by the user.
 * @brief TestTask for analysing qn vectors
 */

class Task : public AnalysisTree::FillTask {
 public:
  Task() = delete;
  explicit Task(Base::AnalysisSetup* global_config) : global_config_(global_config) {}

  void AddQAHistogram(const std::string& qvec_name, const std::vector<AxisD>& axis) {
    qa_histos_.emplace_back(qvec_name, axis);
  }

  void Init(std::map<std::string, void*>&) override;
  void Exec() override;
  void Finish() override;

  void SetPointerToVarManager(AnalysisTree::VarManager* ptr) { var_manager_ = ptr; }

  Base::AnalysisSetup* GetConfig() { return global_config_; }

 protected:
  void FillTracksQvectors();
  void SetCorrectionSteps(const Base::QVector& qvec);
  void InitVariables();
  void AddQAHisto();

  std::shared_ptr<TFile> out_file_{nullptr};
  std::string in_calibration_file_name_{"correction_in.root"};

  TTree* out_tree_{nullptr};
  Qn::CorrectionManager manager_;

  Base::AnalysisSetup* global_config_{nullptr};
  AnalysisTree::VarManager* var_manager_{nullptr};
  std::vector<std::tuple<std::string, std::vector<AxisD>>> qa_histos_;
  std::map<int, int> is_filled_{};
};
}// namespace Qn
#endif//CORRECTION_TASK_H
