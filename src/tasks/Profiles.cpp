#include <analyze/ResourceManager.h>
#include <string>
int main(int argc, char* argv[]) {

  std::string inputfile = "/home/vklochkov/Data/cbm/oct19_fr_18.2.1_fs_jun19p1/dcmqgsm_smm_pluto/auau/12agev/mbias/psd44_hole20_pipe0/TGeant4/flow_psdmc/analysis.root";
  std::string outputfile = "/home/vklochkov/Data/cbm/oct19_fr_18.2.1_fs_jun19p1/dcmqgsm_smm_pluto/auau/12agev/mbias/psd44_hole20_pipe0/TGeant4/flow_psdmc/profiles.root";

  //  {0., 5, 10, 20, 30, 50, 70, 100.}
  Qn::Axis centralityAxis("Centrality", {0, 10, 30, 50, 100});

  ResourceManager rm;
  rm.LoadFile(inputfile);
  rm.ForMatchingExec(".*", ProfileExporter(outputfile).Folder("raw"));
  rm.ForMatchingExec("RES_.*", ProfileExporter(outputfile).Folder("resolution").CorrelatedErrors());
  rm.ForMatchingExec("v1_.*", ProfileExporter(outputfile).Folder("v1").Rebin(centralityAxis).CorrelatedErrors().Unfold());
  // v1 with the original binning for dV1/dy vs Centrality plots
  rm.ForMatchingExec("v1_.*", ProfileExporter(outputfile).Folder("v1_origbin").CorrelatedErrors().Unfold());
  // Combined v1
  rm.ForMatchingExec("v1_.*(CC|CR|CA)", ProfileExporter(outputfile).Folder("v1_combined").Rebin(centralityAxis).CorrelatedErrors().Unfold());
  // Combined v1
  // v1 with the original binning for dV1/dy vs Centrality plots
  rm.ForMatchingExec("v1_.*(CC|CR|CA)", ProfileExporter(outputfile).Folder("v1_combined_origbin").CorrelatedErrors().Unfold());
  rm.ForMatchingExec("v1_.*(CC|CR|CA)", ProfileExporter(outputfile).Folder("v1_combined_15_35").Rebin({"Centrality", {10, 30}}).CorrelatedErrors().Unfold());

  rm.ForMatchingExec("v2_.*", ProfileExporter(outputfile).Folder("v2").Rebin(centralityAxis).CorrelatedErrors().Unfold());
  rm.ForMatchingExec("v2_.*(CC|CR|CA)", ProfileExporter(outputfile).Folder("v2_combined").Rebin(centralityAxis).CorrelatedErrors().Unfold());
  rm.ForMatchingExec("v2_.*(CC|CR|CA)", ProfileExporter(outputfile).Folder("v2_combined_15_35").Rebin({"Centrality", {10, 30}}).CorrelatedErrors().Unfold());
  return 0;
}
