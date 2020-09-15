//
// Created by eugene on 11/08/2020.
//

#ifndef FLOW_SRC_CONFIG_CONFIG_H
#define FLOW_SRC_CONFIG_CONFIG_H

#include <yaml-cpp/yaml.h>

#include "ConfigUtils.h"

#include <base/AnalysisSetup.h>
#include <base/Axis.h>
#include <base/Cut.h>
#include <base/QVector.h>
#include <base/Variable.h>

namespace Qn::Analysis::Config {

Base::AnalysisSetup ReadSetupFromFile(const std::string& filename, const std::string& config_name);

}

namespace YAML {

/* VARIABLE */
template<>
struct convert<Qn::Analysis::Base::VariableConfig> {

  static bool decode(const Node& node, Qn::Analysis::Base::VariableConfig& var) {
    using Qn::Analysis::Config::Utils::TokenizeString;
    if (node.IsScalar()) {
      if (node.Scalar() == "Ones" || node.Scalar() == "ones" || node.Scalar() == "ONES") {
        var = Qn::Analysis::Base::VariableConfig::Ones();
        return true;
      }

      auto tokenized = TokenizeString(node.Scalar(), '/');
      if (tokenized.size() == 2) {
        var.branch = tokenized[0];
        var.field = tokenized[1];
        return true;
      }
      /* todo warning */
      return false;
    } else if (node.IsMap()) {
      var.branch = node["branch"].Scalar();
      var.field = node["field"].Scalar();
      return true;
    }
    /* todo warning */
    return false;
  }
};

template<>
struct convert<Qn::Analysis::Base::AxisConfig> {

  static bool decode(const Node& node, Qn::Analysis::Base::AxisConfig& axis_config) {
    using namespace Qn::Analysis::Base;
    if (node.IsMap()) {
      axis_config.variable = node["name"].as<VariableConfig>();

      if (node["nb"] && node["lo"] && node["hi"]) {
        axis_config.type = AxisConfig::RANGE;
        axis_config.nb = node["nb"].as<int>();
        axis_config.lo = node["lo"].as<double>();
        axis_config.hi = node["hi"].as<double>();
        return true;
      } else if (node["bin-edges"]) {
        axis_config.type = AxisConfig::BIN_EDGES;
        axis_config.bin_edges = node["bin-edges"].as<std::vector<double>>();
        return true;
      }

      return false;
    }

    return false;
  }
};

template<>
struct convert<Qn::Analysis::Base::CutConfig> {

  static bool decode(const Node& node, Qn::Analysis::Base::CutConfig& cut_config) {
    using namespace Qn::Analysis::Base;
    if (node.IsMap()) {
      cut_config.variable = node["variable"].as<VariableConfig>(VariableConfig());

      if (node["equals"]) {
        cut_config.type = CutConfig::EQUAL;
        cut_config.equal_val = node["equals"].as<double>();
        cut_config.equal_tol = node["tol"].as<double>(0.);
        return true;
      } else if (node["range"]) {
        if (node["range"].IsSequence() && node["range"].size() == 2) {
          cut_config.type = CutConfig::RANGE;
          cut_config.range_lo = node["range"][0].as<double>();
          cut_config.range_hi = node["range"][1].as<double>();
          return true;
        }

        return false;
      } else if (node["function"]) {
        cut_config.type = CutConfig::NAMED_FUNCTION;
        cut_config.named_function_name = node["function"].Scalar();
        return true;
      }

      /* todo warning user */
      return false;
    }

    return false;
  }
};

template<>
struct convert<Qn::Analysis::Base::HistogramConfig> {

  static bool decode(const Node& node, Qn::Analysis::Base::HistogramConfig& config) {
    using namespace Qn::Analysis::Base;
    if (node.IsMap()) {
      /* it could be one axis */
      try {
        auto ax = node.as<AxisConfig>();
        config.axes.emplace_back(std::move(ax));
        return true;
      } catch (std::exception& e) { /* ignore */
      }

      config.axes = node["axes"].as<std::vector<AxisConfig>>();
      return true;
    } else if (node.IsSequence()) {
      config.axes = node.as<std::vector<AxisConfig>>();
      return true;
    }// IsSequence

    return false;
  }
};

template<>
struct convert<Qn::Analysis::Base::QVectorCorrectionConfig> {

  static bool decode(const Node& node, Qn::Analysis::Base::QVectorCorrectionConfig& config) {
    using namespace Qn::Analysis::Base;
    using namespace Qn::Analysis::Config::Utils;

    if (node.IsMap()) {
      auto type_str = node["type"].Scalar();
      if (type_str == "recentering") {
        config.type = EQVectorCorrectionType::RECENTERING;
      } else if (type_str == "twist-and-rescale" || type_str == "twist_and_rescale") {
        config.type = EQVectorCorrectionType::TWIST_AND_RESCALE;
      } else {
        return false;
      }

      config.no_of_entries = node["no-entries"].as<int>(0);

      if (config.type == EQVectorCorrectionType::RECENTERING) {
        config.recentering_width_equalization = node["apply-width-equalization"].as<bool>(false);
        return true;
      } else if (config.type == EQVectorCorrectionType::TWIST_AND_RESCALE) {
        auto method_str = node["method"].Scalar();
        if (method_str == "double-harmonic" || method_str == "double_harmonic") {
          config.twist_rescale_method = ETwistRescaleMethod::DOUBLE_HARMONIC;
        } else if (method_str == "correlations") {
          config.twist_rescale_method = ETwistRescaleMethod::CORRELATIONS;
        } else {
          return false;
        }
        config.twist_rescale_apply_twist = node["apply-twist"].as<bool>();
        config.twist_rescale_apply_rescale = node["apply-rescale"].as<bool>();
        return true;
      }

      assert(false);
    }
    /* presets */
    else if (node.IsScalar()) {

      if (node.Scalar() == "recentering") {
        config = QVectorCorrectionConfig::RecenteringDefault();
        return true;
      } else if (node.Scalar() == "twist-and-rescale") {
        config = QVectorCorrectionConfig::TwistAndRescaleDefault();
        return true;
      }

      /* invalid preset */
      return false;
    }

    return false;
  }
};

template<>
struct convert<Qn::Analysis::Base::QVectorConfig> {

  static bool decode(const Node& node, Qn::Analysis::Base::QVectorConfig& config) {
    using namespace Qn::Analysis::Base;
    using namespace Qn::Analysis::Config::Utils;

    if (node.IsMap()) {
      config.name = node["name"].Scalar();

      auto type_str = node["type"].Scalar();
      if (type_str == "track" || type_str == "TRACK") {
        config.type = EQVectorType::TRACK;
      } else if (type_str == "channel" || type_str == "CHANNEL") {
        config.type = EQVectorType::CHANNEL;
      } else if (type_str == "psi" || type_str == "PSI") {
        config.type = EQVectorType::EVENT_PSI;
      } else {
        /* todo notify */
        return false;
      }

      config.phi = node["phi"].as<VariableConfig>();
      config.weight = node["weight"].as<VariableConfig>(VariableConfig::Ones());
      config.harmonics = node["harmonics"].as<std::string>("00000111");
      config.corrections =
          node["corrections"].as<std::vector<QVectorCorrectionConfig>>(EmptyVector<QVectorCorrectionConfig>());

      if (node["qa"] && node["qa"].IsSequence()) {
        config.qa = node["qa"].as<std::vector<HistogramConfig>>();
      }

      if (config.type == EQVectorType::TRACK) {
        /* axes */
        config.axes = node["axes"].as<std::vector<AxisConfig>>(EmptyVector<AxisConfig>());
        /* cuts */
        if (node["cuts"] && node["cuts"].IsMap()) {
          /* Shortcut: KEY = Variable, VALUE = Cut with empty target */
          for (auto& map_element : node["cuts"]) {
            auto cut_variable = map_element.first.as<VariableConfig>();
            auto cut_config = map_element.second.as<CutConfig>();
            cut_config.variable = cut_variable;
            config.cuts.emplace_back(std::move(cut_config));
          }
        } else if (node["cuts"] && node["cuts"].IsSequence()) {
          config.cuts = node["cuts"].as<std::vector<CutConfig>>();
        }
      } else if (config.type == EQVectorType::CHANNEL) {
        config.channel_ids = node["channel-ids"].as<std::vector<int>>(EmptyVector<int>());
      }
      return true;
    }// IsMap

    return false;
  }
};

template<>
struct convert<Qn::Analysis::Base::AnalysisSetupConfig> {
  static bool decode(const Node& node, Qn::Analysis::Base::AnalysisSetupConfig& config) {
    using namespace Qn::Analysis::Base;
    using namespace Qn::Analysis::Config::Utils;
    if (node.IsMap()) {
      config.event_axes = node["axes"].as<std::vector<AxisConfig>>(EmptyVector<AxisConfig>());
      config.event_variables = node["event-variables"].as<std::vector<VariableConfig>>(EmptyVector<VariableConfig>());
      config.q_vectors = node["q-vectors"].as<std::vector<QVectorConfig>>();
      return true;
    }// IsMap

    return false;
  }
};

}// namespace YAML

#endif//FLOW_SRC_CONFIG_CONFIG_H
