#ifndef DATATREEFLOW_GLOBALCONFIG_H
#define DATATREEFLOW_GLOBALCONFIG_H

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "TError.h"
#include "TString.h"

#include <QnTools/Axis.hpp>

#include <QnAnalysisBase/QVector.hpp>

namespace Qn::Analysis::Base {

struct AnalysisSetupConfig : public TObject {

  std::vector<QVectorConfig> q_vectors;
  std::vector<AxisConfig> event_axes;
  std::vector<VariableConfig> event_variables;

  ClassDef(Qn::Analysis::Base::AnalysisSetupConfig, 2);
};

using QVectorPtr = std::shared_ptr<QVector>;

struct AnalysisSetup {

 public:
  AnalysisSetup() = default;// Needed to read from file

  /**
   * @brief Adds Q-vector to analysis
   * @param qv
   */
  void AddQVector(QVector* qv) {
    auto new_qv = QVectorPtr(qv);
    q_vectors.emplace_back(new_qv);
    if (qv->GetType() == EQVectorType::TRACK) {
      track_qvectors_.emplace_back(dynamic_cast<QVectorTrack*>(new_qv.get()));
    } else if (qv->GetType() == EQVectorType::CHANNEL) {
      channel_qvectors_.emplace_back(dynamic_cast<QVectorChannel*>(new_qv.get()));
    } else if (qv->GetType() == EQVectorType::EVENT_PSI) {
      psi_qvectors_.emplace_back(dynamic_cast<QVectorPsi*>(new_qv.get()));
    }
  }

  template<typename T>
  void AddQVector(T&& qv) {
    AddQVector(new T(qv));
  }

  void AddCorrectionAxis(const Qn::AxisD& axis) { correction_axes_.emplace_back(axis); }

  const std::vector<Qn::AxisD>& GetCorrectionAxes() const { return correction_axes_; }
  const std::vector<std::string>& GetCorrelationNames() const { return correlation_names_; }

  static std::string ConstructCorrelationName(const std::vector<std::string>& q_vectors, const std::string& type) {
    std::string name;
    for (const auto& qn : q_vectors) {
      name += qn + "_";
    }
    name += type;
    return name;
  }
  const std::vector<AnalysisTree::Variable>& GetEventVars() const { return event_vars_; }
  std::vector<AnalysisTree::Variable>& EventVars() { return event_vars_; }
  void AddEventVar(const AnalysisTree::Variable& var) {
    event_vars_.emplace_back(var);
  }

  [[maybe_unused]] void Print() const;

  std::vector<std::shared_ptr<QVector>> q_vectors;
  std::vector<QVectorPsi*> psi_qvectors_{};
  std::vector<QVectorTrack*> track_qvectors_{};
  std::vector<QVectorChannel*> channel_qvectors_{};

  // Correction parameters
  std::vector<Qn::AxisD> correction_axes_{};// fixme it should be Qn::Analysis::Base::Axis

  std::vector<AnalysisTree::Variable> event_vars_{};
  // Correlation parameters
  std::vector<std::string> correlation_names_{};
};

namespace Utils {

AnalysisSetup Convert(const AnalysisSetupConfig& config);

}

}// namespace Qn::Analysis::Base

#endif//DATATREEFLOW_GLOBALCONFIG_H
