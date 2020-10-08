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
  InitializeTasks();
}
void Qn::Analysis::Correlate::CorrelationTaskRunner::Run() {
  Info(__func__, "Go!");

  for (auto &task : initialized_tasks_) {
    for (auto &c : task->correlations) {
      std::cout << "Processing " << c.result_ptr->GetName() << std::endl;
      c.result_ptr.GetValue();
    }
  }

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
      } catch (bad_config_file &e) {
        Warning(__func__, "%s", e.what());
      }
    }
  }

}
bool Qn::Analysis::Correlate::CorrelationTaskRunner::LoadConfiguration(const std::filesystem::path &path) {
  using namespace YAML;

  Node top_node;
  try {
    top_node = LoadFile(path.string());
  } catch (YAML::BadFile &e) {
    throw bad_config_file(e);
  }

  Info(__func__, "Loaded %s...", path.c_str());

  config_tasks_ = top_node[configuration_node_name_].as<std::vector<CorrelationTask>>();
  return true;
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

  throw std::runtime_error("Unknown input file extension " + input_file_.extension().string());
}

Qn::AxisD CorrelationTaskRunner::ToQnAxis(const AxisConfig &c) {
  if (c.type == AxisConfig::RANGE) {
    return Qn::AxisD(c.variable, c.nb, c.lo, c.hi);
  } else if (c.type == AxisConfig::BIN_EDGES) {
    return Qn::AxisD(c.variable, c.bin_edges);
  }
  __builtin_unreachable();
}

std::string CorrelationTaskRunner::ToQVectorFullName(const QVectorTagged &qv) {
  return qv.name + "_" + qv.correction_step._to_string();
}

