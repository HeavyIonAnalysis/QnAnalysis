//
// Created by eugene on 25/11/2020.
//

#ifndef QNANALYSIS_SRC_QNANALYSISTOOLS_COMBINEPERMUTE_HPP
#define QNANALYSIS_SRC_QNANALYSISTOOLS_COMBINEPERMUTE_HPP

#include <QnAnalysisTools/TensorIndex.hpp>

#include <boost/iterator/function_input_iterator.hpp>

#include <tuple>
#include <utility>

namespace Qn {

namespace Analysis {

namespace Tools {

namespace Impl {

}

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
  value_type GetValueImpl(const std::vector<size_t>& tuple_index, std::index_sequence<I...>) {
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
    value_type operator* () { return GetValue(); }
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
    difference_type cached_linear_index_;
  };

  iterator begin() {
    return combination_iterator_impl(0, this);
  }

  iterator end() {
    return combination_iterator_impl(tindex.size(), this);
  }

};

}

}

}

#endif //QNANALYSIS_SRC_QNANALYSISTOOLS_COMBINEPERMUTE_HPP
