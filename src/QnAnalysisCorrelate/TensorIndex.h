//
// Created by eugene on 03/08/2020.
//

#ifndef DATATREEFLOW_SRC_CORRELATION_TENSORINDEX_H
#define DATATREEFLOW_SRC_CORRELATION_TENSORINDEX_H

namespace Details {

struct TensorIndex {
  explicit TensorIndex(std::vector<size_t> s) : shape(std::move(s)) {}

  std::vector<size_t> shape;

  [[nodiscard]] size_t size() const {
    size_t result = 1;
    for (auto i_end : shape) {
      result *= i_end;
    }
    return result;
  }

  [[nodiscard]] size_t dim() const { return shape.size(); }

  std::vector<size_t> operator[](size_t i) const {
    std::vector<size_t> result;
    result.resize(dim());

    for (size_t d = 0; d < dim(); ++d) {
      result[d] = i % shape[d];
      i = i / shape[d];
    }

    return result;
  }
};

} // Details

#endif  // DATATREEFLOW_SRC_CORRELATION_TENSORINDEX_H
