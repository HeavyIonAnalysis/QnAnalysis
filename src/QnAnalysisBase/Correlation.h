//
// Created by eugene on 29/07/2020.
//

#ifndef DATATREEFLOW_SRC_CONFIG_CORRELATION_H
#define DATATREEFLOW_SRC_CONFIG_CORRELATION_H

#include <iostream>
#include <string>
#include <vector>

#include <CorrectionStep.h>

#include "Detector.h"
#include "Utils.h"
#include <yaml-cpp/yaml.h>

namespace config {

enum class ESamplingMethod {
  SUBSAMPLING,
  BOOTSTRAP
};

enum class ECorrelationWeights {
  OBSERVABLE,
  REFERENCE
};

struct CorrelationArgument {
  utils::Query query;
  ECorrectionStep correction_step{ECorrectionStep::PLAIN};
  ECorrelationWeights weights{ECorrelationWeights::OBSERVABLE};

  std::vector<Detector> detectors; // transient field
};

struct Correlation {
  std::vector<CorrelationArgument> arguments;
  std::vector<std::string> actions;
  std::string output_prefix;

  Axis event_axis;
};

}  // namespace config

namespace YAML {

template <>
struct convert<config::ECorrelationWeights> {

  static bool decode(const Node& node, config::ECorrelationWeights &weights) {
    using config::utils::Details::CompareStrings;
    using config::utils::EStringMatchingPolicy;


    const auto& node_str = node.Scalar();

    if (CompareStrings("reference", node_str, EStringMatchingPolicy::CASE_INSENSITIVE)) {
      weights = config::ECorrelationWeights::REFERENCE;
      return true;
    } else if (CompareStrings("observable", node_str, EStringMatchingPolicy::CASE_INSENSITIVE)) {
      weights = config::ECorrelationWeights::OBSERVABLE;
      return true;
    }

    return false;
  }

};

template <>
struct convert<config::ESamplingMethod> {


  static bool decode(const Node& node, config::ESamplingMethod &sampling_method) {

    using config::utils::Details::CompareStrings;
    using config::utils::EStringMatchingPolicy;

    const auto& node_str = node.Scalar();

    if (CompareStrings("subsampling", node_str, EStringMatchingPolicy::CASE_INSENSITIVE)) {
      sampling_method = config::ESamplingMethod::SUBSAMPLING;
      return true;
    } else if (CompareStrings("bootstrap", node_str, EStringMatchingPolicy::CASE_INSENSITIVE)) {
      sampling_method = config::ESamplingMethod::BOOTSTRAP;
      return true;
    }

    return false;
  }

};

template <>
struct convert<config::CorrelationArgument> {
  static bool decode(const Node& node,
                     config::CorrelationArgument& correlation_argument) {
    if (node["query"] && node["detectors"]) {
      return false;
    }

    correlation_argument.query =
        node["query"].as<config::utils::Query>(config::utils::Query());
    correlation_argument.detectors =
        node["detectors"].as<std::vector<config::Detector>>(std::vector<config::Detector>());
    correlation_argument.weights =
        node["weights"].as<config::ECorrelationWeights>();
    correlation_argument.correction_step =
        node["correction-step"].as<config::ECorrectionStep>();
    return true;
  }
};

template <>
struct convert<config::Correlation> {
  static bool decode(const Node& node, config::Correlation& correlation) {
    correlation.arguments =
        node["arguments"].as<std::vector<config::CorrelationArgument>>();
    correlation.actions = node["actions"].as<std::vector<std::string>>();
    correlation.output_prefix = node["output-prefix"].as<std::string>("");
    correlation.event_axis = node["event-axis"].as<config::Axis>();
    return true;
  }
};

}  // namespace YAML

#endif  // DATATREEFLOW_SRC_CONFIG_CORRELATION_H
