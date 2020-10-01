//
// Created by eugene on 30/07/2020.
//

#ifndef DATATREEFLOW_SRC_CONFIG_UTILS_H
#define DATATREEFLOW_SRC_CONFIG_UTILS_H

#include <cassert>
#include <regex>
#include <algorithm>

#include <yaml-cpp/yaml.h>

namespace config {
namespace utils {

enum EStringMatchingPolicy { CASE_SENSITIVE, CASE_INSENSITIVE };

struct QueryPredicate {
  enum EPredicateType { EXACT_MATCH, IN, REGEX };

  enum ESequencePolicy { ANY, ALL };


  /* either scalar or sequence of scalars */
  std::string target_field;
  EPredicateType predicate_type{EPredicateType::EXACT_MATCH};
  ESequencePolicy sequence_policy{ESequencePolicy::ALL};
  EStringMatchingPolicy string_matching_policy{
      EStringMatchingPolicy::CASE_SENSITIVE};

  /* EXACT_MATCH options */
  std::string match_value;

  /* IN options */
  std::vector<std::string> match_values{};

  /* REGEX options */
  std::string regex_string;
};

struct Query {
  bool is_empty{true};
  std::vector<QueryPredicate> predicates;
};

template <typename T>
struct QueryResult {
  Query query;
  std::vector<T> query_result;
};

namespace Details {

static inline std::string ToLower(const std::string &orig) {
  std::string result = orig;
  std::transform(orig.begin(), orig.end(), result.begin(), [] (int c) { return std::tolower(c); });
  return result;
}

static inline bool IsSequenceOfScalars(const YAML::Node& node) {
  return node.IsSequence() &&
         std::all_of(node.begin(), node.end(),
                     [](const YAML::Node& node) { return node.IsScalar(); });
}

static inline bool CompareStrings(const std::string& str1, const std::string& str2,
                           EStringMatchingPolicy policy) {
  if (policy == EStringMatchingPolicy::CASE_SENSITIVE) {
    return str1 == str2;
  } else if (policy == EStringMatchingPolicy::CASE_INSENSITIVE) {
    return ToLower(str1) == ToLower(str2);
  }

  assert(false);
}

static inline bool TestPredicateScalar(const QueryPredicate& predicate,
                                const YAML::Node& test_value_node) {
  auto test_value = test_value_node.Scalar();
  if (predicate.predicate_type == QueryPredicate::EXACT_MATCH) {
    return CompareStrings(test_value, predicate.match_value,
                          predicate.string_matching_policy);
  } else if (predicate.predicate_type == QueryPredicate::IN) {
    return std::any_of(
        predicate.match_values.begin(), predicate.match_values.end(),
        [&test_value, &predicate](const std::string& predicate_value) {
          return CompareStrings(predicate_value, test_value,
                                predicate.string_matching_policy);
        });
  } else if (predicate.predicate_type == QueryPredicate::REGEX) {
    std::regex re(predicate.regex_string);
    return std::regex_match(test_value, re);
  }

  return false;
}

static inline bool TestPredicateSequence(const QueryPredicate& predicate, const YAML::Node& test_sequence_node) {
  if (predicate.sequence_policy == QueryPredicate::ESequencePolicy::ALL) {
    return std::all_of(test_sequence_node.begin(), test_sequence_node.end(),
                       [&predicate] (const YAML::Node& scalar_node) { return TestPredicateScalar(predicate, scalar_node); });
  } else if (predicate.sequence_policy == QueryPredicate::ESequencePolicy::ANY) {
    return std::any_of(test_sequence_node.begin(), test_sequence_node.end(),
                       [&predicate] (const YAML::Node& scalar_node) { return TestPredicateScalar(predicate, scalar_node); });
  }

  return false;
}


static inline bool TestPredicate(const QueryPredicate& predicate,
                           const YAML::Node& node) {
  if (!node) return false;  // TODO Warning?

  auto target_field_node = node[predicate.target_field];
  if (!target_field_node) return false;  // TODO Warning?

  if (target_field_node.IsScalar()) {
    return TestPredicateScalar(predicate, target_field_node);
  } else if (IsSequenceOfScalars(target_field_node)) {
    return TestPredicateSequence(predicate, target_field_node);
  }

  return false;
}

}  // namespace details

template <typename T>
static QueryResult<T> ProcessQuery(const Query& query, const YAML::Node& node,
                         const std::vector<std::string>& allowed_fields) {

  QueryResult<T> result;
  result.query = query;
  auto &query_result = result.query_result;

  for (auto && element : node) {
    auto is_ok = std::all_of(query.predicates.begin(), query.predicates.end(), [&element] (const QueryPredicate& predicate) {
      return Details::TestPredicate(predicate, element);
    });
    if (is_ok) {
      query_result.emplace_back(std::move(element.as<T>()));
    }
  }

  return result;
}

}  // namespace utils
}  // namespace config

namespace YAML {

template <>
struct convert<config::utils::QueryPredicate> {
  static bool decode(const Node& node,
                     config::utils::QueryPredicate& predicate) {
    predicate.target_field = node["target"].as<std::string>("");

    // TODO matching policies

    if (node["is"]) {
      predicate.predicate_type = config::utils::QueryPredicate::EXACT_MATCH;
      predicate.match_value = node["is"].as<std::string>();
      return true;
    }

    if (node["is-in"]) {
      predicate.predicate_type = config::utils::QueryPredicate::IN;
      predicate.match_values = node["is-in"].as<std::vector<std::string>>();
      return true;
    }

    if (node["regex"]) {
      predicate.predicate_type = config::utils::QueryPredicate::REGEX;
      predicate.regex_string = node["regex"].as<std::string>();
      return true;
    }

    return false;
  }
};

template <>
struct convert<config::utils::Query> {

  static bool decode(const Node& node, config::utils::Query& query) {
    if (node.IsSequence()) {
      query.predicates = node.as<std::vector<config::utils::QueryPredicate>>();
      query.is_empty = false;
      return true;
    } else if (node.IsMap()) {
      for (auto&& element : node) {
        auto target_field = element.first.as<std::string>();
        auto predicate = element.second.as<config::utils::QueryPredicate>();
        predicate.target_field = target_field;
        query.predicates.emplace_back(std::move(predicate));
      }

      query.is_empty = false;
      return true;
    }

    return false;
  }
};

}  // namespace YAML

#endif  // DATATREEFLOW_SRC_CONFIG_UTILS_H
