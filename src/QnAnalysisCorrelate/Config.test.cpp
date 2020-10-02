//
// Created by eugene on 02/10/2020.
//

#include "Config.hpp"
#include <BuildOptions.hpp>
#include <gtest/gtest.h>

namespace {

using namespace YAML;
using namespace YAMLHelper;
using namespace Qn::Analysis::Correlate;

TEST(Config, QuerySequence) {
  auto node = LoadFile(Qn::Analysis::GetSetupsDir() + "/" + "correlation/example.yml");

  auto q0 = node["_queries"][0].as<YAMLSequenceQuery>();
  EXPECT_EQ(QuerySequence(node["_detectors"], q0).size(), 1);
  std::cout << "Q0" << std::endl << QuerySequence(node["_detectors"], q0) << std::endl;
  auto q1 = node["_queries"][1].as<YAMLSequenceQuery>();
  EXPECT_EQ(QuerySequence(node["_detectors"], q1).size(), 1);
  std::cout <<  "Q1" << std::endl << QuerySequence(node["_detectors"], q1) << std::endl;
  auto q2 = node["_queries"][2].as<YAMLSequenceQuery>();
  EXPECT_EQ(QuerySequence(node["_detectors"], q2).size(), 3);
  std::cout <<  "Q2" << std::endl << QuerySequence(node["_detectors"], q2) << std::endl;
}

TEST(Config, BetterEnumsConversion) {

  Node observable_node("observable");
  EXPECT_EQ(observable_node.as<Enum<EQnWeight>>(), EQnWeight(EQnWeight::OBSERVABLE));
  Node reference_node("reference");
  EXPECT_EQ(reference_node.as<Enum<EQnWeight>>(), EQnWeight(EQnWeight::REFERENCE));

  Node n;
  n = Enum<EQnWeight>(EQnWeight::OBSERVABLE);
  EXPECT_EQ(n.Scalar(), "OBSERVABLE");
  n = Enum<EQnWeight>(EQnWeight::REFERENCE);
  EXPECT_EQ(n.Scalar(), "REFERENCE");

}

TEST(Config, QVectorTagged) {
  auto node = LoadFile(Qn::Analysis::GetSetupsDir() + "/" + "correlation/example.yml");
  EXPECT_NO_THROW(node["_detectors"].as<std::vector<QVectorTagged>>());
}

TEST(Config, YAMLSequenceQuery) {
  auto node = LoadFile(Qn::Analysis::GetSetupsDir() + "/" + "correlation/example.yml");
  EXPECT_NO_THROW(node["_queries"].as<std::vector<YAMLSequenceQuery>>());
}

TEST(Config, CorrelationTaskArgument) {
  auto node = LoadFile(Qn::Analysis::GetSetupsDir() + "/" + "correlation/example.yml");
  EXPECT_NO_THROW(node["_task_args"].as<std::vector<CorrelationTaskArgument>>());

  auto result = node["_task_args"].as<std::vector<CorrelationTaskArgument>>();
  result.size();

}

}
