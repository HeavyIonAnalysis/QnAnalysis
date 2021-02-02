
//
// Created by eugene on 25/11/2020.
//

#ifndef QNANALYSIS_SRC_QNANALYSISTOOLS_TENSORINDEX_HPP
#define QNANALYSIS_SRC_QNANALYSISTOOLS_TENSORINDEX_HPP

#include <vector>

namespace Qn {

namespace Analysis {

namespace Tools {

struct TensorIndex {
  typedef std::vector<std::size_t> shape_type;
  typedef std::vector<std::size_t> index_type;
  typedef std::size_t distance_type;

  explicit TensorIndex(shape_type s) : shape(std::move(s)) {}

  shape_type shape;

  distance_type size() const {
    if (shape.empty()) {
      return 0;
    }

    size_t result = shape[0];
    for (distance_type i = 1; i < shape.size(); ++i) {
      result *= shape[i];
    }
    return result;
  }

  distance_type dim() const { return shape.size(); }

  void GetIndex(distance_type i, index_type &result) {
    for (distance_type d = 0; d < dim(); ++d) {
      result[d] = i % shape[d];
      i = i / shape[d];
    }
  }
};

}

}

}

#endif //QNANALYSIS_SRC_QNANALYSISTOOLS_TENSORINDEX_HPP
