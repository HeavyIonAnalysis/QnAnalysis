#include "AnalysisSetup.hpp"

ClassImp(Qn::Analysis::Base::AnalysisSetupConfig)

    [[maybe_unused]] void Qn::Analysis::Base::AnalysisSetup::Print() const {
  std::cout << " ***** Tracking detectors ***** :" << std::endl;
  for (const auto& det : track_qvectors_) {
    det.Print();
  }
  std::cout << " ***** Channelized detectors ***** :" << std::endl;
  for (const auto& det : channel_qvectors_) {
    det.Print();
  }
  if (is_simulation_) {
    std::cout << " ***** Psi_RP ***** :" << std::endl;
    psi_qvector_.Print();
  }
}
