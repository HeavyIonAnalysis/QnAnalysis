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

  auto r1 = c(protons, q1)*sqrt(Value(2.0)* c(q1, q2)* c(q1, q3)/ c(q2, q3));
  r1.value().Print();
}

TEST(Tensor, Basics) {
  using namespace C4::TensorOps;

  auto t1 = enumerate("ref",{"psd1", "psd2", "psd3"}).tensor();
  EXPECT_EQ(t1.size(), 3);
  EXPECT_NO_THROW(t1.at({{"ref", 2}}));
  EXPECT_THROW(t1.at({{"ref1", 1}}), std::out_of_range);
  EXPECT_THROW(t1.at({{"ref", 3}}), std::out_of_range);
}

TEST(Tensor, Indexing) {
  using namespace C4::TensorOps;
  auto t1 = enumerate("ref", {1.,2.,3.}).tensor();
  auto t2 = enumerate("component", {10.,20.,30.}).tensor();
  auto t3 = enumerate("qwerty", {100.,200.,300.}).tensor();

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
TEST(Tensor, MergeAxes) {
  using namespace ::C4::TensorOps;

  TensorAxes ax1{{"ax1", 10}};
  TensorAxes ax2{{"ax2", 20}};
  TensorAxes ax3{{"ax3", 30}};
  EXPECT_EQ(mergeAxes(ax1, ax2, ax3).size(), 3);
  EXPECT_EQ(mergeAxes(ax1, ax2, ax1).size(), 2);
}

TEST(Tensor, Tensorize) {
  using namespace ::C4::Correlations;
  using namespace ::C4::TensorOps;

  auto en_obs = enumerate("obs",{"protons_RESCALED", "pion_neg_RESCALED","pion_pos_RESCALED"});
  auto en_comp = enumerate("component", {EComponent::X, EComponent::Y});
  auto en_comp_inv = enumerate("component", {EComponent::Y, EComponent::X});
  auto en_ref = enumerate("ref", {"psd1_RECENTERED", "psd2_RECENTERED", "psd3_RECENTERED"});

  auto obs_v1 = ct(qt(en_obs, 1, en_comp), qt(en_ref, 1, en_comp));
  auto obs_c1 = ct(qt(en_obs, 1, en_comp), qt(en_ref, 1, en_comp_inv));

  EXPECT_EQ(obs_v1.size(), 18);
  EXPECT_EQ(obs_c1.size(), 18);

  for (TensorLinearIndex li = 0ul; li < obs_v1.size(); ++li) {
    auto index = obs_v1.getIndex(li);
    auto correlation = obs_v1.at(li);
    EXPECT_EQ(correlation.q_vectors_.back().name_, en_ref.index_.at(index["ref"]));
    EXPECT_EQ(correlation.q_vectors_.front().name_, en_obs.index_.at(index["obs"]));
  }

  for (TensorLinearIndex li = 0ul; li < obs_c1.size(); ++li) {
    auto correlation = obs_c1.at(li);
    std::cout << correlation.nameInFile() << std::endl;
  }


}

TEST(Tensor, BinaryOps) {
  using namespace C4::TensorOps;
  auto t1 = enumerate("ref", {1.,2.,3.}).tensor();
  auto t2 = enumerate("component", {10.,20.}).tensor();
  double t3 = 11;
  auto result = t1 * t2 / t3;
  EXPECT_EQ(result.size(), t1.size()*t2.size());
  EXPECT_EQ(result.getAxNames(), std::vector<std::string>({"component", "ref"}));

  auto result1 = t3 * t1 / t2;
  EXPECT_EQ(result1.size(), t1.size()*t2.size());
  EXPECT_EQ(result1.getAxNames(), std::vector<std::string>({"component", "ref"}));
}
