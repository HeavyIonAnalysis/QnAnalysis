//
// Created by eugene on 22/12/2021.
//

#include <gtest/gtest.h>

#include "Correlation.hpp"


TEST(Correlation, Basics) {
  using namespace ::C4::Correlations;

  auto q1 = q("psd1_RECENTERED", 1, EComponent::X);
  auto q2 = q("psd2_RECENTERED", 1, EComponent::X);
  auto q3 = q("psd3_RECENTERED", 1, EComponent::X);
  auto protons = q("protons_RESCALED", 1, EComponent::X);

  auto r1 = C(protons, q1)*sqrt(Value(2.0)*C(q1, q2)*C(q1, q3)/C(q2, q3));
  r1.value().Print();
}

TEST(Tensor, Basics) {
  using namespace C4::TensorOps;

  auto t1 = Tensor<std::string>(Enumeration("ref",std::vector<std::string>({"psd1", "psd2", "psd3"})));
  EXPECT_EQ(t1.size(), 3);
  EXPECT_NO_THROW(t1.at({{"ref", 2}}));
  EXPECT_THROW(t1.at({{"ref1", 1}}), std::out_of_range);
  EXPECT_THROW(t1.at({{"ref", 3}}), std::out_of_range);
}

TEST(Tensor, Indexing) {
  using namespace C4::TensorOps;
  auto t1 = Tensor<double>(enumerate("ref", {1.,2.,3.}));
  auto t2 = Tensor<double>(enumerate("component", {10.,20.,30.}));
  auto t3 = Tensor<double>(enumerate("qwerty", {100.,200.,300.}));

  for (TensorLinearIndex li = 0ul; li < t1.size(); ++li) {
    auto ind = t1.getIndex(li);
    auto il = t1.getLinearIndex(ind);
    EXPECT_EQ(li, il);
  }
  auto result = t1*t2*t3;
  for (TensorLinearIndex li = 0ul; li < result.size(); ++li) {
    auto ind = result.getIndex(li);
    auto il = result.getLinearIndex(ind);
    EXPECT_EQ(li, il);
  }

}

TEST(Tensor, BinaryOps) {
  using namespace C4::TensorOps;
  auto t1 = Tensor<double>(enumerate("ref", {1.,2.,3.}));
  auto t2 = Tensor<double>(enumerate("component", {10.,20.}));
  double t3 = 11;
  auto result = t1 * t2 / t3;
  EXPECT_EQ(result.size(), t1.size()*t2.size());
  EXPECT_EQ(result.getAxNames(), std::vector<std::string>({"component", "ref"}));

  auto result1 = t3 * t1 / t2;
  EXPECT_EQ(result1.size(), t1.size()*t2.size());
  EXPECT_EQ(result1.getAxNames(), std::vector<std::string>({"component", "ref"}));
}
