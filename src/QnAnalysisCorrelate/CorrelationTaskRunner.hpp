//
// Created by eugene on 21/07/2020.
//

#ifndef DATATREEFLOW_SRC_CORRELATION_CORRELATIONTASK_H
#define DATATREEFLOW_SRC_CORRELATION_CORRELATIONTASK_H

#include <algorithm>
#include <utility>
#include <vector>
#include <string>
#include <filesystem>

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

  struct file_not_found_exception : public std::exception {
    file_not_found_exception() = default;
    file_not_found_exception(const std::exception&e) : std::exception(e) {};
  };

public:


  boost::program_options::options_description GetBoostOptions();


  void Initialize();
  /**
   * @brief Generates list of correlations of size
   * N(arg1)*N(arg2)*...N(argN)*N(actions)
   * @param task
   */
  void LoadTasks();

  void Run();

private:
  void LookupConfiguration();
  bool LoadConfiguration(const std::filesystem::path& path);


  std::filesystem::path configuration_file_path_{};
  std::string configuration_node_name_{};

  std::vector<CorrelationTask> tasks_;







};

}

#endif  // DATATREEFLOW_SRC_CORRELATION_CORRELATIONTASK_H
