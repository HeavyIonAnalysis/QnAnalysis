//
// Created by eugene on 21/07/2020.
//

#include <filesystem>

#include <yaml-cpp/yaml.h>

#include <BuildOptions.hpp>
#include "CorrelationTaskRunner.hpp"
#include "Utils.hpp"

using std::filesystem::path;
using std::filesystem::current_path;
using namespace boost::program_options;

using namespace Qn::Analysis::Correlate;

boost::program_options::options_description Qn::Analysis::Correlate::CorrelationTaskRunner::GetBoostOptions() {
  options_description desc("Correlation options");
  desc.add_options()
      ("configuration-file", value(&configuration_file_path_)->required(), "Path to the YAML configuration")
      ("configuration-name", value(&configuration_node_name_)->required(), "Name of YAML node");

  return desc;
}

void Qn::Analysis::Correlate::CorrelationTaskRunner::Initialize() {
  LookupConfiguration();
  LoadTasks();
}
void Qn::Analysis::Correlate::CorrelationTaskRunner::Run() {

}

void Qn::Analysis::Correlate::CorrelationTaskRunner::LookupConfiguration() {
  if (configuration_file_path_.is_absolute()) {
    LoadConfiguration(configuration_file_path_);
  } else {
    for (const auto &lookup_dir : {current_path(), path(::Qn::Analysis::GetSetupsDir())}) {
      auto lookup_path = lookup_dir / configuration_file_path_;
      Info(__func__, "Looking for configuration in '%s'", lookup_path.c_str());
      try {
        LoadConfiguration(lookup_path);
        break;
      } catch (file_not_found_exception&e) {
        Warning(__func__ , "File not found.");
      }
    }
  }

}
bool Qn::Analysis::Correlate::CorrelationTaskRunner::LoadConfiguration(const std::filesystem::path &path) {
  using namespace YAML;

  Node top_node;
  try {
    top_node = LoadFile(path.string());
  } catch (std::exception &e) {
    throw file_not_found_exception(e);
  }

  Info(__func__, "Loaded %s...", path.c_str());

  tasks_ = top_node[configuration_node_name_].as<std::vector<CorrelationTask>>();
}

void Qn::Analysis::Correlate::CorrelationTaskRunner::LoadTasks() {

  using QVectorList = std::vector<QVectorTagged>;

  for (auto &task : tasks_) {

    std::vector<QVectorList> argument_lists_to_combine;
    argument_lists_to_combine.reserve(task.arguments.size());
    for (auto &arg_list : task.arguments) {
      argument_lists_to_combine.emplace_back(arg_list.query_result);
    }
    std::vector<QVectorList> arguments_combined;
    Utils::CombineDynamic(argument_lists_to_combine.begin(),
                          argument_lists_to_combine.end(),
                          std::back_inserter(arguments_combined));
    /* now we combine them with actions */
    std::vector<std::tuple<QVectorList, std::string>> arguments_actions_combined;
    Utils::Combine(std::back_inserter(arguments_actions_combined), arguments_combined, task.actions);
    std::cout << std::endl;



  }

}

