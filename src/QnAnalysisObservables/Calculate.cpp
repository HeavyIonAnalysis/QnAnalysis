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
  V1Observables obs( V1Observables::METHODS::MethodOf3SE );
  obs.SetUvectors("u_RESCALED", {"x1", "y1"});
  obs.SetEPvectors({"W1_RESCALED", "W2_RESCALED", "W3_RESCALED"}, {"x1", "y1"});
  obs.SetResolutionVectors({"W1_RESCALED", "W2_RESCALED", "W3_RESCALED", "Mf_RESCALED", "Mb_RESCALED"});
  obs.SetQqCorrelationsDirectory("/QQ/SP");
  obs.SetUqCorrelationsDirectory("/uQ/SP");
  obs.Calculate();
  auto file_out = TFile::Open("out.root", "recreate");
  file_out->cd();
  obs.Write();
  file_out->Close();

  return 0;
}