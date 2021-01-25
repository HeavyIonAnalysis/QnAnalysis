//
// Created by mikhail on 10/23/20.
//

#include "ConvertConfigs.hpp"
#include "FileManager.hpp"
#include "V1Observables.hpp"
#include <boost/program_options.hpp>
#include <yaml-cpp/yaml.h>

int main(int argv, char** argc){
  namespace po = boost::program_options;
  std::string input_file;
  std::string output_file_name{"output.root"};
  std::string config_file;
  po::options_description options("Options");
  options.add_options()
      ("help,h", "Help screen")
      ("MC,m", "MC data sample")
      ("input,i", po::value<std::string>(&input_file), "Input file list")
      ("output,o", po::value<std::string>(&output_file_name), "Name of output file")
      ("config,c", po::value<std::string>(&config_file),"path to yaml-config file");
  po::variables_map vm;
  po::parsed_options parsed = po::command_line_parser(argv, argc).options(options).run();
  po::store(parsed, vm);
  po::notify(vm);
  if (vm.count("help")){
    std::cout << options << std::endl;
    return 0;
  }
  FileManager::OpenFile( input_file );
  auto config = YAML::LoadFile( config_file );
  auto detectors_configs = config["_detectors"].as<std::vector<VectorConfig>>();
  auto observables_configs = config["_observables"].as<std::vector<VnObservablesConfig>>();
  std::vector<V1Observables> observables;
  observables.reserve(observables_configs.size());

  auto out_file = TFile::Open(output_file_name.c_str(), "recreate" );
  for( const auto &observable_config : observables_configs  ){
    observables.emplace_back( observable_config.method );
    observables.back().SetQqCorrelationsDirectory(observable_config.qq_correlations_directory);
    observables.back().SetUqCorrelationsDirectory(observable_config.uq_correlations_directory);
    observables.back().SetUvector( observable_config.u_vector );
    std::vector<VectorConfig> ep_vectors;
    for( const auto& vector : detectors_configs )
      if( std::count(vector.tags.begin(), vector.tags.end(), observable_config.ep_vectors_tag) > 0 )
        ep_vectors.emplace_back(vector);
    observables.back().SetEPvectors(ep_vectors);
    std::vector<std::string> resolution_vectors;
    for( const auto& vector : detectors_configs )
      if( std::count(vector.tags.begin(), vector.tags.end(), observable_config.resolution_vectors_tag) > 0 )
        resolution_vectors.emplace_back(vector.name+"_"+vector.correction_step);
    observables.back().SetResolutionVectors(resolution_vectors);
    observables.back().Calculate();
    out_file->cd();
    observables.back().Write();
  }
  out_file->Close();
//  V1Observables obs_sp( V1Observables::METHODS::MethodOf3SE );
//  obs_sp.SetUvector("u_RESCALED", {"x1", "y1"});
//  obs_sp.SetEPvectors({"W1_RESCALED", "W2_RESCALED", "W3_RESCALED"}, {"x1", "y1"});
//  obs_sp.SetResolutionVectors({
//      "W1_RESCALED",
//      "W2_RESCALED",
//      "W3_RESCALED",
//      "Mf_protons_RESCALED",
//      "Mb_protons_RESCALED",
//  });
//  obs_sp.SetQqCorrelationsDirectory("/QQ/SP");
//  obs_sp.SetUqCorrelationsDirectory("/uQ/SP");
//  obs_sp.Calculate();
//  V1Observables obs_ep( V1Observables::METHODS::MethodOf3SE );
//  obs_ep.SetUvector("u_RESCALED", {"x1", "y1"});
//  obs_ep.SetEPvectors({"W1_RESCALED", "W2_RESCALED", "W3_RESCALED"}, {"cos1", "sin1"});
//  obs_ep.SetResolutionVectors({
//                                  "W1_RESCALED",
//                                  "W2_RESCALED",
//                                  "W3_RESCALED",
//                                  "Mf_protons_RESCALED",
//                                  "Mb_protons_RESCALED",
//                              });
//  obs_ep.SetQqCorrelationsDirectory("/QQ/EP");
//  obs_ep.SetUqCorrelationsDirectory("/uQ/EP");
//  obs_ep.Calculate();
//  V1Observables obs_rs( V1Observables::METHODS::MethodOfRS );
//  obs_rs.SetUvector("u_RESCALED", {"x1", "y1"});
//  obs_rs.SetEPvectors({"R1_RESCALED", "R2_RESCALED"}, {"x1", "y1"});
//  obs_rs.SetResolutionVectors({"R1_RESCALED", "R2_RESCALED"});
//  obs_rs.SetQqCorrelationsDirectory("/QQ/SP");
//  obs_rs.SetUqCorrelationsDirectory("/uQ/SP");
////  obs_rs.Calculate();
//  auto file_out = TFile::Open("agag-158-szymon.root", "recreate");
//  file_out->mkdir("SP");
//  file_out->cd("/SP");
//  obs_sp.Write();
//  obs_rs.Write();
//  file_out->mkdir("EP");
//  file_out->cd("/EP");
//  obs_ep.Write();
//  file_out->Close();

  return 0;
}