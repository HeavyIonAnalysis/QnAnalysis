//
// Created by eugene on 13/08/2020.
//
#include "Config.hpp"
#include "Convert.hpp"

#include <yaml-cpp/yaml.h>

Qn::Analysis::Base::AnalysisSetup Qn::Analysis::Config::ReadSetupFromFile(const std::string& filename, const std::string& config_name) {
  auto node = YAML::Utils::ExpandInheritance(::YAML::LoadFile(filename));
  std::cout << node << std::endl;

  auto analysis_setup_config = node[config_name].as<Base::AnalysisSetupConfig>();
  return Config::Utils::Convert(analysis_setup_config);
}
