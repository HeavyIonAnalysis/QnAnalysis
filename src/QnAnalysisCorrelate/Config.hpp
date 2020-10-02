//
// Created by eugene on 02/10/2020.
//

#ifndef QNANALYSIS_SRC_QNANALYSISCORRELATE_CONFIG_HPP
#define QNANALYSIS_SRC_QNANALYSISCORRELATE_CONFIG_HPP

#include "enum.h"
#include <algorithm>
#include <iostream>
#include <yaml-cpp/yaml.h>
#include <regex>
#include <cassert>

namespace YAMLHelper {

BETTER_ENUM(EYAMLQueryPredicateType, int, EQUALS, ANY_IN, ALL_IN, REGEX_MATCH);
struct YAMLQueryPredicate {
  std::string target_field;
  EYAMLQueryPredicateType type{EYAMLQueryPredicateType::EQUALS};

  /* EQUALS options: scalars only */
  std::string equals_value;

  /* ANY_IN, ALL_IN options: scalars only */
  std::vector<std::string> in_list;

  /* REGEX_MATCH options: scalars only */
  std::string regex_pattern;
};

struct YAMLSequenceQuery {
  std::vector<YAMLQueryPredicate> predicates;
};

inline bool EvalQueryPredicate(const YAML::Node& node, const YAMLQueryPredicate& predicate) {
  auto target_field_node = node[predicate.target_field];

  if (!target_field_node)
    throw std::runtime_error("Target field '" + predicate.target_field + "' does not exist in node");

  if (predicate.type._value == EYAMLQueryPredicateType::EQUALS) {
    if (!target_field_node.IsScalar())
      throw std::runtime_error("Target field for EQUALS predicate must be scalar");

    return target_field_node.Scalar() == predicate.equals_value;
  } else if (
      predicate.type._value == EYAMLQueryPredicateType::ANY_IN ||
      predicate.type._value == EYAMLQueryPredicateType::ALL_IN
      ) {
    if (predicate.in_list.empty()) {
      throw std::runtime_error("Predicate argument must be not empty");
    }
    if (target_field_node.IsScalar()) {
      return std::find(predicate.in_list.cbegin(), predicate.in_list.cend(), target_field_node.Scalar()) != predicate.in_list.cend();
    } else if (target_field_node.IsSequence()) {
      if (target_field_node.size() == 0) {
        return false;
      }
      std::vector<std::string> predicate_list(predicate.in_list.begin(), predicate.in_list.end());
      std::vector<std::string> target_seq_scalars(target_field_node.size());
      std::vector<std::string> intersection;
      std::transform(target_field_node.begin(), target_field_node.end(),
                     target_seq_scalars.begin(), [] (const YAML::Node& n) { return n.Scalar(); });
      std::sort(target_seq_scalars.begin(), target_seq_scalars.end());
      std::sort(predicate_list.begin(), predicate_list.end());
      std::set_intersection(target_seq_scalars.begin(), target_seq_scalars.end(),
                            predicate.in_list.begin(), predicate.in_list.end(),
                            std::back_inserter(intersection));
      if (predicate.type._value == EYAMLQueryPredicateType::ALL_IN) {
        return intersection.size() == target_seq_scalars.size();
      } else {
        return !intersection.empty();
      }
    }
  } else if (predicate.type._value == EYAMLQueryPredicateType::REGEX_MATCH) {
    std::regex re(predicate.regex_pattern);
    if (!target_field_node.IsScalar()) {
      throw std::runtime_error("Target field for REGEX_MATCH predicate must be scalar");
    }
    return std::regex_match(target_field_node.Scalar(), re);
  }

  assert(false);
  __builtin_unreachable();
}

inline YAML::Node QuerySequence(const YAML::Node& node, const YAMLSequenceQuery& query) {
  using namespace YAML;
  Node result;

  if (!node.IsSequence())
    throw std::runtime_error("Expected sequence node");

  for (auto &seq_entry : node) {
    bool entry_ok = true;
    for (auto &predicate : query.predicates) {
      entry_ok = entry_ok & EvalQueryPredicate(seq_entry, predicate);
      if (!entry_ok) break;
    }
    if (entry_ok) {
      result.push_back(seq_entry);
    }
  }

  return result;
}

}

namespace Qn::Analysis::Correlate {

BETTER_ENUM(EQnWeight, int, OBSERVABLE, REFERENCE)
BETTER_ENUM(EQnCorrectionStep, int, PLAIN, RECENTERED, TWIST, RESCALED, ALIGNED)

struct Axis {

};


struct QVectorTagged {
  std::string name;
  std::vector<std::string> tags;
  EQnCorrectionStep correction_step{EQnCorrectionStep::PLAIN};
};

struct CorrelationTaskArgument {
  YAMLHelper::YAMLSequenceQuery query;
  EQnWeight weight;
};

/* list of arguments, list of actions to apply */
struct CorrelationTask {
  std::vector<CorrelationTaskArgument> arguments;
  std::vector<std::string> actions;
};

}

namespace YAML {

/**
 * @brief Wrapper to convert better-enums
 * @tparam T enumerator
 */
template<typename T>
struct Enum {
  std::optional<T> opt_enum;
  /* default constructor is required for decode */
  Enum() = default;
  Enum(const T &v) { opt_enum = v; }
  Enum(typename T::_integral v) { opt_enum = T::_from_integral(v); }
  operator T const &() const {
    return opt_enum.value();
  }
};

/**
 * @brief Universal case-insensitive encoder-decoder for better-enums
 * @tparam T enumerator
 */
template<typename T>
struct convert<Enum<T>> {
  static bool decode(const Node &node, Enum<T> &e) {
    if (node.IsScalar()) {
      if (!T::_is_valid_nocase(node.Scalar().c_str())) {
        return false;
      }
      e.opt_enum = T::_from_string_nocase(node.Scalar().c_str());
      return true;
    }
    return false;
  }

  static Node encode(const Enum<T> &e) {
    Node result;
    result = e.opt_enum.value()._to_string();
    return result;
  }
};

template<>
struct convert<Qn::Analysis::Correlate::QVectorTagged> {

  static bool decode(const Node &node, Qn::Analysis::Correlate::QVectorTagged &qv) {
    using namespace Qn::Analysis::Correlate;
    if (node.IsMap()) {
      qv.name = node["name"].Scalar();
      qv.tags = node["tags"].as<std::vector<std::string>>(std::vector<std::string>{});
      qv.correction_step =
          node["correction-step"].as<Enum<EQnCorrectionStep>>(Enum<EQnCorrectionStep>(EQnCorrectionStep::PLAIN));
      return true;
    }

    return false;
  }

};

template<>
struct convert<YAMLHelper::YAMLQueryPredicate> {
  static bool decode(const Node &node, YAMLHelper::YAMLQueryPredicate &predicate) {
    using namespace YAMLHelper;
    if (node.IsMap()) {
      predicate.target_field = node["target-field"].as<std::string>("");
      if (node["equals"]) {
        predicate.type = EYAMLQueryPredicateType::EQUALS;
        predicate.equals_value = node["equals"].as<std::string>();
      } else if (node["any-in"]) {
        predicate.type = EYAMLQueryPredicateType::ANY_IN;
        predicate.in_list = node["any-in"].as<std::vector<std::string>>();
      } else if (node["all-in"]) {
        predicate.type = EYAMLQueryPredicateType::ALL_IN;
        predicate.in_list = node["all-in"].as<std::vector<std::string>>();
      } else if (node["regex-match"]) {
        predicate.type = EYAMLQueryPredicateType::REGEX_MATCH;
        predicate.regex_pattern = node["regex-match"].as<std::string>();
      }
      return true;
    }
    return false;
  }
};

template<>
struct convert<YAMLHelper::YAMLSequenceQuery> {
  static bool decode(const Node &node, YAMLHelper::YAMLSequenceQuery &qv) {
    using namespace YAMLHelper;
    if (node.IsMap()) {
      for (auto element : node) {
       std::string target_field = element.first.Scalar();
       auto predicate = element.second.as<YAMLQueryPredicate>();
       if (!predicate.target_field.empty()) {
         return false;
       }
       predicate.target_field = target_field;
       qv.predicates.emplace_back(std::move(predicate));
      }
      return true;
    } // IsMap
    else if (node.IsSequence()) {
      qv.predicates = node.as<std::vector<YAMLQueryPredicate>>();
      return true;
    }

    return false;
  }
};

}

#endif //QNANALYSIS_SRC_QNANALYSISCORRELATE_CONFIG_HPP
