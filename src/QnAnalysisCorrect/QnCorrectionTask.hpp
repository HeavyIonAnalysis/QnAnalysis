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
  explicit QnCorrectionTask(Base::AnalysisSetup *global_config) : analysis_setup_(global_config) {}

  boost::program_options::options_description GetBoostOptions() override;
 protected:
  bool UseATI2() const override {
    return true;
  }
 public:
  void PreInit() override;
  void UserInit(std::map<std::string, void *> &) override;
  void UserExec() override;
  void UserFinish() override;


  Base::AnalysisSetup *GetConfig() { return analysis_setup_; }

 protected:

  class ValueSource {
   public:
    virtual ~ValueSource() = default;
    virtual void Notify() {}
    virtual double Value() const = 0;
  };

  typedef std::shared_ptr<ValueSource> ValueSourcePtr;

  class ValueSourceRef :
      public ValueSource {
   public:
    explicit ValueSourceRef(ValueSourcePtr ptr) : ptr(std::move(ptr)) {}

    double Value() const final {
      return ptr->Value();
    }

    ValueSourcePtr Ptr() const {
      return ptr;
    }
   private:
    ValueSourcePtr ptr;
  };

  class FunctionValueSourceImpl
      : public ValueSource {
   public:
    FunctionValueSourceImpl(std::function<double(void)> value, std::function<void(void)> notify) : value(std::move(
        value)), notify(std::move(notify)) {}
    explicit FunctionValueSourceImpl(std::function<double(void)> value) : value(std::move(value)) {}

    void Notify() override {
      if (notify) {
        notify();
      }
    }
    double Value() const override {
      return value();
    }

   private:
    std::function<double (void)> value;
    std::function<void (void)> notify;
  };

  class ATI2ValueSourceImpl :
      public ValueSource {
   public:
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
    void Update() {
      auto &branch = *v.GetParentBranch();
      if (branch.GetBranchType() == AnalysisTree::DetType::kEventHeader) {
        cached_value = branch[v];
      } else {
        cached_value = branch[i_channel][v];
      }
      is_set = true;
    }
    double Value() const override {
      return cached_value;
    }

   private:
    ATI2::Variable v;
    size_t i_channel{0ul};

    bool is_set{false};
    double cached_value{-999};
  };

  struct ValueSink {
    virtual ~ValueSink() = default;
    virtual void Reset() {};
    virtual void AssignValue(double value) = 0;
  };

  typedef std::shared_ptr<ValueSink> ValueSinkPtr;

  class ValueSinkRef :
      public ValueSink {
   public:
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

   private:
    std::shared_ptr<ValueSink> ptr;
  };

  class QnVariable :
      public ValueSink {
   public:
    QnVariable(std::string name, size_t idx, size_t ichannel) : name(std::move(name)), idx(idx), ichannel(ichannel) {}

    size_t GetIdx() const {
      return idx;
    }
    size_t GetIchannel() const {
      return ichannel;
    }
    const std::string &GetName() const {
      return name;
    }
    void SetData(double *data) {
      QnVariable::data = data;
    }

    void Reset() override {
      is_set = false;
    }
    void AssignValue(double value) override {
//      if (is_set) {
//        throw std::runtime_error("Variable '" + name + "' already set");
//      }
      data[idx + ichannel] = value;
      is_set = true;
    }

   private:
    std::string name;
    size_t idx;
    size_t ichannel;

    double *data{};
    bool is_set{false};
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

  void SetCorrectionSteps(const Base::QVector &qvec);
  void AddQAHisto();


  std::vector<QnDataPtr> AddQnVariable(const std::string &name, size_t length = 1);
  QnDataPtr FindFirstQnVariableByName(std::string_view name);
  static
  std::shared_ptr<ATI2ValueSourceImpl>
  FindATI2SourceByName(std::string_view variable_name, std::vector<std::shared_ptr<ATI2ValueSourceImpl>> collection);

  std::string yaml_config_file_;
  std::string yaml_config_node_;

  std::string qa_file_name_;
  std::shared_ptr<TFile> out_file_;
  std::string in_calibration_file_name_{"correction_in.root"};

  TTree *out_tree_{nullptr};



  Qn::CorrectionManager manager_;

  Base::AnalysisSetup *analysis_setup_{nullptr};
  std::vector<std::tuple<std::string, std::vector<AxisD>>> qa_histos_;

  std::vector<QnDataPtr> qn_variables_;

  std::vector<MappingContext> track_loop_contexts_;

  std::vector<std::pair<ValueSourceRef, ValueSinkRef>> event_var_mapping_;
  std::vector<std::shared_ptr<ATI2ValueSourceImpl>> ati2_event_sources_;

 TASK_DEF(QnCorrectionTask, 2)
};
}// namespace Qn
#endif//CORRECTION_TASK_H
