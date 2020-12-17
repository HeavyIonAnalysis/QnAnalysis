#include "AnalysisSetup.hpp"

ClassImp(Qn::Analysis::Base::AnalysisSetupConfig)

    [[maybe_unused]] void Qn::Analysis::Base::AnalysisSetup::Print() const {
  std::cout << " ***** Tracking detectors ***** :" << std::endl;
  for (const auto& det : q_vectors) {
    det->Print();
  }

}
