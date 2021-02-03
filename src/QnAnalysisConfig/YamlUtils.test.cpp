//
// Created by eugene on 03/02/2021.
//

#include <yaml-cpp/yaml.h>
#include "YamlUtils.hpp"
#include <gtest/gtest.h>

using ::YAML::Load;
using Qn::Analysis::Config::YAML::Utils::MergeRecursive;
using Qn::Analysis::Config::YAML::Utils::ExpandInheritance;

TEST(YamlUtils, MergeRecursive) {

  /* merge two maps */
  {
    auto c1 = Load(R"YAML(
key1: value1
    )YAML");

    auto c2 = Load(R"YAML(
key2: value2
    )YAML");

    auto merge_result = MergeRecursive(c1, c2);
    std::cout << merge_result << std::endl;
  }

  /* override value */
  {
    auto c1 = Load(R"YAML(
key1: value1
    )YAML");

    auto c2 = Load(R"YAML(
key1: new_value1
    )YAML");

    auto merge_result = MergeRecursive(c1, c2);
    std::cout << std::endl;
    std::cout << merge_result << std::endl;
  }


  /* merge nested */
  {
    auto c1 = Load(R"YAML(
nested: { key1: value1 }
    )YAML");

    auto c2 = Load(R"YAML(
nested: { key2: value2 }
    )YAML");

    auto merge_result = MergeRecursive(c1, c2);
    std::cout << std::endl;
    std::cout << merge_result << std::endl;
  }

}

TEST(YamlUtils, ExpandInheritance) {
  {
    auto node = Load(R"YAML(
_objs:
  - &base
    base_key1: value1
    base_key2: value2
  - &derived
    _from: *base
    derived_key1: value1
    base_key2: new_value2
)YAML");

    auto expand_result = ExpandInheritance(node["_objs"]);
    std::cout << std::endl;
    std::cout << expand_result << std::endl;
  }

}


