//
// Created by eugene on 02/10/2020.
//

#include "Config.hpp"
#include <gtest/gtest.h>

namespace {

using namespace YAML;
using namespace Qn::Analysis::Correlate;

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

}
