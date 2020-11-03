//
// Created by mikhail on 10/23/20.
//

#include "FileManager.hpp"
#include "MethodOf3SE.hpp"
#include "V1Observables.hpp"

int main(){
  FileManager::OpenFile( "~/Correlations/new_qn_analysis.root" );
  MethodOf3SE test_x = MethodOf3SE({"u_RESCALED", "x1"},
                                   {{"W1_RESCALED","x1"}},
                                 {
                      {"W2_RESCALED"}, {"W3_RESCALED"},
                      {"Mf_RESCALED"}, {"Mb_RESCALED"}
//3
                                 } );
  V1Observables obs_sp( V1Observables::METHODS::MethodOf3SE );
  obs_sp.SetUvectors("u_RESCALED", {"x1", "y1"});
  obs_sp.SetEPvectors({"W1_RESCALED", "W2_RESCALED", "W3_RESCALED"}, {"x1", "y1"});
  obs_sp.SetResolutionVectors({"W1_RESCALED", "W2_RESCALED", "W3_RESCALED", "Mf_RESCALED", "Mb_RESCALED"});
  obs_sp.SetQqCorrelationsDirectory("/QQ/SP");
  obs_sp.SetUqCorrelationsDirectory("/uQ/SP");
  obs_sp.Calculate();
  V1Observables obs_ep( V1Observables::METHODS::MethodOf3SE );
  obs_ep.SetUvectors("u_RESCALED", {"x1", "y1"});
  obs_ep.SetEPvectors({"W1_RESCALED", "W2_RESCALED", "W3_RESCALED"}, {"cos1", "sin1"});
  obs_ep.SetResolutionVectors({"W1_RESCALED", "W2_RESCALED", "W3_RESCALED", "Mf_RESCALED", "Mb_RESCALED"});
  obs_ep.SetQqCorrelationsDirectory("/QQ/EP");
  obs_ep.SetUqCorrelationsDirectory("/uQ/EP");
  obs_ep.Calculate();
  auto file_out = TFile::Open("out.root", "recreate");
  file_out->mkdir("SP");
  file_out->cd("/SP");
  obs_sp.Write();
  file_out->mkdir("EP");
  file_out->cd("/EP");
  obs_ep.Write();
  file_out->Close();

  return 0;
}