//
// Created by eugene on 03/02/2021.
//

#ifndef QNANALYSIS_SRC_QNANALYSISCONFIG_YAMLUTILS_HPP_
#define QNANALYSIS_SRC_QNANALYSISCONFIG_YAMLUTILS_HPP_

#include <yaml-cpp/yaml.h>

namespace Qn::Analysis::Config::YAML::Utils {

inline const ::YAML::Node &CNode(const ::YAML::Node &n) {
  return n;
}

/**
 * @brief Recursively merges two YAML::Nodes
 * adapted from: https://stackoverflow.com/a/41337824
 * keys from 'rhs' have more priority than from 'lhs' argument
 * @param lhs
 * @param rhs
 * @return
 */
inline
::YAML::Node
MergeRecursive(const ::YAML::Node &lhs, const ::YAML::Node &rhs) {
  using ::YAML::Node;
  using ::YAML::NodeType;

  if (!rhs.IsMap()) {
    // If 'rhs' is not a map, merge result is 'rhs', unless 'rhs' is null
    return rhs.IsNull() ? lhs : rhs;
  }
  if (!lhs.IsMap()) {
    // If 'lhs' is not a map, merge result is 'rhs'
    return rhs;
  }
  if (!rhs.size()) {
    // If 'lhs' is a map, and 'rhs' is an empty map, return 'lhs'
    return lhs;
  }
  // Create a new map 'result' with the same mappings as 'lhs', merged with 'rhs'
  auto result = Node(NodeType::Map);
  for (auto l_c : lhs) {
    if (l_c.first.IsScalar()) {
      const std::string &key = l_c.first.Scalar();
      auto r_c = Node(CNode(rhs)[key]);
      if (r_c) {
        result[l_c.first] = MergeRecursive(l_c.second, r_c);
        continue;
      }
    }
    // move node as-is
    result[l_c.first] = l_c.second;
  }
  // Add the mappings from 'rhs' not already in 'result'
  for (auto r_c : rhs) {
    if (
        !r_c.first.IsScalar() ||
            !CNode(result)[r_c.first.Scalar()]) {
      result[r_c.first] = r_c.second;
    }
  }
  return result;
}

inline
::YAML::Node
Merge(const ::YAML::Node &lhs, const ::YAML::Node &rhs) {
  using ::YAML::Clone;
  using ::YAML::Node;
  using ::YAML::NodeType;

  auto result = Node(NodeType::Map);

  for (auto &l_c: lhs) {
    if (l_c.first.IsScalar()) {
      const std::string &key = l_c.first.Scalar();
      auto r_c = Node(CNode(rhs)[key]);
      if (r_c) {
        result[l_c.first] = Clone(r_c);
        continue;
      }
    }
    // move node as-is
    result[l_c.first] = l_c.second;
  }

  for (auto &r_c : rhs) {
    if (
        !r_c.first.IsScalar() ||
            !CNode(result)[r_c.first.Scalar()]) {
      result[r_c.first] = r_c.second;
    }
  }
  return result;
}

inline
::YAML::Node
ExpandInheritance(const ::YAML::Node &node) {
  using ::YAML::Node;
  using ::YAML::NodeType;


  if (node.IsNull()) {
    return node;
  }

  if (node.IsScalar()) {
    return ::YAML::Clone(node);
  }

  auto result = Node(node.Type());

  if (node.IsSequence()) {
    for (auto &child : node) {
      result.push_back(ExpandInheritance(child));
    }

    return result;
  }

  // Maps
  for (auto &child : node) {
    if (child.first.IsScalar() && child.first.Scalar() == "_from") {
      continue;
    }
    result[::YAML::Clone(child.first)] = ExpandInheritance(child.second);
  }

  if (node["_from"].IsDefined()) {
    auto from_node = ExpandInheritance(node["_from"]);
    if (from_node.IsMap()) {
      return Merge(from_node, result);
    }
  }

  return result;
}

}

#endif //QNANALYSIS_SRC_QNANALYSISCONFIG_YAMLUTILS_HPP_
