//
// Created by eugene on 21/07/2020.
//

#include "CorrelationTask.h"


void CorrelationTask::ExpandCorrelations() {


  for (auto &config_correlation : config_.correlations) {
    auto &config_arguments = config_correlation.arguments;
    std::vector<size_t> tensor_shape(config_arguments.size());
    std::transform(config_arguments.begin(), config_arguments.end(),
                   tensor_shape.begin(),
                   [](config::CorrelationArgument &arg) {
                     return arg.detectors.size();
                   });
    tensor_shape.push_back(config_correlation.actions.size());

    Details::TensorIndex index(tensor_shape);

    for (size_t i = 0; i < index.size(); ++i) {
      Correlation correlation;
      auto index_element = index[i];

      /* populate arguments */
      for (size_t i_arg = 0; i_arg < index.dim() - 1; ++i_arg) {
        CorrelationArgument arg;
        config::Detector &arg_detector =
            config_arguments[i_arg].detectors[index_element[i_arg]];
        arg.detector_name = arg_detector.name;
        arg.correction_step =
            GetCorrectionStepStr(config_arguments[i_arg].correction_step);
        arg.branch_name = GetBranchNameStr(arg);
        arg.weights = config_arguments[i_arg].weights;
        correlation.arguments.emplace_back(std::move(arg));
      }

      correlation.action_name =
          config_correlation.actions[index_element.back()];

      correlation.output_prefix = config_correlation.output_prefix;

      correlation.output_name = GetCorrelationOutputStr(correlation);

      correlation.event_axis = config_correlation.event_axis;

      correlations_.emplace_back(std::move(correlation));
    }  // correlations
  }

  std::cout << "Done" << std::endl;
}

void CorrelationTask::CheckCorrelationArguments() {

  TTreeReader reader(input_tree_);
  for (auto &correlation : correlations_) {
    auto& arguments = correlation.arguments;
    correlation.is_valid_input =
        std::all_of(arguments.begin(), arguments.end(), [&reader] (const CorrelationArgument& arg) {
          TTreeReaderValue<Qn::DataContainerQVector> val(reader, arg.branch_name.c_str());
          reader.SetEntry(1);
          auto result = (val.GetSetupStatus() >= 0);

          if (!result) {
            Warning("Initialize", "Missing branch '%s'", arg.branch_name.c_str());
          }

          reader.Restart();
          return result;
        });

    if (!correlation.is_valid_input) {
      Warning("Initialize", "Some arguments are missing for '%s'", correlation.output_name.c_str());
    }
  }



}

std::string CorrelationTask::GetCorrelationOutputStr(
    const CorrelationTask::Correlation &correlation) {
  std::string result;

  std::string join_result;
  for (auto &arg : correlation.arguments) {
    if (!join_result.empty()) join_result.append(".");
    join_result.append(arg.detector_name);
  }
  join_result.append(".").append(correlation.action_name);

  result.append(correlation.output_prefix).append(join_result);
  return result;
}

std::string CorrelationTask::GetBranchNameStr(
    const CorrelationTask::CorrelationArgument &arg) {
  std::string result;
  result.append(arg.detector_name).append("_").append(arg.correction_step);
  return result;
}


std::string CorrelationTask::GetCorrectionStepStr(
    config::ECorrectionStep step_enum) {
  switch (step_enum) {
    case config::ECorrectionStep::PLAIN:
      return "PLAIN";
    case config::ECorrectionStep::RECENTERED:
      return "RECENTERED";
    case config::ECorrectionStep::TWIST:
      return "TWIST";
    case config::ECorrectionStep::RESCALED:
      return "RESCALED";
  }

  assert(false);
}

void CorrelationTask::Run() {


  ROOT::RDataFrame df("tree", input_file_name_);
  auto resampled_dfs = Qn::Correlation::Resample(df, config_.n_samples);

  output_file_.reset(TFile::Open(output_file_name_.c_str(), "RECREATE"));

  Process<1>(resampled_dfs);
  Process<2>(resampled_dfs);
  Process<3>(resampled_dfs);
  Process<4>(resampled_dfs);
  Process<5>(resampled_dfs);
  Process<6>(resampled_dfs);

  for (auto &cb : write_stats_callbacks_) {
    cb(output_file_);
  }

}

void CorrelationTask::Initialize() {
  ReadYAMLConfig();
  input_tree_ = utils::MakeChain(input_file_name_, "tree");

  ExpandCorrelations();

  CheckCorrelationArguments();
  CheckCorrelationAction<1>();
  CheckCorrelationAction<2>();
  CheckCorrelationAction<3>();
  CheckCorrelationAction<4>();
  CheckCorrelationAction<5>();
  CheckCorrelationAction<6>();

  std::vector<Correlation> filtered_correlations;
  std::copy_if(correlations_.begin(), correlations_.end(), std::back_inserter(filtered_correlations),
               [] (const Correlation& c) { return c.is_valid_input && c.is_valid_action; });

  std::swap(correlations_, filtered_correlations);
}

Qn::Stats::Weights CorrelationTask::Convert(
    config::ECorrelationWeights weights) {
  switch (weights) {
    case config::ECorrelationWeights::REFERENCE: return Qn::Stats::Weights::REFERENCE;
    case config::ECorrelationWeights::OBSERVABLE: return Qn::Stats::Weights::OBSERVABLE;
  }
  assert(false);
}

Qn::AxisD CorrelationTask::Convert(const config::Axis &ax) {
  if (!ax.bin_edges.empty()) {
    assert(std::is_sorted(ax.bin_edges.begin(), ax.bin_edges.end()));
    return Qn::AxisD(ax.name, ax.bin_edges);
  } else {
    assert(ax.nbins > 0 && ax.hi > ax.lo);
    return Qn::AxisD(ax.name, ax.nbins, ax.lo, ax.hi);
  }
  assert(false);
}

void CorrelationTask::ReadYAMLConfig() {
  auto node = YAML::LoadFile(config_file_name_);
  config_ = node[config_name_].as<config::AnalysisTask>();
}
