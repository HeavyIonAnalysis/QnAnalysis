//
// Created by eugene on 22/12/2021.
//

#include <gtest/gtest.h>

#include "Correlation.hpp"

using namespace C4;


TEST(Correlation, Basics) {
  auto q1 = q("psd1_RECENTERED", 1, EComponent::X);
  auto q2 = q("psd2_RECENTERED", 1, EComponent::X);
  auto q3 = q("psd3_RECENTERED", 1, EComponent::X);
  auto protons = q("protons_RESCALED", 1, EComponent::X);

  auto r1 = C(protons, q1)*sqrt(Value(2.0)*C(q1, q2)*C(q1, q3)/C(q2, q3));
  r1.value().Print();
}



