#ifndef FLOW_SRC_CORRELATE_FUNCTIONS_H_
#define FLOW_SRC_CORRELATE_FUNCTIONS_H_

#include <QnTools/QVector.hpp>

namespace Qn{

// 2 particle correlations

[[maybe_unused]] static auto XY = [](const Qn::QVector &a, const Qn::QVector &b) {
  return 2 * a.x(1) * b.y(1);
};
[[maybe_unused]] static auto YX = [](const Qn::QVector &a, const Qn::QVector &b) {
  return 2 * a.y(1) * b.x(1);
};
[[maybe_unused]] static auto XX = [](const Qn::QVector &a, const Qn::QVector &b) {
  return 2 * a.x(1) * b.x(1);
};
[[maybe_unused]] static auto YY = [](const Qn::QVector &a, const Qn::QVector &b) {
  return 2 * a.y(1) * b.y(1);
};
[[maybe_unused]] static auto scalar = [](const Qn::QVector &a, const Qn::QVector &b) {
  return Qn::ScalarProduct(a, b, 2);
};

// event plane first harmonic <Q1, Q1>
[[maybe_unused]] static auto XX_EP = [](const Qn::QVector &a, const Qn::QVector &b) {
  return 2 * a.x(1) * b.x(1) / (a.mag(1) * b.mag(1));
};
[[maybe_unused]] static auto YY_EP = [](const Qn::QVector &a, const Qn::QVector &b) {
  return 2 * a.y(1) * b.y(1) / (a.mag(1) * b.mag(1));;
};
[[maybe_unused]] static auto XY_EP = [](const Qn::QVector &a, const Qn::QVector &b) {
  return 2 * a.x(1) * b.y(1) / (a.mag(1) * b.mag(1));
};
[[maybe_unused]] static auto YX_EP = [](const Qn::QVector &a, const Qn::QVector &b) {
  return 2 * a.y(1) * b.x(1) / (a.mag(1) * b.mag(1));;
};

// event plane first harmonic <u1, Q1>
[[maybe_unused]] static auto xX_EP = [](const Qn::QVector &a, const Qn::QVector &b) {
  return 2 * a.x(1) * b.x(1) / (b.mag(1));
};
[[maybe_unused]] static auto yY_EP = [](const Qn::QVector &a, const Qn::QVector &b) {
  return 2 * a.y(1) * b.y(1) / (b.mag(1));;
};
[[maybe_unused]] static auto xY_EP = [](const Qn::QVector &a, const Qn::QVector &b) {
  return 2 * a.x(1) * b.y(1) / (b.mag(1));
};
[[maybe_unused]] static auto yX_EP = [](const Qn::QVector &a, const Qn::QVector &b) {
  return 2 * a.y(1) * b.x(1) / (b.mag(1));;
};

// 3 particle correlations

[[maybe_unused]] static auto Y2XY = [](const Qn::QVector &a, const Qn::QVector &b, const Qn::QVector &c) {
  return a.y(2) * b.x(1) * c.y(1);
};
[[maybe_unused]] static auto Y2YX = [](const Qn::QVector &a, const Qn::QVector &b, const Qn::QVector &c) {
  return a.y(2) * b.y(1) * c.x(1);
};
[[maybe_unused]] static auto X2XX = [](const Qn::QVector &a, const Qn::QVector &b, const Qn::QVector &c) {
  return a.x(2) * b.x(1) * c.x(1);
};
[[maybe_unused]] static auto X2YY = [](const Qn::QVector &a, const Qn::QVector &b, const Qn::QVector &c) {
  return a.x(2) * b.y(1) * c.y(1);
};

[[maybe_unused]] static auto X2XY = [](const Qn::QVector &a, const Qn::QVector &b, const Qn::QVector &c) {
  return a.x(2) * b.x(1) * c.y(1);
};
[[maybe_unused]] static auto X2YX = [](const Qn::QVector &a, const Qn::QVector &b, const Qn::QVector &c) {
  return a.x(2) * b.y(1) * c.x(1);
};
[[maybe_unused]] static auto Y2XX = [](const Qn::QVector &a, const Qn::QVector &b, const Qn::QVector &c) {
  return a.y(2) * b.x(1) * c.x(1);
};
[[maybe_unused]] static auto Y2YY = [](const Qn::QVector &a, const Qn::QVector &b, const Qn::QVector &c) {
  return a.y(2) * b.y(1) * c.y(1);
};

// 4 particle correlations

[[maybe_unused]] static auto Y3YYY = [](const Qn::QVector &a, const Qn::QVector &b, const Qn::QVector &c, const Qn::QVector &d) {
  return a.y(3) * b.y(1) * c.y(1) * d.y(1);
};
[[maybe_unused]] static auto Y3XYY = [](const Qn::QVector &a, const Qn::QVector &b, const Qn::QVector &c, const Qn::QVector &d) {
  return a.y(3) * b.x(1) * c.y(1) * d.y(1);
};
[[maybe_unused]] static auto Y3YXY = [](const Qn::QVector &a, const Qn::QVector &b, const Qn::QVector &c, const Qn::QVector &d) {
  return a.y(3) * b.y(1) * c.x(1) * d.y(1);
};
[[maybe_unused]] static auto Y3YYX = [](const Qn::QVector &a, const Qn::QVector &b, const Qn::QVector &c, const Qn::QVector &d) {
  return a.y(3) * b.y(1) * c.y(1) * d.x(1);
};
[[maybe_unused]] static auto Y3YXX = [](const Qn::QVector &a, const Qn::QVector &b, const Qn::QVector &c, const Qn::QVector &d) {
  return a.y(3) * b.y(1) * c.x(1) * d.x(1);
};
[[maybe_unused]] static auto Y3XYX = [](const Qn::QVector &a, const Qn::QVector &b, const Qn::QVector &c, const Qn::QVector &d) {
  return a.y(3) * b.x(1) * c.y(1) * d.x(1);
};
[[maybe_unused]] static auto Y3XXY = [](const Qn::QVector &a, const Qn::QVector &b, const Qn::QVector &c, const Qn::QVector &d) {
  return a.y(3) * b.x(1) * c.x(1) * d.y(1);
};
[[maybe_unused]] static auto Y3XXX = [](const Qn::QVector &a, const Qn::QVector &b, const Qn::QVector &c, const Qn::QVector &d) {
  return a.y(3) * b.x(1) * c.x(1) * d.x(1);
};
[[maybe_unused]] static auto X3YYY = [](const Qn::QVector &a, const Qn::QVector &b, const Qn::QVector &c, const Qn::QVector &d) {
  return a.x(3) * b.y(1) * c.y(1) * d.y(1);
};
[[maybe_unused]] static auto X3XYY = [](const Qn::QVector &a, const Qn::QVector &b, const Qn::QVector &c, const Qn::QVector &d) {
  return a.x(3) * b.x(1) * c.y(1) * d.y(1);
};
[[maybe_unused]] static auto X3YXY = [](const Qn::QVector &a, const Qn::QVector &b, const Qn::QVector &c, const Qn::QVector &d) {
  return a.x(3) * b.y(1) * c.x(1) * d.y(1);
};
[[maybe_unused]] static auto X3YYX = [](const Qn::QVector &a, const Qn::QVector &b, const Qn::QVector &c, const Qn::QVector &d) {
  return a.x(3) * b.y(1) * c.y(1) * d.x(1);
};
[[maybe_unused]] static auto X3YXX = [](const Qn::QVector &a, const Qn::QVector &b, const Qn::QVector &c, const Qn::QVector &d) {
  return a.x(3) * b.y(1) * c.x(1) * d.x(1);
};
[[maybe_unused]] static auto X3XYX = [](const Qn::QVector &a, const Qn::QVector &b, const Qn::QVector &c, const Qn::QVector &d) {
  return a.x(3) * b.x(1) * c.y(1) * d.x(1);
};
[[maybe_unused]] static auto X3XXY = [](const Qn::QVector &a, const Qn::QVector &b, const Qn::QVector &c, const Qn::QVector &d) {
  return a.x(3) * b.x(1) * c.x(1) * d.y(1);
};
[[maybe_unused]] static auto X3XXX = [](const Qn::QVector &a, const Qn::QVector &b, const Qn::QVector &c, const Qn::QVector &d) {
  return a.x(3) * b.x(1) * c.x(1) * d.x(1);
};



}


#endif //FLOW_SRC_CORRELATE_FUNCTIONS_H_
