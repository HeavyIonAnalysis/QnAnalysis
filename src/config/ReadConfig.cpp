//
// Created by eugene on 13/08/2020.
//

#include "Config.h"
#include "Convert.h"
#include <base/AnalysisSetup.h>

#include <yaml-cpp/yaml.h>

int main() {

  using namespace Flow::Base;
  auto node = YAML::LoadFile("analysis-config.yml");
  auto setup_config = node["test"].as<AnalysisSetupConfig>();
  auto setup = Flow::Config::Utils::Convert(setup_config);
  for (auto& qv : setup.GetQvectorsConfig()) {
    qv.Print();
  }
  return 0;
}
