#ifndef QN_QVECTOR_CONFIG_H
#define QN_QVECTOR_CONFIG_H

#include <algorithm>
#include <bitset>
#include <cstdlib>
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <TObject.h>
#include "TTreeReader.h"

#include <QnTools/Axis.hpp>
#include <QnTools/QVector.hpp>
#include <QnTools/CorrectionOnQnVector.hpp>
#include <QnTools/Stat.hpp>

#include <AnalysisTree/Constants.hpp>
#include <AnalysisTree/Variable.hpp>

#include <QnAnalysisBase/Axis.hpp>
#include <QnAnalysisBase/Cut.hpp>
#include <QnAnalysisBase/Histogram.hpp>
#include <QnAnalysisBase/Variable.hpp>

/* forward declarations */
namespace Qn::Analysis::Base {

struct QVectorConfig;
class QVector;
class QVectorChannel;
class QVectorTrack;

}// namespace Qn::Analysis::Base

namespace Qn::Analysis::Base {

enum class EQVectorCorrectionType {
  NONE,
  RECENTERING,
  TWIST_AND_RESCALE,
};

enum class ETwistRescaleMethod {
  DOUBLE_HARMONIC,
  CORRELATIONS
};

struct QVectorCorrectionConfig {
  EQVectorCorrectionType type{EQVectorCorrectionType::NONE};
  int no_of_entries{0};

  /* recentering options */
  bool recentering_width_equalization{false};

  /* twist-rescale options */
  ETwistRescaleMethod twist_rescale_method{ETwistRescaleMethod::DOUBLE_HARMONIC};
  bool twist_rescale_apply_twist{true};
  bool twist_rescale_apply_rescale{true};

  static QVectorCorrectionConfig RecenteringDefault() {
    QVectorCorrectionConfig result;
    result.type = EQVectorCorrectionType::RECENTERING;
    result.no_of_entries = 0;
    result.recentering_width_equalization = false;
    return result;
  }

  static QVectorCorrectionConfig TwistAndRescaleDefault() {
    QVectorCorrectionConfig result;
    result.type = EQVectorCorrectionType::TWIST_AND_RESCALE;
    result.no_of_entries = 0;
    result.twist_rescale_method = ETwistRescaleMethod::DOUBLE_HARMONIC;
    result.twist_rescale_apply_rescale = true;
    result.twist_rescale_apply_twist = true;
    return result;
  }
};

enum class EQVectorType {
  EVENT_PSI,
  TRACK,
  CHANNEL
};

struct QVectorConfig : public TObject {
  std::string name;
  EQVectorType type{EQVectorType::TRACK};

  VariableConfig phi;
  VariableConfig weight;

  std::vector<HistogramConfig> qa;

  /* fields specific for track */
  std::vector<AxisConfig> axes;
  std::vector<CutConfig> cuts;
  std::vector<QVectorCorrectionConfig> corrections;

  /* fields specific for channel detector */
  std::vector<int> channel_ids;

  std::string harmonics;

  ClassDef(Qn::Analysis::Base::QVectorConfig, 2)
};

class QVector {

 public:
  typedef std::shared_ptr<Qn::CorrectionOnQnVector> CorrectionPtr;

  enum eCorrSteps {
    kRecentering = 0,
    kTwist,
    kRescale,
    nSteps
  };

  explicit QVector(EQVectorType Type) : type_(Type) {}
  QVector(std::string name,
          EQVectorType type,
          AnalysisTree::Variable phi,
          AnalysisTree::Variable weight) : name_(std::move(name)),
                                           type_(type),
                                           phi_(std::move(phi)),
                                           weight_(std::move(weight)) {
  }
  virtual ~QVector() = default;

  std::string GetName() const { return name_; }
  EQVectorType GetType() const {
    return type_;
  }
  void SetName(const std::string& name) { name_ = name; }
  const AnalysisTree::Variable& GetWeightVar() const { return weight_; }

  const AnalysisTree::Variable& GetPhiVar() const { return phi_; }
  AnalysisTree::Variable& WeightVar() { return weight_; }
  AnalysisTree::Variable& PhiVar() { return phi_; }
  void SetWeightsType(Qn::Stat::WeightType type) { weights_type_ = type; }

  /**
   * @brief Adds correction to Q-vector, QVector owns the pointer
   * @param correction
   */
  void AddCorrection(Qn::CorrectionOnQnVector* correction) {
    corrections_.emplace_back(correction);
  }
  template<typename T>
  void AddCorrection(T&& correction) {
    AddCorrection(new T(correction));
  }

  const std::list<CorrectionPtr>& GetCorrections() const {
    return corrections_;
  }

  void SetCorrectionSteps(bool rec, bool twist, bool res) {
    corrertions_[kRecentering] = rec;
    corrertions_[kTwist] = twist;
    corrertions_[kRescale] = res;
  }

  std::bitset<8> GetHarmonics() const {
    return harmonics_;
  }
  void SetHarmonics(const std::bitset<8>& Harmonics) {
    harmonics_ = Harmonics;
  }

  Qn::Stat::WeightType GetWeightsType() const { return weights_type_; }

  virtual std::vector<AnalysisTree::Variable> GetListOfVariables() const { return {phi_, weight_}; }

  int GetVarEntryId() const { return var_entry_id_; }

  void SetVarEntryId(int var_entry_id) { var_entry_id_ = var_entry_id; }

  /* QA Histograms */
  void AddQAHistogram(const Axis& axis) {
    Histogram h;
    h.axes.emplace_back(axis);
    qa_histograms_.emplace_back(std::move(h));
  }
  void AddQAHistogram(const Axis& ax1, const Axis& ax2) {
    Histogram h;
    h.axes.emplace_back(ax1);
    h.axes.emplace_back(ax2);
    qa_histograms_.emplace_back(std::move(h));
  }
  void AddQAHistogram(const Histogram& h) {
    qa_histograms_.emplace_back(h);
  }
  auto GetQAHistograms() const {
    return qa_histograms_;
  }

  virtual void Print() const {
    std::cout << "Q-vector " << name_ << std::endl;
    std::cout << "Phi variable: " << std::endl;
    phi_.Print();
    std::cout << "Weight variable: " << std::endl;
    weight_.Print();
  }

  std::string GetLastStepName() const {
    if (corrertions_[kRescale]) {
      return "RESCALED";
    } else if (corrertions_[kTwist]) {
      return "TWIST";
    } else if (corrertions_[kRecentering]) {
      return "RECENTERED";
    }
    return "PLAIN";
  }

 protected:
  std::string name_;///<  Name of the Q-vector
  EQVectorType type_;
  AnalysisTree::Variable phi_{};
  AnalysisTree::Variable weight_{};
  std::array<bool, nSteps> corrertions_{false, false, false};
  std::list<CorrectionPtr> corrections_;
  Qn::Stat::WeightType weights_type_ = Qn::Stat::WeightType::OBSERVABLE;

  std::bitset<8> harmonics_;

  int var_entry_id_{};
  std::list<Histogram> qa_histograms_;
};

class QVectorChannel : public QVector {
 public:
  QVectorChannel() : QVector(EQVectorType::CHANNEL) {}
  QVectorChannel(const std::string& name, const AnalysisTree::Variable& phi, const AnalysisTree::Variable& weight,
                 std::vector<int> ids) : QVector(name, EQVectorType::CHANNEL, phi, weight),
                                         module_ids_(std::move(ids)) {
    phi_.SetSize(module_ids_.size());
    weight_.SetSize(module_ids_.size());
  }

  size_t GetNumberOfModules() const { return module_ids_.size(); }
  const std::vector<int>& GetModuleIds() const { return module_ids_; }

 private:
  std::vector<int> module_ids_{};
  short branch_id{-1};
};

class QVectorTrack : public QVector {

 public:
  QVectorTrack() : QVector(EQVectorType::TRACK) {}
  QVectorTrack(std::string name, AnalysisTree::Variable phi, AnalysisTree::Variable weight,
               std::vector<Axis> axes) : QVector(std::move(name), EQVectorType::TRACK, std::move(phi), std::move(weight)),
                                         axes_(std::move(axes)) {
    auto var = AnalysisTree::Variable(*phi_.GetBranches().begin(), "Filled");
    this->AddCut({var, [](const double is) { return is > 0.; }, "is_filled"});
  }

  std::vector<Qn::AxisD> GetAxes() const;

  void AddCut(const Cut& cut) {
    cuts_.emplace_back(cut);
  }

  std::vector<AnalysisTree::Variable> GetListOfVariables() const override;
  const std::vector<Cut>& GetCuts() const { return cuts_; };

 protected:
  static bool EndsWith(std::string const& name, std::string const& ending) {

    if (name.length() >= ending.length()) {
      auto substr = name.substr(name.length() - ending.length(), name.length());
      if (substr == ending) return true;
    }
    return false;
  }

  std::vector<Axis> axes_{};
  std::vector<Cut> cuts_{};
};

class QVectorPsi : public QVector {
 public:
  QVectorPsi() : QVector(EQVectorType::EVENT_PSI) {}
  QVectorPsi(const std::string& Name,
             const AnalysisTree::Variable& Phi,
             const AnalysisTree::Variable& Weight) : QVector(Name, EQVectorType::EVENT_PSI, Phi, Weight){};
};

}// namespace Qn::Analysis::Base

#endif//QN_QVECTOR_CONFIG_H
