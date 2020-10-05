//
// Created by eugene on 21/07/2020.
//

#include <filesystem>

#include <yaml-cpp/yaml.h>

#include <BuildOptions.hpp>
#include "CorrelationTaskRunner.hpp"

using std::filesystem::path;
using std::filesystem::current_path;
using namespace boost::program_options;

boost::program_options::options_description Qn::Analysis::Correlate::CorrelationTaskRunner::GetBoostOptions() {
  options_description desc("Correlation options");
  desc.add_options()
      ("configuration-file", value(&configuration_file_path_)->required(), "Path to the YAML configuration")
      ("configuration-name", value(&configuration_node_name_)->required(), "Name of YAML node");

  return desc;
}

void Qn::Analysis::Correlate::CorrelationTaskRunner::Initialize() {
  LookupConfiguration();
}
void Qn::Analysis::Correlate::CorrelationTaskRunner::Run() {

}


void Qn::Analysis::Correlate::CorrelationTaskRunner::LookupConfiguration() {
  if (configuration_file_path_.is_absolute()) {
    if (!LoadConfiguration(configuration_file_path_)) {
      throw std::runtime_error("Configuration file was not found");
    }
  } else {
    for (const auto& lookup_dir : {current_path(), path(::Qn::Analysis::GetSetupsDir())}) {
      auto lookup_path = lookup_dir / configuration_file_path_;
      Info(__func__, "Looking for configuration in '%s'", lookup_path.c_str());
      if (LoadConfiguration(lookup_path)) {
        break;
      }
    }
  }



}
bool Qn::Analysis::Correlate::CorrelationTaskRunner::LoadConfiguration(const std::filesystem::path& path) {
  using namespace YAML;

  Node top_node;
  try {
    top_node = LoadFile(path.string());
  } catch (std::exception&e) {
    Error(__func__, "File %s not found", path.c_str());
    return false;
  }

  Info(__func__, "Loaded %s...", path.c_str());

  tasks_ = top_node[configuration_node_name_].as<std::vector<CorrelationTask>>();

  return true;
}
