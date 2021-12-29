//
// Created by eugene on 22/12/2021.
//

#include <gtest/gtest.h>

#include "Correlation.hpp"

using namespace std;

TEST(Correlation, Basics) {

  using namespace ::C4::CorrelationOps;

  auto q1 = q("psd1_RECENTERED", 1, EComponent::X);
  auto q2 = q("psd2_RECENTERED", 1, EComponent::X);
  auto q3 = q("psd3_RECENTERED", 1, EComponent::X);
  auto protons = q("protons_RESCALED", 1, EComponent::X);

  auto r1 = c(protons, q1) * sqrt(Value(2.0) * c(q1, q2) * c(q1, q3) / c(q2, q3));
  r1.value().Print();
}

TEST(Tensor, Basics) {
  using namespace C4::TensorOps;

  auto t1 = enumerate("ref", {"psd1", "psd2", "psd3"}).tensor();
  EXPECT_EQ(t1.size(), 3);
  EXPECT_NO_THROW(t1.at({{"ref", 2}}));
  EXPECT_THROW(t1.at({{"ref1", 1}}), std::out_of_range);
  EXPECT_THROW(t1.at({{"ref", 3}}), std::out_of_range);
}
TEST(Tensor, Indexing) {
  using namespace C4::TensorOps;
  auto t1 = enumerate("ref", {1., 2., 3.}).tensor();
  auto t2 = enumerate("component", {10., 20., 30.}).tensor();
  auto t3 = enumerate("qwerty", {100., 200., 300.}).tensor();

  for (TensorLinearIndex li = 0ul; li < t1.size(); ++li) {
    auto ind = t1.getIndex(li);
    auto il = t1.getLinearIndex(ind);
    EXPECT_EQ(li, il);
  }
  auto result = t1 * t2 * t3;
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

auto r1_3sub(
    ::C4::TensorOps::Enumeration<const char *> ref1,
    ::C4::TensorOps::Enumeration<::C4::CorrelationOps::EComponent> comp) {
  using namespace C4::TensorOps;
  using namespace C4::CorrelationOps;

  auto ref2 = ref1.clone("r2");
  auto ref3 = ref1.clone("r3");

  auto r1t = Value(2.) * ct(qt(ref1, 1, comp), qt(ref2, 1, comp)) *
      ct(qt(ref1, 1, comp), qt(ref3, 1, comp)) / ct(qt(ref2, 1, comp), qt(ref3, 1, comp));

  return Tensor{
      {
          {comp.getName(), comp.size()},
          {ref1.getName(), ref1.size()}}, [=](const TensorIndex &ind) {
        auto component_id = ind.at(comp.getName());
        auto ref_id = ind.at(ref1.getName());

        return r1t.at({
                          {comp.getName(), component_id},
                          {ref1.getName(), ref_id},
                          {ref2.getName(), ref_id == 0 ? 1 : (ref_id == 1 ? 2 : 0)},
                          {ref3.getName(), ref_id == 0 ? 2 : (ref_id == 1 ? 0 : 1)}
                      });
      }};
}

TEST(Tensor, Tensorize) {
  using namespace ::C4::TensorOps;
  using namespace ::C4::CorrelationOps;

  auto en_obs = enumerate("obs", {"protons_RESCALED", "pion_neg_RESCALED", "pion_pos_RESCALED"});
  auto en_comp = enumerate("component", {EComponent::X, EComponent::Y});
  auto en_comp_inv = enumerate("component", {EComponent::Y, EComponent::X});
  auto en_ref = enumerate("ref", {"psd1_RECENTERED", "psd2_RECENTERED", "psd3_RECENTERED"});

  auto resolution_psi = ct(qt(en_ref, 1, en_comp), qt("psi_RP", 1, en_comp));

  auto resolution_3sub = r1_3sub(en_ref, en_comp);
  auto obs_v1 = Value(2.) * ct(qt(en_obs, 1, en_comp), qt(en_ref, 1, en_comp)) / resolution_3sub;
  auto obs_c1 = Value(2.) * ct(qt(en_obs, 1, en_comp), qt(en_ref, 1, en_comp_inv)) / resolution_3sub;

  EXPECT_EQ(obs_v1.size(), 18);
  EXPECT_EQ(obs_c1.size(), 18);

  for (TensorLinearIndex li = 0ul; li < obs_v1.size(); ++li) {
    auto index = obs_v1.getIndex(li);
    auto correlation = obs_v1.at(li);
    cout << correlation << endl;
  }

}

TEST(Tensor, v2) {
  using namespace ::C4::TensorOps;
  using namespace ::C4::CorrelationOps;

  auto en_obs = enumerate("obs", {"protons_RESCALED", "pion_neg_RESCALED", "pion_pos_RESCALED"});
  auto en_comp0 = enumerate("comp0", {EComponent::X, EComponent::Y});
  auto en_comp1 = en_comp0.clone("comp1");
  auto en_comp2 = en_comp0.clone("comp2");

  auto en_ref1 = enumerate("ref1", {"psd1_RECENTERED", "psd2_RECENTERED", "psd3_RECENTERED"});
  auto en_ref2 = en_ref1.clone("ref2");

  auto r1_ref1 = r1_3sub(en_ref1, en_comp1);
  auto r1_ref2 = r1_3sub(en_ref2, en_comp2);

  auto v2 = ct(
      qt(en_obs, 2, en_comp0),
      qt(en_ref1, 1, en_comp1),
      qt(en_ref2, 1, en_comp2)) / r1_ref1 / r1_ref2;

  auto v2_xy = ct(
      qt(en_obs, 2, en_comp0),
      qt(en_ref1, 1, en_comp1),
      qt(en_ref2, 1, en_comp2)
  ) / ct(
      qt(en_ref1, 1, en_comp1),
      qt(en_ref2, 1, en_comp2));

  for (TensorLinearIndex li = 0ul; li < v2_xy.size(); ++li) {
    auto comp0 = en_comp0.at(v2_xy.getIndex(li));
    auto obs = en_obs.at(v2_xy.getIndex(li));
    cout << v2_xy.at(li) << endl;
    v2_xy.at(li).value();
    cout << endl;
  }

  auto value = v2_xy.at(makeIndex(
      en_obs.index("protons_RESCALED"),
      en_comp0.index(EComponent::Y),
      en_comp1.index(EComponent::X),
      en_comp2.index(EComponent::Y),
      en_ref1.index("psd1_RECENTERED"),
      en_ref2.index("psd2_RECENTERED")
      ));
  cout << value;
  cout << endl;

}

TEST(Tensor, BinaryOps) {
  using namespace C4::TensorOps;
  auto t1 = enumerate("ref", {1., 2., 3.}).tensor();
  auto t2 = enumerate("component", {10., 20.}).tensor();
  double t3 = 11;
  auto result = 1.0/t3 * t1 * t2;
  EXPECT_EQ(result.size(), t1.size() * t2.size());
  EXPECT_EQ(result.getAxNames(), std::vector<std::string>({"component", "ref"}));

  auto result1 = t3 * t1 / t2;
  EXPECT_EQ(result1.size(), t1.size() * t2.size());
  EXPECT_EQ(result1.getAxNames(), std::vector<std::string>({"component", "ref"}));
}

TEST(Tensor, Resolution) {
  using namespace C4::TensorOps;
  using namespace C4::CorrelationOps;

  auto r1 = r1_3sub(
      enumerate("ref", {"psd1", "psd2", "psd3"}),
      enumerate("comp", {EComponent::X, EComponent::Y}));
  for (TensorLinearIndex li = 0ul; li < r1.size(); ++li) {
    cout << r1.at(li) << endl;
  }

}
