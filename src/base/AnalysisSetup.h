#ifndef DATATREEFLOW_GLOBALCONFIG_H
#define DATATREEFLOW_GLOBALCONFIG_H

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "TError.h"
#include "TString.h"

#include <QnTools/Axis.hpp>

#include "QVector.h"

namespace Qn::Analysis::Base {

struct AnalysisSetupConfig : public TObject {

  std::vector<QVectorConfig> q_vectors;
  std::vector<AxisConfig> event_axes;
  std::vector<VariableConfig> event_variables;

  ClassDef(Qn::Analysis::Base::AnalysisSetupConfig, 2);
};

struct AnalysisSetup {

 public:
  AnalysisSetup() = default;// Needed to read from file

  /**
   * @brief Adds Q-vector to analysis
   * @param qv
   */
  void AddQVector(QVector* qv) {
    if (qv->GetType() == EQVectorType::TRACK) {
      track_qvectors_.emplace_back(*dynamic_cast<const QVectorTrack*>(qv));
    } else if (qv->GetType() == EQVectorType::CHANNEL) {
      channel_qvectors_.emplace_back(*dynamic_cast<const QVectorChannel*>(qv));
    } else if (qv->GetType() == EQVectorType::EVENT_PSI) {
      /* todo ignore at this moment */
    }
    q_vectors.emplace_back(qv);
  }

  template<typename T>
  void AddQVector(T&& qv) {
    AddQVector(new T(qv));
  }

  [[deprecated]] void AddTrackQvector(const QVectorTrack& conf) { track_qvectors_.emplace_back(conf); }
  [[deprecated]] void AddChannelQvector(const QVectorChannel& conf) { channel_qvectors_.emplace_back(conf); }
  void SetPsiQvector(const QVectorPsi& conf) {
    is_simulation_ = true;
    psi_qvector_ = conf;
  }
  void AddCorrectionAxis(const Qn::AxisD& axis) { correction_axes_.emplace_back(axis); }

  void SetNSamples(int n_samples) { n_samples_ = n_samples; }

  const std::vector<QVectorTrack>& GetQvectorsConfig() const { return track_qvectors_; }
  const std::vector<QVectorChannel>& GetChannelConfig() const { return channel_qvectors_; }
  const QVector& GetQvectorConfig(const std::string& name) {
    for (const auto& vector : track_qvectors_) {
      if (vector.GetName() == name)
        return vector;
    }
    for (const auto& vector : channel_qvectors_) {
      if (vector.GetName() == name)
        return vector;
    }
    std::cout << "Error in GlobalConfig::GetQvectorConfig" << std::endl;
    std::cout << "No such Qvector named " << name << std::endl;
    abort();
  }
  const QVector& GetPsiQvector() const { return psi_qvector_; }
  QVector& PsiQvector() { return psi_qvector_; }
  const std::vector<Qn::AxisD>& GetCorrectionAxes() const { return correction_axes_; }

  bool IsSimulation() const { return is_simulation_; }
  int GetNSamples() const { return n_samples_; }

  std::vector<QVectorTrack>& QvectorsConfig() { return track_qvectors_; }
  std::vector<QVectorChannel>& ChannelConfig() { return channel_qvectors_; }

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

  // Correction parameters
  std::vector<QVectorTrack> track_qvectors_{};
  std::vector<QVectorChannel> channel_qvectors_{};
  QVectorPsi psi_qvector_;

  std::vector<Qn::AxisD> correction_axes_{};// fixme it should be Qn::Analysis::Base::Axis
  std::vector<AnalysisTree::Variable> event_vars_{};

  std::vector<std::shared_ptr<QVector>> q_vectors;
  // Correlation parameters
  int n_samples_{50};
  std::vector<std::string> correlation_names_{};

  // Global parameters
  bool is_simulation_{false};
};

namespace Utils {

AnalysisSetup Convert(const AnalysisSetupConfig& config);

}

}// namespace Qn::Analysis::Base

#endif//DATATREEFLOW_GLOBALCONFIG_H
