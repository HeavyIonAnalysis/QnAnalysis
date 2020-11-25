//
// Created by eugene on 25/11/2020.
//

#ifndef QNANALYSIS_SRC_QNANALYSISTOOLS_COMBINEPERMUTE_HPP
#define QNANALYSIS_SRC_QNANALYSISTOOLS_COMBINEPERMUTE_HPP

#include <QnAnalysisTools/TensorIndex.hpp>

#include <algorithm>
#include <cmath>
#include <tuple>
#include <utility>

namespace Qn {

namespace Analysis {

namespace Tools {

template<typename ... Axis>
struct Combination {

  class combination_iterator_impl; /// fwd declaration

  typedef std::tuple<Axis...> basis_tuple;
  typedef std::tuple<typename Axis::value_type...> value_type;
  typedef combination_iterator_impl iterator;

  basis_tuple basis_;
  TensorIndex tindex;

  template<typename T> static std::size_t GetAxisSize(T axis) { return axis.size(); }
  template<typename ... Ax> static TensorIndex::shape_type GetShape(Ax...axes) { return {GetAxisSize(axes)...}; }

  explicit Combination(Axis ... axes) :
      basis_(std::make_tuple(axes...)), tindex(GetShape(axes...)) {}

  value_type GetValue(const std::vector<size_t> &tuple_index) {
    return GetValueImpl(tuple_index, std::make_index_sequence<sizeof...(Axis)>());
  }

  template<size_t ... I>
  value_type GetValueImpl(const std::vector<size_t> &tuple_index, std::index_sequence<I...>) {
    return std::make_tuple((std::get<I>(basis_)[tuple_index[I]])...);
  }

  class combination_iterator_impl {

  public:
    using iterator_category = std::bidirectional_iterator_tag;
    using value_type = std::tuple<typename Axis::value_type...>;
    using difference_type = TensorIndex::distance_type;
    using pointer = value_type *;
    using reference = value_type &;

    combination_iterator_impl(unsigned long LinearIndex, Combination<Axis...> *Parent)
        : linear_index_(LinearIndex), parent_(Parent) {}

    /* equality same parent + same state */
    bool operator==(const combination_iterator_impl &Rhs) const {
      return linear_index_ == Rhs.linear_index_ &&
          parent_ == Rhs.parent_;
    }
    bool operator!=(const combination_iterator_impl &Rhs) const {
      return !(Rhs == *this);
    }
    /* increments */
    void operator++() { ++linear_index_; }
    void operator++(int) { linear_index_++; }
    /* decrements */
    void operator--() { --linear_index_; }
    void operator--(int) { linear_index_--; }
    /* dereferencing (rvalue) */
    value_type operator*() { return GetValue(); }
    value_type operator->() { return GetValue(); }

  private:
    value_type GetValue() {
      tuple_index_.resize(parent_->tindex.dim());
      parent_->tindex.GetIndex(linear_index_, tuple_index_);
      return parent_->GetValue(tuple_index_);
    }

    difference_type linear_index_{0};
    Combination<Axis...> *parent_; /// non-owing pointer


    TensorIndex::index_type tuple_index_;
    std::unique_ptr<const value_type> cached_value_{nullptr};
    difference_type cached_linear_index_{0};
  }; /// combination_iterator_impl

  iterator begin() {
    return combination_iterator_impl(0, this);
  }

  iterator end() {
    return combination_iterator_impl(tindex.size(), this);
  }

};

template<typename Container>
struct Permutation {
  class permutation_iterator_impl; /// fwd declaration

  typedef Container container_type;
  typedef permutation_iterator_impl iterator;
  typedef Container value_type;

  explicit Permutation(container_type container) : container_(container) {}

  class permutation_iterator_impl {
  public:
    using iterator_category = std::bidirectional_iterator_tag;
    using value_type = container_type;
    using difference_type = std::size_t;
    using pointer = value_type *;
    using reference = value_type &;

    permutation_iterator_impl(bool beyond_end, container_type container)
        : beyound_last_flag_(beyond_end), container_(container) {}

    bool operator==(const permutation_iterator_impl &Rhs) const {
      return beyound_last_flag_ == Rhs.beyound_last_flag_ &&
          container_ == Rhs.container_;
    }
    bool operator!=(const permutation_iterator_impl &Rhs) const {
      return !(Rhs == *this);
    }
    /* increments */
    void operator++() { beyound_last_flag_ = !std::next_permutation(std::begin(container_), std::end(container_)); }
    void operator++(int) { beyound_last_flag_ = !std::next_permutation(std::begin(container_), std::end(container_)); }
    /* decrements */
    void operator--() { std::prev_permutation(std::begin(container_), std::end(container_)); }
    void operator--(int) { std::prev_permutation(std::begin(container_), std::end(container_)); }
    /* dereferencing (rvalue) */
    value_type operator*() { return container_; }
    value_type operator->() { return container_; }

  private:
    bool beyound_last_flag_{false};
    container_type container_;
  }; /// permutation_iterator_impl

  iterator begin() {
    auto container_sorted = container_;
    std::sort(std::begin(container_sorted), std::end(container_sorted));
    return permutation_iterator_impl(false, container_sorted);
  }

  iterator end() {
    auto container_sorted = container_;
    std::sort(std::begin(container_sorted), std::end(container_sorted));
    return permutation_iterator_impl(true, container_sorted);
  }

  std::size_t size() const {
    return factorial(container_.size());
  }

  static inline std::size_t factorial(std::size_t N) { return (N < 2)? 1 : factorial(N-1); }

  container_type container_;

};

}

}

}

#endif //QNANALYSIS_SRC_QNANALYSISTOOLS_COMBINEPERMUTE_HPP
