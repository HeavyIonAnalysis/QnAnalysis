//
// Created by eugene on 04/10/2020.
//

#include <gtest/gtest.h>
#include "Utils.hpp"

namespace {

using namespace Qn::Analysis::Correlate::Details;

TEST(Utils, CombineDynamic) {

  std::vector<std::vector<int>> test{
      {1,2,3},
      {1,2},
      {1,2,3,4}
  }; /// shape = (3,2,4); size = 24

  std::vector<std::vector<int>> result;
  CombineDynamic(test.begin(), test.end(), std::back_inserter(result));
  ASSERT_EQ(result.size(), 24);
  ASSERT_EQ(result.front(), std::vector<int>({1,1,1}));
  ASSERT_EQ(result.back(), std::vector<int>({3,2,4}));




}






}

