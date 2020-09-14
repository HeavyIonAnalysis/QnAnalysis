//
// Created by eugene on 12/08/2020.
//

#ifndef FLOW_SRC_BASE_HISTOGRAM_H
#define FLOW_SRC_BASE_HISTOGRAM_H

#include <vector>
#include <algorithm>
#include "Axis.h"

namespace Flow::Base {


struct HistogramConfig {
  std::vector<AxisConfig> axes;
};

struct Histogram {
  std::vector<Axis> axes;
};

namespace Utils {

Histogram Convert(const HistogramConfig &histogram_config);


}

}

#endif //FLOW_SRC_BASE_HISTOGRAM_H
