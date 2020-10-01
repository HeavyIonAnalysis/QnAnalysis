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
#include <TTreeReader.h>
#include <TTreeReaderValue.h>
#include <boost/program_options.hpp>

#include "CorrelationAction.h"
#include "TensorIndex.h"
#include <yaml-cpp/yaml.h>

#include "AnalysisTask.h"

class CorrelationTask {
 public:
  struct CorrelationArgument {
    std::string detector_name;
    std::string correction_step;
    std::string branch_name;
    config::ECorrelationWeights weights{config::ECorrelationWeights::OBSERVABLE};
  };

  struct Correlation {
    std::string output_name;
    std::string output_prefix;
    std::vector<CorrelationArgument> arguments;
    std::string action_name;
    config::Axis event_axis;

    bool is_valid_input{false};
    bool is_valid_action{false};
    bool is_processed{false};

  };

  boost::program_options::options_description GetBoostOptions() {
    using namespace boost::program_options;

    options_description desc("Options of the CorrelationTask");

    desc.add_options()("input,i", value(&input_file_name_)->required(),
                       "input ROOT file (or list of files *.list)")(
        "output,o", value(&output_file_name_)->required(), "output ROOT file")(
        "config,c", value(&config_file_name_)->required(),
        "name of YAML config file")("config-name,c",
                                    value(&config_name_)->required(),
                                    "name of config node in YAML structure");
    return desc;
  }

  void Initialize();

  void ReadYAMLConfig();

  void ExpandCorrelations();

  void Run();

  void Finalize() {}

  template <size_t NArgs>
  void Process(ROOT::RDF::RInterface<ROOT::Detail::RDF::RLoopManager> &df_sampled) {
    using ::Correlation::Action::GetRegistry;

    Info(__func__, "Processing correlations with %zu arguments...", NArgs);

    for (auto &c : correlations_) {
      if (c.arguments.size() != NArgs) continue;

      std::string output_name = c.output_name;
      int n_samples = config_.n_samples;
      std::array<std::string, NArgs> input_names;
      std::array<Qn::Stats::Weights, NArgs> weights;

      for (size_t i_arg = 0; i_arg < NArgs; ++i_arg) {
        input_names[i_arg] = c.arguments[i_arg].branch_name;
        weights[i_arg] = Convert(c.arguments[i_arg].weights);
      }

      auto event_axis = Qn::MakeAxes(Convert(c.event_axis));

      auto action_fct = GetRegistry<NArgs>().Get(c.action_name);
      auto average_helper = Qn::MakeAverageHelper(Qn::Correlation::MakeCorrelationAction(
          output_name,
          action_fct,
          input_names,
          weights,
          event_axis,
          n_samples
          )).BookMe(df_sampled);

      auto callback = [average_helper, output_name] (const std::shared_ptr<TFile>& output_file) {
        decltype(average_helper) average_helper_local(average_helper);
        Info("Callback", "Evaluating and saving '%s'...", output_name.c_str());
        Qn::DataContainerStats stats = average_helper_local->GetDataContainer();
        output_file->cd();
        stats.Write(output_name.c_str());
      };

      write_stats_callbacks_.emplace_back(callback);

      Info(__func__, "%s", c.output_name.c_str());
      c.is_processed = true;
    }
  }

 private:
  static inline std::string GetCorrectionStepStr(
      config::ECorrectionStep step_enum);

  static inline std::string GetBranchNameStr(const CorrelationArgument &arg);

  static inline std::string GetCorrelationOutputStr(
      const Correlation &correlation);

  static inline Qn::Stats::Weights Convert(config::ECorrelationWeights weights);

  static inline Qn::AxisD Convert(const config::Axis &ax);

  void CheckCorrelationArguments();

  template <size_t NArgs>
  void CheckCorrelationAction() {
    using ::Correlation::Action::GetRegistry;

    auto action_names_nd = GetRegistry<NArgs>().ListKeys();

    for (auto &correlation : correlations_) {
      if (correlation.arguments.size() != NArgs) continue;

      auto & action_name = correlation.action_name;
      correlation.is_valid_action = std::any_of(action_names_nd.begin(), action_names_nd.end(),
          [action_name] (const std::string& name) { return action_name == name; });

      if (!correlation.is_valid_action) {
        Warning(__func__, "Action '%s' is missing for '%s'", action_name.c_str(), correlation.output_name.c_str());
      }
    } // correlations
  }

  std::string input_file_name_;

  std::string config_file_name_;
  std::string config_name_;
  config::AnalysisTask config_;

  std::vector<Correlation> correlations_;

  std::string output_file_name_;
  std::shared_ptr<TFile> output_file_;

  std::list<std::function<void (std::shared_ptr<TFile>)>> write_stats_callbacks_;

  TTree *input_tree_{nullptr};
};

#endif  // DATATREEFLOW_SRC_CORRELATION_CORRELATIONTASK_H
