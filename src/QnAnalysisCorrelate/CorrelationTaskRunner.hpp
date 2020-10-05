//
// Created by eugene on 21/07/2020.
//

#ifndef DATATREEFLOW_SRC_CORRELATION_CORRELATIONTASK_H
#define DATATREEFLOW_SRC_CORRELATION_CORRELATIONTASK_H

#include <algorithm>
#include <utility>
#include <vector>
#include <string>

#include <QnDataFrame.hpp>
#include <TFile.h>
#include <TTree.h>
#include <boost/program_options.hpp>

#include <yaml-cpp/yaml.h>


#include "Config.hpp"


namespace Qn::Analysis::Correlate {

class CorrelationTaskRunner {

  struct ActionArg {

  };

  struct Action {
    std::string action_name;
  };

  struct Correlation {
    std::vector<ActionArg> action_args;
    Action action;
  };

public:


  boost::program_options::options_description GetBoostOptions();


  void Initialize();
  /**
   * @brief Generates list of correlations of size
   * N(arg1)*N(arg2)*...N(argN)*N(actions)
   * @param task
   */
  void LoadCorrelationTask(const CorrelationTask& task) {
  }

  void Run();

private:
  std::string configuration_file_name_{};
  std::string configuration_node_name_{};









};

}

#endif  // DATATREEFLOW_SRC_CORRELATION_CORRELATIONTASK_H
