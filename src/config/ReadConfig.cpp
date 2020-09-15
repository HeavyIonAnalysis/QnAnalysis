//
// Created by eugene on 13/08/2020.
//

#include "Config.hpp"
#include "Convert.hpp"
#include <base/AnalysisSetup.hpp>

#include <yaml-cpp/yaml.h>

int main() {

  using namespace Qn::Analysis::Base;
  auto node = YAML::LoadFile("analysis-config.yml");
  auto setup_config = node["test"].as<AnalysisSetupConfig>();
  auto setup = Qn::Analysis::Config::Utils::Convert(setup_config);
  for (auto& qv : setup.GetQvectorsConfig()) {
    qv.Print();
  }
  return 0;
}
