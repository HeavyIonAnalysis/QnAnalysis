//
// Created by eugene on 08/10/2020.
//

#include "StaticRegistry.h"
#include "UserCorrelationAction.hpp"
#include <gtest/gtest.h>
#include <QnTools/QVector.hpp>

namespace {

using namespace Qn::Analysis::Correlate::Action;
using namespace Qn::Analysis::Correlate::Action::Details;
using namespace Qn::Analysis::Correlate;

TEST(CorrelationAction, WrapFunction) {

  EXPECT_EQ(WrapFunction(&ScalarProduct<1>).ARITY, 2);
  EXPECT_EQ(WrapFunction([] (Qn::QVector qv1) { return 1.; }).ARITY, 1);

  Register(std::string("scalar_product"), WrapFunction(ScalarProduct<1>));
  Register(std::string("test"), WrapFunction([] (Qn::QVector qv) { return 1.;}));
  EXPECT_NO_THROW(GetActionRegistry<1>().Get("test"));
  EXPECT_THROW(GetActionRegistry<2>().Get("test1"), std::out_of_range);

}

TEST(CorrelationAction, Builder) {

  const auto xx = Builder<Qx1,Qx1>::F;
  const auto yy = Builder<Qy1,Qy1>::F;
  const auto xy = Builder<Qx1,Qy1>::F;
  Qn::QVector v1;
  v1.ActivateHarmonic(1);
  v1.SetX(1, 1.);
  v1.SetY(1, 2.);
  Qn::QVector v2;
  v2.ActivateHarmonic(1);
  v2.SetX(1, 3.);
  v2.SetY(1, 4.);
  EXPECT_EQ(xx(v1,v2), 1*3);
  EXPECT_EQ(yy(v1,v2), 2*4);
  EXPECT_EQ(xy(v1,v2), 1*4);

}

TEST(CorrelationAction, PredefinedActions) {
  EXPECT_NO_THROW(GetActionRegistry<2>().Get("xx"));
  EXPECT_NO_THROW(GetActionRegistry<2>().Get("xy"));
  EXPECT_NO_THROW(GetActionRegistry<2>().Get("yx"));
  EXPECT_NO_THROW(GetActionRegistry<2>().Get("yy"));
}

}

