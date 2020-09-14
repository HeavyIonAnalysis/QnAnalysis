#include <TFileCollection.h>
#include "TTreeReaderValue.h"

#include <QnTools/QnDataFrame.hpp>

#include "CorrelationTask.h"
#include "Utils.h"
#include "QVector.h"
#include "AnalysisSetup.h"

constexpr auto obs = Qn::Stats::Weights::OBSERVABLE;
constexpr auto ref = Qn::Stats::Weights::REFERENCE;

CorrelationTask::CorrelationTask(const std::string& file, const std::string& treename) :
  in_file_(TFile::Open(file.c_str(), "READ")),
  config_(in_file_->Get<Flow::Base::AnalysisSetup>("Config")),
  in_tree_(in_file_->Get<TTree>(treename.c_str())),
  reader_(TTreeReader(in_tree_)),
  df_(ROOT::RDataFrame(*in_tree_)),
  dfs_(Qn::Correlation::Resample(df_, config_->GetNSamples())),
  event_(config_->GetCorrectionAxes().at(0)) // NOTE
{}

void CorrelationTask::FillCorrelations() {
  using std::vector;
  using std::string;

  // Q1 * Psi_RP
  if (config_->IsSimulation()) {
    for (const auto& qn : config_->GetQvectorsConfig()) {
      AddQ1Q1Correlations(qn, config_->GetPsiQvector(), u1Q1_EVENT_PLANE); // <Qa, Qb> / |Qb|, because the magnitude of PsiRp-vector must be 1
    }
    for (const auto& qn : config_->GetChannelConfig()) {
      AddQ1Q1Correlations(qn, config_->GetPsiQvector(), u1Q1_EVENT_PLANE);  // <Qa, Qb> / |Qb|, because the magnitude of PsiRp-vector must be 1
    }
  }
//  // Q1_channel * Q1_track
//  for (const auto& qn_ch : config_->GetChannelConfig()) {
//    for (const auto& qn_tr : config_->GetQvectorsConfig()) {
//      AddQ1Q1Correlations(qn_tr, qn_ch);
//    }
//  }
//  // Q1_channel * Q1_channel
//  for (const auto& qn_ch1 : config_->GetChannelConfig()) {
//    for (const auto& qn_ch2 : config_->GetChannelConfig()) {
//      if( qn_ch1.GetName() != qn_ch2.GetName() )
//      AddQ1Q1Correlations(qn_ch1, qn_ch2);
//    }
//  }
//  // Q2_track * Q1_channel * Q1_channel
//  for (const auto& qn_tr : config_->GetQvectorsConfig()) {
//    for (const auto& qn_ch1 : config_->GetChannelConfig()) {
//      for (const auto& qn_ch2 : config_->GetChannelConfig()) {
//        if(qn_ch1.GetName() != qn_ch2.GetName())
//          AddQ2Q1Q1Correlations(qn_tr, qn_ch1, qn_ch2);
//      }
//    }
//  }
}

void CorrelationTask::Run() {
  std::cout << "CorrelationManager::Run()..." << std::endl;

  FillCorrelations();

  auto* out_file = new TFile("correlation_out.root", "RECREATE");
  out_file->cd();

  std::vector<Qn::DataContainerStats> stats;
  for (auto &correlation : correlations_) {
    stats.push_back(correlation.second.GetValue().GetDataContainer());
  }

  for (auto &correlation : stats) {
    correlation.Write();
  }

  for (auto &correlation : correlations_) {
    correlation.second->Write();
  }
  out_file->Close();

  std::cout << "Done." << std::endl;
}

void CorrelationTask::AddQ1Q1Correlations(const Flow::Base::QVector& a, const Flow::Base::QVector& b, int type){
  using function2_t = const std::function<double(const Qn::QVector &a, const Qn::QVector &b)>;
  std::string a_name = a.GetName() + "_" + a.GetLastStepName();
  std::string b_name = b.GetName() + "_" + b.GetLastStepName();
  static std::vector<std::pair<std::string, function2_t>> corr_func;
  switch (type) {
    case SCALAR_PRODUCT:
      corr_func = Qn::Q1Q1(non_zero_only_);
      break;
    case Q1Q1_EVENT_PLANE:
      corr_func = Qn::Q1Q1EP(non_zero_only_);
      break;
    case u1Q1_EVENT_PLANE:
      corr_func = Qn::u1Q1EP(non_zero_only_);
      break;
    default:
      std::cerr << "CorrelationTask::AddQ1Q1Correlations: Unknown type of correlations" << std::endl;
      abort();
  }

  for (const auto& cor : corr_func) {
    if(verbosity_ >= 1){
      std::cout << "Adding correlation: " << a_name << " " << b_name << " " << cor.first << std::endl;
    }
    const auto name = a_name + "_" + b_name + "_" + cor.first;
    auto q11 = Qn::EventAverage(Qn::Correlation::Correlation(
      name,
      cor.second,
      {a_name, b_name},
      {a.GetWeightsType(), b.GetWeightsType()},
      Qn::EventAxes(event_),
      config_->GetNSamples()))
      .BookMe(dfs_);

    correlations_.emplace(name, q11);
  }
}

void CorrelationTask::AddQ2Q1Q1Correlations(const Flow::Base::QVectorTrack& t, const Flow::Base::QVector& a, const Flow::Base::QVector& b) {
  std::string t_name = t.GetName() + "_" + t.GetLastStepName();
  std::string a_name = a.GetName() + "_" + a.GetLastStepName();
  std::string b_name = b.GetName() + "_" + b.GetLastStepName();
  for (const auto& cor : Qn::Q2Q1Q1(non_zero_only_)) {
    if(verbosity_ >= 1){
      std::cout << "Adding correlation: " << t_name << " " << a_name << " " << b_name << " " << cor.first << std::endl;
    }

    const auto name = t_name + "_" + a_name + "_" + b_name + "_" + cor.first;
    auto q2q1q1 = Qn::EventAverage(Qn::Correlation::Correlation(
      name,
      cor.second,
      {t_name, a_name, b_name},
      {t.GetWeightsType(), a.GetWeightsType(), b.GetWeightsType()},
      Qn::EventAxes(event_),
      config_->GetNSamples()))
      .BookMe(dfs_);

    correlations_.emplace(name, q2q1q1);
  }
}
