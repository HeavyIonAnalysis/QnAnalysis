//
// Created by eugene on 12/08/2020.
//

#ifndef FLOW_SRC_BASE_HISTOGRAM_H
#define FLOW_SRC_BASE_HISTOGRAM_H

#include <algorithm>
#include <vector>

#include <QnAnalysisBase/Axis.hpp>

namespace Qn::Analysis::Base {

struct HistogramConfig {
  std::vector<AxisConfig> axes;
  std::string weight{"Ones"};
};

struct Histogram {
  std::vector<Axis> axes;
  std::string weight;
};

namespace Utils {

Histogram Convert(const HistogramConfig& histogram_config);

}

}// namespace Qn::Analysis::Base

#endif//FLOW_SRC_BASE_HISTOGRAM_H
