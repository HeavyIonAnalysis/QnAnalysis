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

#include "Config.hpp"

using std::filesystem::path;
using std::filesystem::current_path;
using namespace Qn::Analysis::Correlate;



boost::program_options::options_description Qn::Analysis::Correlate::CorrelationTaskRunner::GetBoostOptions() {
  using namespace boost::program_options;
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

void CorrelationTaskRunner::InitializeTasks() {
  initialized_tasks_.clear();
  InitializeTasksImpl(std::back_inserter(initialized_tasks_),
                      config_tasks_,
                      make_index_range<1, MAX_ARITY>(), make_index_range<1, MAX_AXES>());
}

void Qn::Analysis::Correlate::CorrelationTaskRunner::Run() {
  Info(__func__, "Go!");

  TFile f(output_file_.c_str(), "RECREATE");

  for (auto &task : initialized_tasks_) {
    auto dir = mkcd(task->output_folder, f);

    for (auto &correlation : task->correlations) {
      Info(__func__, "Processing '%s'... ", correlation.result_ptr->GetName().c_str());
      auto &container = correlation.result_ptr.GetValue().GetDataContainer();

      auto correlation_meta = GenCorrelationMeta(correlation);

      dir->WriteObject(&container, correlation.meta_key.c_str());
      dir->WriteObject(&correlation_meta, (correlation.meta_key + "_meta").c_str());
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


  auto make_arg = [] (const QVectorTagged& qv,
                      EQnCorrectionStep step,
                      const std::string& component_name,
                      const std::string& weight) -> CorrelationArg {
    return {
      .q_vector_name = qv.name,
      .correction_step = step,
      .component = std::string(component_name),
      .weight = weight
    };
  };

  std::vector<CorrelationArgList> arglists_to_combine;
  for (auto &task_arg : t.arguments) {
    CorrelationArgList arglist;
    Utils::Combine(std::back_inserter(arglist), make_arg,
                   task_arg.query_result,
                   task_arg.corrections_steps,
                   task_arg.components,
                   std::vector<std::string>({task_arg.weight}));
    arglists_to_combine.emplace_back(std::move(arglist));
  }

  std::vector<CorrelationArgList> arglists_combined;
  Utils::CombineDynamic(std::begin(arglists_to_combine), std::end(arglists_to_combine),
                        std::back_inserter(arglists_combined));


  /* now we combine them with actions */
  auto make_correlation = [](const CorrelationArgList &args) {
    Correlation c;
    c.args_list = args;
    std::transform(std::begin(c.args_list), std::end(c.args_list),
                   std::back_inserter(c.argument_names),
                   [] (const CorrelationArg& arg) { return arg.q_vector_name + "_" + arg.correction_step._to_string(); });
    auto meta_key = JoinStrings(c.argument_names.begin(), c.argument_names.end(), ".");
    meta_key.append(".");
    for (auto &arg : c.args_list) {
      meta_key.append(arg.component);
    }

    c.meta_key = std::move(meta_key);
    return c;
  };

  std::vector<Correlation> result;
  std::transform(
      std::begin(arglists_combined), std::end(arglists_combined), std::back_inserter(result),
      make_correlation);

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

TStringMeta CorrelationTaskRunner::GenCorrelationMeta(const CorrelationTaskRunner::Correlation &c) {
  using namespace YAML;

  Node n;
  n["meta_key"] = c.meta_key;
  for (auto &arg : c.args_list) {
    Node arg_node;
    arg_node["q-vector"] = arg.q_vector_name;
    arg_node["correction-step"] = Enum<EQnCorrectionStep>(arg.correction_step);
    arg_node["component"] = arg.component;
    arg_node["weight"] = arg.weight;
    n["args"].push_back(arg_node);
  }

  std::stringstream stream;
  stream << n;
  return TStringMeta(stream.str());
}

CorrelationTaskRunner::QVectorComponentFct CorrelationTaskRunner::GetQVectorComponentFct(const CorrelationArg &arg) {
  using namespace std::regex_constants;
  const std::regex re("^(x|y|cos|sin)(\\d)$", ECMAScript | icase);

  std::smatch match_results;
  bool match_ok = std::regex_search(arg.component, match_results, re);

  // [0] - entire string, [1] - component, [2] - harmonic
  if (!(match_ok || match_results.size() == 3)) {
    throw bad_qvector_component();
  }

  auto harmonic = boost::lexical_cast<unsigned int>(match_results[2].str());

  if (match_results[1] == "x") {
    return {.component = QVectorComponentFct::kX, .harmonic=harmonic};
  }
  else if (match_results[1] == "y") {
    return {.component = QVectorComponentFct::kY, .harmonic=harmonic};
  }
  else if (match_results[1] == "cos" || match_results[1] == "Cos") {
    return {.component = QVectorComponentFct::kCos, .harmonic=harmonic};
  }
  else if (match_results[1] == "sin" || match_results[1] == "Sin") {
    return {.component = QVectorComponentFct::kSin, .harmonic=harmonic};
  }
  else {
    throw bad_qvector_component();
  }
}
CorrelationTaskRunner::QVectorWeightFct CorrelationTaskRunner::GetQVectorWeightFct(const CorrelationTaskRunner::CorrelationArg &arg) {
  using namespace std::regex_constants;
  const std::regex re("^(Sumw|Ones)$", ECMAScript | icase);

  std::smatch match_results;
  bool match_ok = std::regex_search(arg.weight, match_results, re);
  if (!match_ok) {
    throw bad_qvector_weight();
  }
  if (match_results[1] == "Sumw" || match_results[1] == "sumw") {
    return {.type = QVectorWeightFct::kSumw};
  } else if (match_results[1] == "ones" || match_results[1] == "Ones") {
    return {.type = QVectorWeightFct::kOne};
  } else {
    throw bad_qvector_weight();
  }
}

