//
// Created by eugene on 11/08/2020.
//

#ifndef FLOW_SRC_CONFIG_CONFIG_H
#define FLOW_SRC_CONFIG_CONFIG_H

#include <yaml-cpp/yaml.h>
#include <regex>

#include "ConfigUtils.hpp"
#include "YamlUtils.hpp"

#include <QnAnalysisBase/AnalysisSetup.hpp>
#include <QnAnalysisBase/Axis.hpp>
#include <QnAnalysisBase/Cut.hpp>
#include <QnAnalysisBase/QVector.hpp>
#include <QnAnalysisBase/Variable.hpp>

namespace Qn::Analysis::Config {

Base::AnalysisSetup ReadSetupFromFile(const std::string &filename, const std::string &config_name);

}

namespace YAML {

/* VARIABLE */
template<>
struct convert<Qn::Analysis::Base::VariableConfig> {

  static bool decode(const Node &node, Qn::Analysis::Base::VariableConfig &var) {
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

  static bool decode(const Node &node, Qn::Analysis::Base::AxisConfig &axis_config) {
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

  static bool decode_impl(const Node &node,
                          Qn::Analysis::Base::CutConfig &cut_config,
                          bool require_variable_field) {
    using namespace Qn::Analysis::Base;


    if (node["equals"]) {
      if (require_variable_field) {
        cut_config.variable = node["variable"].as<VariableConfig>();
      }
      cut_config.type = CutConfig::EQUAL;
      cut_config.equal_val = node["equals"].as<double>();
      cut_config.equal_tol = node["tol"].as<double>(0.);
      return true;
    } else if (node["range"]) {
      if (node["range"].IsSequence() && node["range"].size() == 2) {
        if (require_variable_field) {
          cut_config.variable = node["variable"].as<VariableConfig>();
        }
        cut_config.type = CutConfig::RANGE;
        cut_config.range_lo = node["range"][0].as<double>();
        cut_config.range_hi = node["range"][1].as<double>();
        return true;
      }
      return false;
    } else if (node["any-of"]) {
      if (require_variable_field) {
        cut_config.variable = node["variable"].as<VariableConfig>();
      }
      if (node["any-of"].IsSequence()) {
        cut_config.type = CutConfig::ANY_OF;
        cut_config.any_of_values = node["any-of"].as<std::vector<double>>();
        cut_config.any_of_tolerance = node["tol"].as<double>(0.);
        return true;
      }
      return false;
    } else if (node["expr"]) {
      cut_config.type = CutConfig::EXPR;
      cut_config.expr_string = node["expr"].as<std::string>();
      cut_config.expr_parameters = node["parameters"].as<std::vector<double>>(std::vector<double>());
      return true;
    }
    throw std::runtime_error("Unknown type of the cut");
  }

  static bool decode(const Node &node,
                     Qn::Analysis::Base::CutConfig &cut_config) {
    return decode_impl(node, cut_config, true);
  }
};

template<>
struct convert<Qn::Analysis::Base::CutListConfig> {
  static bool decode(const Node &cut_list_node,
                     Qn::Analysis::Base::CutListConfig &cut_list_config) {
    using Qn::Analysis::Base::CutConfig;
    if (cut_list_node.IsMap()) {
      for (const auto &cut_definition_config : cut_list_node) {
        auto variable = cut_definition_config.first.as<Qn::Analysis::Base::VariableConfig>();
        CutConfig cut_definition;
        assert(convert<CutConfig>::decode_impl(cut_definition_config.second, cut_definition, false));
        cut_definition.variable = variable;
        cut_list_config.cuts.emplace_back(cut_definition);
      }
      return true;
    } else if (cut_list_node.IsSequence()) {
      cut_list_config.cuts = cut_list_node.as<std::list<CutConfig>>();
      return true;
    } else {
      throw std::runtime_error("List of cuts must be either sequence or map");
    }
  }
};

template<>
struct convert<Qn::Analysis::Base::HistogramConfig> {

  static bool decode(const Node &node, Qn::Analysis::Base::HistogramConfig &config) {
    using namespace Qn::Analysis::Base;
    if (node.IsMap()) {
      /* it could be one axis */
      try {
        auto ax = node.as<AxisConfig>();
        config.axes.emplace_back(std::move(ax));
        /* taking weight (if present) directly from the same node */
        config.weight = node["weight"].as<std::string>("Ones");
        return true;
      } catch (std::exception &e) { /* ignore */
      }

      config.axes = node["axes"].as<std::vector<AxisConfig>>();
      config.weight = node["weight"].as<std::string>("Ones");
      return true;
    } else if (node.IsSequence()) {
      config.axes = node.as<std::vector<AxisConfig>>();
      config.weight = "Ones";
      return true;
    }// IsSequence

    return false;
  }
};

template<>
struct convert<Qn::Analysis::Base::QVectorCorrectionConfig> {

  static bool decode(const Node &node, Qn::Analysis::Base::QVectorCorrectionConfig &config) {
    using namespace Qn::Analysis::Base;
    using namespace Qn::Analysis::Config::Utils;

    if (node.IsMap()) {
      auto type_str = node["type"].Scalar();
      if (type_str == "recentering") {
        config.type = EQVectorCorrectionType::RECENTERING;
      } else if (type_str == "twist-and-rescale" || type_str == "twist_and_rescale") {
        config.type = EQVectorCorrectionType::TWIST_AND_RESCALE;
      } else if (type_str == "alignment") {
        config.type = EQVectorCorrectionType::ALIGNMENT;
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
      } else if (config.type == EQVectorCorrectionType::ALIGNMENT) {
        config.alignment_harmonic = node["alignment-harmonic"].as<int>(1);
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

  static bool decode(const Node &node, Qn::Analysis::Base::QVectorConfig &config) {
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

      auto norm_str = node["norm"].as<std::string>("none");
      if (norm_str == "none" || norm_str == "NONE") {
        config.normalization = Qn::QVector::Normalization::NONE;
      } else if (norm_str == "m" || norm_str == "M") {
        config.normalization = Qn::QVector::Normalization::M;
      } else if (norm_str == "mag" || norm_str == "MAG" || norm_str == "magnitude" || norm_str == "MAGNITUDE") {
        config.normalization = Qn::QVector::Normalization::MAGNITUDE;
      } else if (norm_str == "sqrt_m" || norm_str == "SQRT_M") {
        config.normalization = Qn::QVector::Normalization::SQRT_M;
      } else {
        /* todo notify */
        return false;
      }

      config.harmonics = node["harmonics"].as<std::string>(config.type == EQVectorType::CHANNEL? "1" : "11");
      config.corrections =
          node["corrections"].as<std::vector<QVectorCorrectionConfig>>(EmptyVector<QVectorCorrectionConfig>());

      for (auto &node_element : node) {
        const std::regex re_qa("^qa(-.+)?$");
        auto node_name = node_element.first.Scalar();
        if (!std::regex_match(node_name, re_qa)) continue;

        auto qa = node_element.second.as<std::vector<HistogramConfig>>();
        std::move(qa.begin(), qa.end(), std::back_inserter(config.qa));
      }

      if (config.type == EQVectorType::TRACK) {
        /* axes */
        config.axes = node["axes"].as<std::vector<AxisConfig>>(EmptyVector<AxisConfig>());
        /* cuts */
        for (auto &node_element : node) {
          const std::regex re_cuts("^cuts(-.+)?$");
          auto node_name = node_element.first.Scalar();
          if (!std::regex_match(node_name, re_cuts)) continue;
          auto cuts_list = node_element.second.as<CutListConfig>();
          move(begin(cuts_list.cuts), end(cuts_list.cuts), back_inserter(config.cuts));
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
  static bool decode(const Node &node, Qn::Analysis::Base::AnalysisSetupConfig &config) {
    using namespace Qn::Analysis::Base;
    using namespace Qn::Analysis::Config::Utils;
    if (node.IsMap()) {
      config.event_axes = node["axes"].as<std::vector<AxisConfig>>(EmptyVector<AxisConfig>());
      config.event_variables = node["event-variables"].as<std::vector<VariableConfig>>(EmptyVector<VariableConfig>());
      config.q_vectors = node["q-vectors"].as<std::vector<QVectorConfig>>();

      for (auto &node_element : node) {
        const std::regex re_qa("^qa(-.+)?$");
        auto node_name = node_element.first.Scalar();
        if (!std::regex_match(node_name, re_qa)) continue;

        auto qa = node_element.second.as<std::vector<HistogramConfig>>();
        std::move(qa.begin(), qa.end(), std::back_inserter(config.qa));
      }
      return true;
    }// IsMap

    return false;
  }
};

}// namespace YAML

#endif//FLOW_SRC_CONFIG_CONFIG_H
