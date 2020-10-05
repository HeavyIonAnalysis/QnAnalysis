//
// Created by eugene on 04/10/2020.
//

#include <gtest/gtest.h>
#include "Utils.hpp"

namespace {

using namespace Qn::Analysis::Correlate::Utils;

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

TEST(Utils, Combine) {


  std::vector<std::tuple<int,int,int>> result;
  Combine(std::back_inserter(result), std::vector<int>({1,2,3}), std::vector<int>({1,2}), std::vector<int>({1,2,3,4}));
  ASSERT_EQ(result.size(), 24);
  ASSERT_EQ(result.front(), std::make_tuple(1,1,1));
  ASSERT_EQ(result.back(), std::make_tuple(1,1,1));

}





}

