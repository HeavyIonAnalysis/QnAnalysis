//
// Created by eugene on 04/10/2020.
//

#ifndef QNANALYSIS_SRC_QNANALYSISCORRELATE_UTILS_HPP
#define QNANALYSIS_SRC_QNANALYSISCORRELATE_UTILS_HPP

#include <iostream>
#include <algorithm>
#include <vector>
#include <tuple>

namespace Qn::Analysis::Correlate::Utils {

namespace Details {

struct TensorIndex {
  explicit TensorIndex(std::vector<size_t> s) : shape(std::move(s)) {}

  std::vector<size_t> shape;

  [[nodiscard]] size_t size() const {
    size_t result = 0;
    for (auto i_size : shape) {
      result = result == 0 ? i_size : result * i_size;
    }
    return result;
  }

  [[nodiscard]] size_t dim() const { return shape.size(); }

  void GetIndex(size_t i, std::vector<size_t> &result) {
    for (size_t d = 0; d < dim(); ++d) {
      result[d] = i % shape[d];
      i = i / shape[d];
    }
  }
};

template<typename Tuple, typename Function, size_t ... I>
auto GetTensorElement(const std::vector<size_t> &index, Tuple &&t, Function &&f, std::index_sequence<I...>) {
  assert(index.size() == sizeof...(I));
  return f(std::get<I>(t)[index[I]]...);
}

}

template<typename IIter, typename OIter>
void
CombineDynamic(IIter &&i1, IIter &&i2, OIter &&o1) {
  using value_type = typename IIter::value_type::value_type;

  auto n_dim = std::distance(i1, i2);
  std::vector<std::size_t> shape;
  shape.reserve(n_dim);
  for (auto t = i1; t < i2; ++t) {
    auto &ax = *t;
    shape.emplace_back(std::distance(std::begin(ax), std::end(ax)));
  }
  Details::TensorIndex tind(shape);
  std::vector<size_t> index(n_dim);
  std::vector<value_type> result(n_dim);
  for (size_t i = 0; i < tind.size(); ++i) {
    tind.GetIndex(i, index);
    for (decltype(n_dim) d = 0; d < n_dim; ++d) {
      result[d] = (*(i1 + d))[index[d]];
    }
    o1 = result;
  }
}

template<typename OIter, typename Function, typename ... Container>
void Combine(OIter &&o, Function &&f, Container &&...containers) {
  auto container_tuple = std::make_tuple(containers...);

  auto get_size = [](auto &&c) -> std::size_t { return std::distance(std::cbegin(c), std::cend(c)); };

  std::vector<std::size_t> shape({get_size(containers)...});
  Details::TensorIndex tind(shape);
  std::vector<size_t> index(shape.size());

  for (std::size_t i = 0; i < tind.size(); ++i) {
    tind.GetIndex(i, index);
    o = Details::GetTensorElement(index, container_tuple,
                                  std::forward<Function>(f),
                                  std::make_index_sequence<sizeof...(Container)>());
  }

}

}

#endif //QNANALYSIS_SRC_QNANALYSISCORRELATE_UTILS_HPP
