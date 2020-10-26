//
// Created by mikhail on 10/23/20.
//

#include "FileManager.hpp"
#include "MethodOf3SE.hpp"

int main(){
  FileManager::OpenFile( "~/Correlations/new_qn_analysis.root" );
  MethodOf3SE test_x = MethodOf3SE({"u_RESCALED", "u", "x1"}, {{"W1_RESCALED", "W1","x1"}},
                                 {
                      {"W2_RESCALED", "W2"}, {"W3_RESCALED", "W3"},
                      {"M_RESCALED", "Mb", "", {"event_header_selected_tof_rpc_centrality"},
                                        {{"mdc_vtx_tracks_rapidity", 1, 0.19, 0.39}}},
                                 } );
  test_x.SetQqDirectory("/QQ/SP");
  test_x.SetUqDirectory("/uQ/SP");
  test_x.CalculateObservables();
  auto file_out = TFile::Open("out.root", "recreate");
  file_out->cd();
  test_x.Write();
  file_out->Close();

  return 0;
}