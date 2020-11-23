//
// Created by mikhail on 10/23/20.
//

#include "FileManager.hpp"
#include "V1Observables.hpp"

int main(){
  FileManager::OpenFile( "~/Correlations/agag158_szymon.root" );

  V1Observables obs_sp( V1Observables::METHODS::MethodOf3SE );
  obs_sp.SetUvectors("u_RESCALED", {"x1", "y1"});
  obs_sp.SetEPvectors({"W1_RESCALED", "W2_RESCALED", "W3_RESCALED"}, {"x1", "y1"});
  obs_sp.SetResolutionVectors({
      "W1_RESCALED",
      "W2_RESCALED",
      "W3_RESCALED",
      "Mf_protons_RESCALED",
      "Mb_protons_RESCALED",
  });
  obs_sp.SetQqCorrelationsDirectory("/QQ/SP");
  obs_sp.SetUqCorrelationsDirectory("/uQ/SP");
  obs_sp.Calculate();
  V1Observables obs_ep( V1Observables::METHODS::MethodOf3SE );
  obs_ep.SetUvectors("u_RESCALED", {"x1", "y1"});
  obs_ep.SetEPvectors({"W1_RESCALED", "W2_RESCALED", "W3_RESCALED"}, {"cos1", "sin1"});
  obs_ep.SetResolutionVectors({
                                  "W1_RESCALED",
                                  "W2_RESCALED",
                                  "W3_RESCALED",
                                  "Mf_protons_RESCALED",
                                  "Mb_protons_RESCALED",
                              });
  obs_ep.SetQqCorrelationsDirectory("/QQ/EP");
  obs_ep.SetUqCorrelationsDirectory("/uQ/EP");
  obs_ep.Calculate();
  V1Observables obs_rs( V1Observables::METHODS::MethodOfRS );
  obs_rs.SetUvectors("u_RESCALED", {"x1", "y1"});
  obs_rs.SetEPvectors({"R1_RESCALED", "R2_RESCALED"}, {"x1", "y1"});
  obs_rs.SetResolutionVectors({"R1_RESCALED", "R2_RESCALED"});
  obs_rs.SetQqCorrelationsDirectory("/QQ/SP");
  obs_rs.SetUqCorrelationsDirectory("/uQ/SP");
  obs_rs.Calculate();
  auto file_out = TFile::Open("agag-158-szymon.root", "recreate");
  file_out->mkdir("SP");
  file_out->cd("/SP");
  obs_sp.Write();
  obs_rs.Write();
  file_out->mkdir("EP");
  file_out->cd("/EP");
  obs_ep.Write();
  file_out->Close();

  return 0;
}