//
// Created by eugene on 05/07/2020.
//

#ifndef DATATREEFLOW_SRC_CONFIG_ANALYSISTASK_H
#define DATATREEFLOW_SRC_CONFIG_ANALYSISTASK_H

#include <cassert>
#include <string>
#include <vector>

#include "Axis.h
#include "Correlation.h"
#include "Detector.h"
#include "Utils.h"
#include <yaml-cpp/yaml.h>

namespace config {

struct AnalysisTask {
  std::vector<std::string> event_variables{};
  std::vector<Axis> correction_axes{};
  std::vector<Detector> detectors{};
  std::vector<Histogram> histograms{};

  int n_samples{0};
  config::ESamplingMethod sampling_method;
  std::vector<Correlation> correlations{};

  bool operator==(const AnalysisTask& Rhs) const {
    return event_variables == Rhs.event_variables &&
           correction_axes == Rhs.correction_axes &&
           detectors == Rhs.detectors && histograms == Rhs.histograms;
  }
  bool operator!=(const AnalysisTask& Rhs) const { return !(Rhs == *this); }
};

}  // namespace config

namespace YAML {

template <>
struct convert<config::AnalysisTask> {
  static bool decode(const Node& node, config::AnalysisTask& analysis_task) {
    auto event_variables = node["event_variables"];
    if (event_variables) {
      if (!event_variables.IsSequence()) return false;
      for (auto&& n : event_variables) {
        analysis_task.event_variables.push_back(n.Scalar());
      }
    }

    auto correction_axes = node["correction_axes"];
    if (correction_axes) {
      if (!correction_axes.IsSequence()) return false;
      for (auto&& n : correction_axes) {
        analysis_task.correction_axes.push_back(n.as<config::Axis>());
      }
    }

    analysis_task.detectors =
        node["detectors"].as<std::vector<config::Detector>>();

    analysis_task.histograms =
        node["histograms"].as<std::vector<config::Histogram>>(
            std::vector<config::Histogram>());

    analysis_task.n_samples = node["n-samples"].as<int>();
    analysis_task.sampling_method = node["sampling-method"].as<config::ESamplingMethod>();
    analysis_task.correlations =
        node["correlations"].as<std::vector<config::Correlation>>();

    for (auto& correlation : analysis_task.correlations) {
      for (auto& argument : correlation.arguments) {
        if (!argument.query.is_empty) {
          std::vector<std::string> allowed_query_options{"name", "labels", "type"};
          auto query_result = config::utils::ProcessQuery<config::Detector>(argument.query, node["detectors"], allowed_query_options);
          argument.detectors = query_result.query_result;
        }

      } // argument
    } // correlations

    return true;
  }

  static Node encode(const config::AnalysisTask& task) {
    Node node;
    node["event_variables"] = task.event_variables;
    node["correction_axes"] = task.correction_axes;
    node["detectors"] = task.detectors;
    node["histograms"] = task.histograms;

    return node;
  }
};

}  // namespace YAML

#endif  // DATATREEFLOW_SRC_CONFIG_ANALYSISTASK_H
