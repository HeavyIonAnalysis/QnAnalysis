//
// Created by eugene on 21/07/2020.
//

#include <filesystem>

#include <yaml-cpp/yaml.h>

#include <BuildOptions.hpp>
#include "CorrelationTaskRunner.hpp"
#include "Utils.hpp"

#include <QnDataFrame.hpp>
#include <TFileCollection.h>
#include <TChain.h>

using std::filesystem::path;
using std::filesystem::current_path;
using namespace boost::program_options;

using namespace Qn::Analysis::Correlate;

boost::program_options::options_description Qn::Analysis::Correlate::CorrelationTaskRunner::GetBoostOptions() {
  options_description desc("Correlation options");
  desc.add_options()
      ("configuration-file", value(&configuration_file_path_)->required(), "Path to the YAML configuration")
      ("configuration-name", value(&configuration_node_name_)->required(), "Name of YAML node")
      ("input-file,i", value(&input_file_)->required(), "Name of the input ROOT file (or .list file)")
      ("input-tree,i", value(&input_tree_)->default_value("tree"), "Name of the input tree")
      ("output-file,o", value(&output_file_)->required(), "Name of the output ROOT file");

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

  config_tasks_ = top_node[configuration_node_name_].as<std::vector<CorrelationTask>>();
}

void Qn::Analysis::Correlate::CorrelationTaskRunner::LoadTasks() {

  using QVectorList = std::vector<QVectorTagged>;

  for (auto &task : config_tasks_) {

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

    /* init RDataFrame */
    auto df = GetRDF();
    auto df_sampled = Qn::Correlation::Resample(*df, task.n_samples);
    /* Qn::MakeAxes() */
    std::cout << std::endl;





  }

}
std::unique_ptr<ROOT::RDataFrame> CorrelationTaskRunner::GetRDF() {
  if (".list" == input_file_.extension()) {
    TFileCollection fc("fc", "", input_file_.c_str());
    TChain chain(input_tree_.c_str(), "");
    chain.AddFileInfoList(reinterpret_cast<TCollection *>(fc.GetList()));
    return std::make_unique<ROOT::RDataFrame>(chain);
  } else if (".root" == input_file_.extension()) {
    return std::make_unique<ROOT::RDataFrame>(input_tree_.c_str(), input_file_.c_str());
  }

  throw std::runtime_error("Unknown extension " + input_file_.extension().string());
}

