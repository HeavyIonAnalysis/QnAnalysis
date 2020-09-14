#ifndef FLOW_SRC_CORRELATE_UTILS_H_
#define FLOW_SRC_CORRELATE_UTILS_H_

#include <map>
#include <vector>

#include "QnTools/ReSampleHelper.hpp"

#include "Functions.h"
#include "QVector.h"

namespace Qn {

using function2_t = const std::function<double(const Qn::QVector& a, const Qn::QVector& b)>;
using function3_t = const std::function<double(const Qn::QVector& a, const Qn::QVector& b, const Qn::QVector& c)>;
using function4_t = const std::function<double(const Qn::QVector& a, const Qn::QVector& b, const Qn::QVector& c, const Qn::QVector& d)>;

[[maybe_unused]] static std::vector<std::pair<std::string, function2_t>> Q1Q1(bool non_zero_only) {
  if (non_zero_only) {
    return {{"Q1x_Q1x", XX}, {"Q1y_Q1y", YY}};
  } else {
    return {{"Q1x_Q1x", XX}, {"Q1y_Q1y", YY}, {"Q1x_Q1y", XY}, {"Q1y_Q1x", YX}};
  }
}
[[maybe_unused]] static std::vector<std::pair<std::string, function2_t>> Q1Q1EP(bool non_zero_only) {
  if (non_zero_only) {
    return {{"Q1x_Q1x_EP", XX_EP}, {"Q1y_Q1y_EP", YY_EP}};
  } else {
    return {{"Q1x_Q1x_EP", XX_EP}, {"Q1y_Q1y_EP", YY_EP}, {"Q1x_Q1y_EP", XY_EP}, {"Q1y_Q1x_EP", YX_EP}};
  }
}
[[maybe_unused]] static std::vector<std::pair<std::string, function2_t>> u1Q1EP(bool non_zero_only) {
  if (non_zero_only) {
    return {{"u1x_Q1x_EP", xX_EP}, {"u1y_Q1y_EP", yY_EP}};
  } else {
    return {{"u1x_Q1x_EP", xX_EP}, {"u1y_Q1y_EP", yY_EP}, {"u1x_Q1y_EP", xY_EP}, {"u1y_Q1x_EP", yX_EP}};
  }
}

[[maybe_unused]] static std::vector<std::pair<std::string, function3_t>> Q2Q1Q1(bool non_zero_only) {
  if (non_zero_only) {
    return {{"Q2y_Q1x_Q1y", Y2XY}, {"Q2y_Q1y_Q1x", Y2YX}, {"Q2x_Q1x_Q1x", X2XX}, {"Q2x_Q1y_Q1y", X2YY}};
  } else {
    return {{"Q2y_Q1x_Q1y", Y2XY}, {"Q2y_Q1y_Q1x", Y2YX}, {"Q2x_Q1x_Q1x", X2XX}, {"Q2x_Q1y_Q1y", X2YY}, {"Q2x_Q1x_Q1y", X2XY}, {"Q2x_Q1y_Q1x", X2YX}, {"Q2y_Q1x_Q1x", Y2XX}, {"Q2y_Q1y_Q1y", Y2YY}};
  }
}

[[maybe_unused]] static std::vector<std::pair<std::string, function3_t>> Q3Q1Q1Q1 = {};// TODO

}// namespace Qn

#endif//FLOW_SRC_CORRELATE_UTILS_H_
