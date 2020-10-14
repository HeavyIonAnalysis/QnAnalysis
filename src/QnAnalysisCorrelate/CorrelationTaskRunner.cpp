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
#include <TDirectory.h>

using std::filesystem::path;
using std::filesystem::current_path;
using namespace boost::program_options;

using namespace Qn::Analysis::Correlate;

boost::program_options::options_description Qn::Analysis::Correlate::CorrelationTaskRunner::GetBoostOptions() {
  options_description desc("Correlation options");
  desc.add_options()
      ("configuration-file", value(&configuration_file_path_)->required(), "Path to the YAML configuration")
      ("configuration-name", value(&configuration_node_name_)->required(), "Name of YAML node")
      ("input-file,i", value(&input_file_name_)->required(), "Name of the input ROOT file (or .list file)")
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

  TFile f(output_file_.c_str(), "RECREATE");

  TObjString container_meta;
  for (auto &task : initialized_tasks_) {
    auto dir = mkcd(task->output_folder, f);

    for (auto &correlation : task->correlations) {
      Info(__func__, "Processing '%s'... ", correlation.result_ptr->GetName().c_str());
      auto &container = correlation.result_ptr.GetValue().GetDataContainer();

      container_meta.String() = GenMeta(correlation);

      dir->WriteObject(&container, correlation.meta_key.c_str());
      dir->WriteObject(&container_meta, (correlation.meta_key + "_meta").c_str());
    }
  }

  f.Close();
  Info(__func__, "Written to '%s'...", f.GetName());

}

void Qn::Analysis::Correlate::CorrelationTaskRunner::LookupConfiguration() {
  if (configuration_file_path_.is_absolute()) {
    LoadConfiguration(configuration_file_path_);
  } else {
    bool is_loaded{false};
    for (const auto &lookup_dir : {current_path(), path(::Qn::Analysis::GetSetupsDir())}) {
      auto lookup_path = lookup_dir / configuration_file_path_;
      Info(__func__, "Looking for configuration in '%s'", lookup_path.c_str());
      try {
        LoadConfiguration(lookup_path);
        is_loaded = true;
        break;
      } catch (bad_config_file &e) {
        Warning(__func__, "File '%s' not found", lookup_path.c_str());
      }
    }

    if (!is_loaded) {
      throw std::runtime_error("Unable to find YAML configuration file");
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

std::shared_ptr<TTree> CorrelationTaskRunner::GetTree() {
  if (".list" == input_file_name_.extension()) {
    TFileCollection fc("fc", "", input_file_name_.c_str());
    auto chain = std::make_shared<TChain>(input_tree_.c_str(), "");
    chain->AddFileInfoList(reinterpret_cast<TCollection *>(fc.GetList()));
    return chain;
  } else if (".root" == input_file_name_.extension()) {
    auto input_file = TFile::Open(input_file_name_.c_str(), "READ");
    auto tree_ptr = input_file->Get<TTree>(input_tree_.c_str());

    auto tree_deleter = [input_file](TTree *t) -> void {
      delete input_file;
    };
    return std::shared_ptr<TTree>(tree_ptr, tree_deleter);
  }
  throw std::runtime_error("Unknown input file extension " + input_file_name_.extension().string());
}

std::shared_ptr<ROOT::RDataFrame> CorrelationTaskRunner::GetRDF() {
  if (".list" == input_file_name_.extension()) {
    TFileCollection fc("fc", "", input_file_name_.c_str());
    auto chain = new TChain(input_tree_.c_str(), "");
    chain->AddFileInfoList(reinterpret_cast<TCollection *>(fc.GetList()));
    /* Lifetime of the chain is managed by RDataFrame */
    return std::make_shared<ROOT::RDataFrame>(*chain);
  } else if (".root" == input_file_name_.extension()) {
    return std::make_shared<ROOT::RDataFrame>(input_tree_.c_str(), input_file_name_.c_str());
  }
  throw std::runtime_error("Unknown input file extension " + input_file_name_.extension().string());
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

std::vector<CorrelationTaskRunner::Correlation> CorrelationTaskRunner::GetTaskCombinations(const CorrelationTask &t) {
  std::vector<QVectorList> argument_lists_to_combine;
  argument_lists_to_combine.reserve(t.arguments.size());
  for (auto &arg_list : t.arguments) {
    argument_lists_to_combine.emplace_back(arg_list.query_result);
  }
  std::vector<QVectorList> arguments_combined;
  Utils::CombineDynamic(argument_lists_to_combine.begin(),
                        argument_lists_to_combine.end(),
                        std::back_inserter(arguments_combined));

  /* now we combine them with actions */
  auto make_correlation = [](const QVectorList &qv, const std::string &action) {
    Correlation c;
    c.action_name = action;
    c.args_list = qv;
    std::transform(std::begin(c.args_list), std::end(c.args_list),
                   std::back_inserter(c.argument_names),
                   ToQVectorFullName);
    auto meta_key = JoinStrings(c.argument_names.begin(), c.argument_names.end(), ".")
        .append(".").append(c.action_name);
    c.meta_key = std::move(meta_key);
    return c;
  };

  std::vector<Correlation> result;
  Utils::Combine(std::back_inserter(result), make_correlation, arguments_combined, t.actions);

  return result;
}

TDirectory *CorrelationTaskRunner::mkcd(const path &path, TDirectory& root_dir) {
  TDirectory *pwd = &root_dir;
  for (const auto &path_ele : path) {
    if (path_ele == "/")
      continue;
    pwd = pwd->mkdir(path_ele.c_str(), "", kTRUE);
  }
  return pwd;
}
std::vector<QVectorTagged> CorrelationTaskRunner::LookupQVectors(TTree *t) {
  auto list_branches = t->GetListOfBranches();

  for (auto brobj : *list_branches) {
    brobj->Print();
  }

  return {};
}

std::string CorrelationTaskRunner::GenMeta(const CorrelationTaskRunner::Correlation &c) {
  using namespace YAML;

  Node n;
  n["meta_key"] = c.meta_key;
  n["action_name"] = c.action_name;
  n["argument_names"] = c.argument_names;

  std::stringstream stream;
  stream << n;
  return stream.str();
}

