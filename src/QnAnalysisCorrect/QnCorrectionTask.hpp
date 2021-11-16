#ifndef CORRECTION_TASK_H
#define CORRECTION_TASK_H

#include <array>
#include <random>
#include <string>
#include <utility>
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

#include <at_task/Task.h>

namespace Qn::Analysis::Correction {
/**
 * Qn vector analysis TestTask. It is to be configured by the user.
 * @brief TestTask for analysing qn vectors
 */

class QnCorrectionTask : public UserFillTask {
 public:
  QnCorrectionTask() = default;
  explicit QnCorrectionTask(Base::AnalysisSetup *global_config) : analysis_setup_(global_config) {}

  void AddQAHistogram(const std::string &qvec_name, const std::vector<AxisD> &axis) {
    qa_histos_.emplace_back(qvec_name, axis);
  }
  boost::program_options::options_description GetBoostOptions() override;
  void PreInit() override;
  void UserInit(std::map<std::string, void *> &) override;
  void UserExec() override;
  void UserFinish() override;

  void SetPointerToVarManager(AnalysisTree::VarManager *ptr) { var_manager_ = ptr; }

  Base::AnalysisSetup *GetConfig() { return analysis_setup_; }

 protected:

  struct ValueSource {
    virtual double GetValue() const = 0;
  };

  typedef std::shared_ptr<ValueSource> ValueSourcePtr;

  struct ValueSourceRef :
      public ValueSource {
    explicit ValueSourceRef(ValueSourcePtr ptr) : ptr(std::move(ptr)) {}

    double GetValue() const final {
      return ptr->GetValue();
    }

    ValueSourcePtr Ptr() const {
      return ptr;
    }

    ValueSourcePtr ptr;
  };

  struct ATI2ValueSourceImpl :
      public ValueSource {
    explicit ATI2ValueSourceImpl(ATI2::Variable v) : v(std::move(v)) {}
    ATI2ValueSourceImpl(ATI2::Variable v, size_t i_channel) : v(std::move(v)), i_channel(i_channel) {}

    std::string GetVariableName() const {
      return v.GetName();
    }

    /**
     * @brief assigns value from specific BranchChannel (i_channel is ignored)
     * @param channel
     */
    void Update(ATI2::BranchChannel &channel) {
      cached_value = channel[v];
      is_set = true;
    }
    /**
     * @brief Assigns value from event-header-like branch or specific channel (i_channel)
     * @param branch
     */
    void Update(ATI2::Branch &branch) {
      if (branch.GetBranchType() == AnalysisTree::DetType::kEventHeader) {
        cached_value = branch[v];
      } else {
        cached_value = branch[i_channel][v];
      }
      is_set = true;
    }
    double GetValue() const override {
      return cached_value;
    }

    ATI2::Variable v;
    size_t i_channel{0ul};

    bool is_set{false};
    double cached_value{-999};
  };

  struct ValueSink {
    virtual void Reset() {};
    virtual void AssignValue(double value) = 0;
  };

  typedef std::shared_ptr<ValueSink> ValueSinkPtr;

  struct ValueSinkRef :
      public ValueSink {
    explicit ValueSinkRef(std::shared_ptr<ValueSink> ptr) : ptr(std::move(ptr)) {}

    void AssignValue(double value) final {
      ptr->AssignValue(value);
    }
    void Reset() override {
      ptr->Reset();
    }
    ValueSinkPtr Ptr() const {
      return ptr;
    }

    std::shared_ptr<ValueSink> ptr;
  };

  struct QnVariable :
      public ValueSink {
    QnVariable(const std::string &name, size_t idx, size_t ichannel) : name(name), idx(idx), ichannel(ichannel) {}

    std::string name;
    size_t idx;
    size_t ichannel;

    double *data{};
    bool is_set{false};

    void Reset() override {
      is_set = false;
    }
    void AssignValue(double value) override {
      if (is_set) {
        throw std::runtime_error("Variable '" + name + "' already set");
      }
      data[idx + ichannel] = value;
      is_set = true;
    }
  };
  typedef std::shared_ptr<QnVariable> QnDataPtr;

  struct MappingContext {
    MappingContext(ATI2::Branch *branch_ptr, QnDataPtr lock_qn_variable)
        : branch_ptr(branch_ptr), lock_qn_variable(std::move(lock_qn_variable)) {
    }

    std::string GetBranchName() const {
      return branch_ptr->GetBranchName();
    }


    bool AddMapping(const ValueSourcePtr &source, const ValueSinkPtr &sink) {
      for (auto && [m_source, m_sink] : mappings_) {
        if (sink == m_sink.Ptr()) {
          return false;
        }
      }
      mappings_.emplace_back(source, sink);
      return true;
    }
    bool AddMapping(const ATI2::Variable &source, const ValueSinkPtr &sink) {
      std::shared_ptr<ATI2ValueSourceImpl> source_ptr{std::make_shared<ATI2ValueSourceImpl>(source)};
      if (AddMapping(source_ptr, sink)) {
        ati2_sources_.emplace_back(source_ptr);
        return true;
      }
      return false;
    }

   public:
    ATI2::Branch *branch_ptr{nullptr};
    QnDataPtr lock_qn_variable;

    std::list<std::pair<ValueSourceRef, ValueSinkRef>> mappings_;
    std::list<std::shared_ptr<ATI2ValueSourceImpl>> ati2_sources_;
  };

  void FillTracksQvectors();
  void SetCorrectionSteps(const Base::QVector &qvec);
  void InitVariables();
  void AddQAHisto();



  std::vector<QnDataPtr> AddQnVariable(const std::string &name, size_t length = 1) {
    auto new_idx = qn_variables_.empty() ? 0 : qn_variables_.back()->idx + 1;
    manager_.AddVariable(name, new_idx, length);

    std::vector<QnDataPtr> result(length);
    for (auto ichannel = 0; ichannel < length; ichannel++) {
      result[ichannel] = std::make_shared<QnVariable>(name, new_idx, ichannel);
    }
    copy(begin(result), end(result), back_inserter(qn_variables_));
    return result;
  }

  QnDataPtr FindFirstQnVariableByName(std::string_view name) {
    auto it = std::find_if(begin(qn_variables_), end(qn_variables_), [name](const QnDataPtr &qn_data_ptr) {
      return qn_data_ptr->name == name;
    });
    if (it != end(qn_variables_)) {
      return *it;
    }
    return {};
  }

  std::string yaml_config_file_;
  std::string yaml_config_node_;

  std::string qa_file_name_;
  std::shared_ptr<TFile> out_file_;
  std::string in_calibration_file_name_{"correction_in.root"};

  TTree *out_tree_{nullptr};



  Qn::CorrectionManager manager_;

  Base::AnalysisSetup *analysis_setup_{nullptr};
  AnalysisTree::VarManager *var_manager_{nullptr};
  std::vector<std::tuple<std::string, std::vector<AxisD>>> qa_histos_;
  std::map<int, int> is_filled_{};

  std::vector<QnDataPtr> qn_variables_;
  std::vector<MappingContext> track_loop_contexts_;

 TASK_DEF(QnCorrectionTask, 2)
};
}// namespace Qn
#endif//CORRECTION_TASK_H
