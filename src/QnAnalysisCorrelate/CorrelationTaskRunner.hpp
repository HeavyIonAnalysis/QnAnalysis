//
// Created by eugene on 21/07/2020.
//

#ifndef DATATREEFLOW_SRC_CORRELATION_CORRELATIONTASK_H
#define DATATREEFLOW_SRC_CORRELATION_CORRELATIONTASK_H

#include <algorithm>
#include <utility>

#include <QnDataFrame.hpp>
#include <TFile.h>
#include <TTree.h>
#include <boost/program_options.hpp>

#include <yaml-cpp/yaml.h>


#include "Config.hpp"
#include "TensorIndex.h"


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


  /**
   * @brief Generates list of correlations of size
   * N(arg1)*N(arg2)*...N(argN)*N(actions)
   * @param task
   */
  void LoadCorrelationTask(const CorrelationTask& task) {







  }

private:







};

}

#endif  // DATATREEFLOW_SRC_CORRELATION_CORRELATIONTASK_H
