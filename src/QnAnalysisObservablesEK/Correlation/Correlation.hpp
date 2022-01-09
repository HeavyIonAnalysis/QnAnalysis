//
// Created by eugene on 27/12/2021.
//

#ifndef QNANALYSIS_SRC_QNANALYSISOBSERVABLESEK_CORRELATION_CORRELATION_HPP_
#define QNANALYSIS_SRC_QNANALYSISOBSERVABLESEK_CORRELATION_CORRELATION_HPP_

#include <string>
#include <utility>
#include <vector>
#include <sstream>
#include <map>
#include <tuple>
#include <ostream>
#include <list>
#include <iterator>


#include <QnTools/DataContainer.hpp>
#include <QnTools/StatCalculate.hpp>

#include <Rtypes.h>
#include <TDirectory.h>

namespace C4 {

namespace TensorOps {

typedef std::map<std::string, size_t> TensorAxes;
typedef std::map<std::string, size_t> TensorIndex;
typedef std::size_t TensorLinearIndex;

template<typename T>
struct Tensor;

template<typename T>
decltype(auto) tensorize(T &&t);

inline
TensorAxes
merge_axes(const TensorAxes &lhs, const TensorAxes &rhs) {
  auto result = lhs;
  for (auto &&[axis_name, axis_size] : rhs) {
    auto emplace_result = result.emplace(axis_name, axis_size);
    if (!emplace_result.second) { /* common axis */
      if (emplace_result.first->second != axis_size) {
        throw std::runtime_error("Common axis '" + axis_name + "' must have the same size");
      }
    }
  }
  return result;
}

template<typename ... Rest>
TensorAxes
merge_axes(const TensorAxes &lhs, const Rest &... rest) {
  return merge_axes(lhs, merge_axes(rest...));
}

template <typename T>
struct TensorElement {
  TensorLinearIndex linear_index{0ul};
  TensorIndex index;
  std::function<T()> element_factory;

  T operator()() const {
    return element_factory();
  }

};

template<typename T>
struct TensorIterator {

#ifndef __ROOTCLING__
  /* see https://sft.its.cern.ch/jira/si/jira.issueviews:issue-html/ROOT-9113/ROOT-9113.html */
  using iterator_category = std::bidirectional_iterator_tag;
#endif
  using value_type = const TensorElement<T>;
  using difference_type = long int;
  using pointer = value_type*;
  using reference = value_type&;


  TensorIterator(TensorLinearIndex linear_index, const Tensor<T> *tensor_ptr)
  : linear_index_(linear_index), tensor_ptr_(tensor_ptr) {
    updateElement();
  }

  TensorIterator &operator++() {
    ++linear_index_;
    updateElement();
    return *this;
  }
  TensorIterator &operator--() {
    --linear_index_;
    updateElement();
    return *this;
  }
  reference operator*() const {
    return element_;
  }
  explicit operator bool() const {
    return bool(tensor_ptr_);
  }

  bool operator==(const TensorIterator &rhs) const {
    return linear_index_ == rhs.linear_index_ &&
    tensor_ptr_ == rhs.tensor_ptr_;
  }
  bool operator!=(const TensorIterator &rhs) const {
    return !(rhs == *this);
  }

 private:
  void updateElement() {
    if (linear_index_ < tensor_ptr_->size()) {
      element_.linear_index = linear_index_;
      element_.index = tensor_ptr_->getIndex(linear_index_);
      element_.element_factory = [
          tensor_ptr = tensor_ptr_,
          linear_index = linear_index_]() {
        return tensor_ptr->at(linear_index);
      };
    } else {
      element_.linear_index = tensor_ptr_->size();
      element_.element_factory = nullptr;
    }
  }

  TensorElement<T> element_;
  TensorLinearIndex linear_index_{0ul};
  const Tensor<T> *tensor_ptr_;

};

/**
 * @brief
 * @tparam T
 */
template<typename T>
struct Tensor {
  typedef std::decay_t<T> value_type;
  typedef std::function<value_type(const TensorIndex &)> factory_function_type;


  Tensor(TensorAxes axes, factory_function_type factory_function)
      : axes_(std::move(axes)), factory_function_(factory_function) {
    for (auto &&[axis_name, _] : axes_) {
      ax_names_.emplace_back(axis_name);
    }
  }

  TensorIndex getIndex(TensorLinearIndex idx) const {
    TensorIndex result;
    for (auto &&axis_name : ax_names_) {
      auto axis_size = axes_.at(axis_name);
      auto axis_id = idx % axis_size;
      result.template emplace(axis_name, axis_id);
      idx /= axis_size;
    }
    return result;
  }
  TensorLinearIndex getLinearIndex(const TensorIndex &idx) const {
    checkIndex(idx);
    TensorLinearIndex result = 0ul;

    std::vector<std::string> r_ax_names(ax_names_.rbegin(), ax_names_.rend());
    for (auto &&axis_name : r_ax_names) {
      auto ax_idx = idx.at(axis_name);
      result *= axes_.at(axis_name);
      result += ax_idx;
    }
    return result;
  }
  std::vector<std::string> getAxNames() const {
    return ax_names_;
  }

  value_type at(const TensorIndex &idx) const {
    checkIndex(idx);
    return factory_function_(idx);
  }

  value_type at(TensorLinearIndex idx) const {
    return at(getIndex(idx));
  }

  TensorLinearIndex size() const {
    TensorLinearIndex result = 1;
    for (auto &&[_, axis_size] : axes_) {
      result *= axis_size;
    }
    return result;
  }

  TensorIterator<value_type> begin() const {
    return {0ul, this};
  }
  TensorIterator<value_type> end() const {
    return {size(), this};
  }

  template<typename BinaryFunction, typename OtherArg>
  auto
  applyBinary(BinaryFunction &&binary_function, const OtherArg &other_arg) const {
    auto &&other_tensor = tensorize(other_arg);
    using result_type = std::decay_t<std::invoke_result_t<
        BinaryFunction,
        value_type,
        typename std::decay_t<decltype(other_tensor)>::value_type>>;

    auto new_axes = merge_axes(this->getAxes(), other_tensor.getAxes());
    auto new_factory_function = std::function{[
                                                  lhs = factory_function_,
                                                  rhs = other_tensor.getFactoryFunction(),
                                                  outer = std::forward<BinaryFunction>(binary_function)
                                              ](const TensorIndex &index) {
      return outer(lhs.operator()(index), rhs.operator()(index));
    }};
    return Tensor<result_type>{std::move(new_axes), std::move(new_factory_function)};
  }

  template<typename Left>
  friend
  auto
  operator+(Left lhs, const Tensor<T> &t) {
    return t.applyBinary([](auto &&r, auto &&l) { return l + r; }, lhs);
  }

  template<typename Left>
  friend
  auto
  operator-(Left lhs, const Tensor<T> &t) {
    return t.applyBinary([](auto &&r, auto &&l) { return l - r; }, lhs);
  }

  template<typename Left>
  friend
  auto
  operator*(Left lhs, const Tensor<T> &t) {
    return t.applyBinary([](auto &&r, auto &&l) { return l * r; }, lhs);
  }

  template<typename Left>
  friend
  auto
  operator/(Left lhs, const Tensor<T> &t) {
    return t.applyBinary([](auto &&r, auto &&l) { return l / r; }, lhs);
  }

  template<typename UnaryFunction>
  auto applyUnary(UnaryFunction &&unary_function) const {
    using result_type = std::decay_t<std::invoke_result_t<
        UnaryFunction,
        value_type>>;
    auto new_factory_function = [
        inner = factory_function_,
        outer = unary_function](const TensorIndex &ind) {
      return outer(inner(ind));
    };
    return Tensor<result_type>{axes_, std::move(new_factory_function)};
  }

  void checkIndex(const TensorIndex &ind) const {
    for (auto &&[ax_name, size] : axes_) {
      if (ind.at(ax_name) >= size) {
        throw std::out_of_range("out of range for '" + ax_name + "'");
      }
    }
  }

  const TensorAxes &getAxes() const {
    return axes_;
  }
  const factory_function_type &getFactoryFunction() const {
    return factory_function_;
  }

 private:
  std::vector<std::string> ax_names_;

  TensorAxes axes_;
  factory_function_type factory_function_;

};

template<typename Function>
Tensor(TensorAxes, Function &&) -> Tensor<std::decay_t<std::invoke_result_t<Function, const TensorIndex &>>>;

template<typename T>
auto sqrt(const Tensor<T> &t) {
  return t.applyUnary([](auto &&v) { return sqrt(v); });
}

/**
 * @brief
 * @tparam T
 */
template<typename T>
struct Enumeration {
  typedef std::decay_t<T> value_type;

  template<typename Container>
  Enumeration(std::string name, const Container &c) : name_(std::move(name)) {
    TensorLinearIndex value_index = 0ul;
    for (auto && v : c) {
      index_.emplace(value_index, v);
      inverse_index_.emplace(v, value_index);
      ++value_index;
    }
  }

  const std::string &getName() const {
    return name_;
  }

  T operator()(const TensorIndex &ind) const {
    return index_.at(ind.at(name_));
  }

  Tensor<T> tensor() const {
    return {{{name_, index_.size()}}, *this};
  }

  TensorLinearIndex size() const {
    return index_.size();
  }

  auto at(const TensorIndex &index) {
    return index_.at(index.at(name_));
  }

  TensorIndex index(const T &v) {
    return {{name_, inverse_index_.at(v)}};
  }

  auto clone(const std::string &new_name) const {
    Enumeration<T> result = *this;
    result.name_ = new_name;
    return result;
  }

 private:
  std::string name_;
  std::map<TensorLinearIndex, value_type> index_;
  std::unordered_map<value_type , TensorLinearIndex> inverse_index_;

};

template<typename T>
auto enumerate(std::string name, std::initializer_list<T> args) {
  return Enumeration(name, std::vector<T>(args));
}

template<typename Container> Enumeration(std::string name,
                                         Container &&c) -> Enumeration<typename Container::value_type>;

template<typename ... Args>
TensorIndex make_index(Args &&... args) {
  return merge_axes(std::forward<Args>(args)...);
}

namespace Details {

template<typename T>
struct tag {};

template<typename T>
auto tensor_cast(tag<T>) {
  return [](auto &&t) -> decltype(auto) {
    return Tensor{{}, [value = std::forward<decltype(t)>(t)](const TensorIndex &) { return value; }};
  };
}

template<typename T>
auto tensor_cast(tag<Tensor<T>>) {
  return [](auto &&t) -> decltype(auto) { return std::forward<decltype(t)>(t); };
}

template<typename T>
auto tensor_cast(tag<Enumeration<T>>) {
  return [](auto &&t) -> decltype(auto) { return t.tensor(); };
}

} // namespace Details


template<typename T>
decltype(auto) tensorize(T &&t) {
  auto cast = Details::tensor_cast(Details::tag<std::decay_t<T>>());
  return cast(std::forward<T>(t));
}

template<typename Function, typename ... Args>
auto tensorize_f_args(Function &&f, Args &&... args) {
  static_assert(sizeof...(Args) > 0);
  auto get_axes = [](const auto &t) { return t.getAxes(); };
  auto result_axes = merge_axes(get_axes(tensorize(args))...);
  auto tensors_tuple = std::make_tuple(tensorize(args)...);
  return Tensor(result_axes, [
      function = std::forward<Function>(f),
      tensors_tuple = std::move(tensors_tuple)](const TensorIndex &index) {
    auto args_tuple = std::apply([index](auto &&... tensors) { return std::make_tuple(tensors.at(index)...); },
                                 tensors_tuple);
    return std::apply(function, std::move(args_tuple));
  });
}

} // namespace TensorOps

namespace LazyOps {

} // namespace LazyOps


namespace CorrelationOps {

enum class EComponent {
  X, Y
};

struct LazyArithmeticsTag {};

struct QVec {
  std::string name_;
  QVec(std::string name, unsigned int harmonic, EComponent component)
      : name_(std::move(name)), harmonic_(harmonic), component_(component) {}
  bool operator==(const QVec &rhs) const {
    return name_ == rhs.name_ &&
        harmonic_ == rhs.harmonic_ &&
        component_ == rhs.component_;
  }

  bool operator!=(const QVec &rhs) const {
    return !(rhs == *this);
  }

  friend std::ostream &operator<<(std::ostream &os, const QVec &vec);

  unsigned int harmonic_;
  EComponent component_;

};

inline
std::ostream &operator<<(std::ostream &os, const QVec &vec) {
  static const std::map<EComponent, std::string> map_component{
      {EComponent::X, "X"},
      {EComponent::Y, "Y"}};
  os << "Q_{" << vec.harmonic_ << "," << map_component.at(vec.component_) << "}(" << vec.name_ << ")";
  return os;
}

struct Correlation
    : public LazyArithmeticsTag {
  typedef Qn::DataContainerStatCalculate result_type;

  struct CorrelationNotFoundException : public std::exception {};

  Correlation(TDirectory *directory, std::vector<QVec> q_vectors)
      : directory_(directory), q_vectors_(std::move(q_vectors)) {}

  result_type value() const {
    auto collect = directory_->Get<Qn::DataContainerStatCollect>(nameInFile().c_str());
    if (!collect) {
      throw CorrelationNotFoundException();
    }
    return Qn::DataContainerStatCalculate(*collect);
  }

  std::string nameInFile() const {
    static const std::map<EComponent, std::string> map_component{
        {EComponent::X, "x"},
        {EComponent::Y, "y"}
    };

    std::stringstream stream;
    for (auto &q : q_vectors_) {
      stream << q.name_ << ".";
    }
    /* suffix */
    for (auto &q : q_vectors_) {
      stream << map_component.at(q.component_) << q.harmonic_;
    }
    return stream.str();
  }
  friend std::ostream &operator<<(std::ostream &os, const Correlation &correlation);

  TDirectory *directory_{nullptr};
  std::vector<QVec> q_vectors_;

};

inline
std::ostream &operator<<(std::ostream &os, const Correlation &correlation) {
  os << "< ";
  for (auto &q : correlation.q_vectors_) {
    os << q << " ";
  }
  os << ">";
  return os;
}

template<typename T>
struct Value :
    public LazyArithmeticsTag {
  typedef T result_type;

  explicit Value(T value) : value_(value) {}

  T value() const {
    return value_;
  }

  friend std::ostream &operator<<(std::ostream &os, const Value &value) {
    os << value.value_;
    return os;
  }

 private:
  T value_;

};

template<typename Left, typename Right, typename Function>
struct BinaryOp :
    public LazyArithmeticsTag {
  typedef std::decay_t<
      std::invoke_result_t<
          Function,
          const typename Left::result_type &,
          const typename Right::result_type &>
  > result_type;

  BinaryOp(Left lhs, Right rhs, Function function) : lhs(lhs), rhs(rhs), function_(function) {}
  BinaryOp(Left lhs, Right rhs, Function function, std::string op_symbol)
      : lhs(lhs), rhs(rhs), function_(function), symbol_(std::move(op_symbol)) {}

  result_type value() const {
    return function_(lhs.value(), rhs.value());
  }

  friend std::ostream &operator<<(std::ostream &os, const BinaryOp &op) {
    os << "( " << op.lhs << op.symbol_ << op.rhs << " )";
    return os;
  }

 private:
  Left lhs;
  Right rhs;
  Function function_;
  std::string symbol_{" "};

};

template<typename Arg, typename Function>
struct UnaryOp :
    public LazyArithmeticsTag {
  typedef std::decay_t<
      std::invoke_result_t<
          Function,
          const typename Arg::result_type &>
  > result_type;

  UnaryOp(Arg value, Function function) : arg_(value), function_(function) {}
  UnaryOp(Arg arg, Function function, std::string display_name)
      : arg_(arg), function_(function), display_name_(std::move(display_name)) {}

  result_type value() const {
    return function_(arg_.value());
  }

  template<typename Function1>
  auto apply(Function1 &&f) {
    return UnaryOp(*this, std::forward<Function1>(f));
  }

  friend std::ostream &operator<<(std::ostream &os, const UnaryOp &op) {
    os << op.display_name_ << "(" << op.arg_ << ")";
    return os;
  }

 private:
  Arg arg_;
  Function function_;
  std::string display_name_{};

};



template<typename ... Args>
constexpr bool have_lazy_arithmetics_v = (... && std::is_base_of_v<LazyArithmeticsTag, std::decay_t<Args>>);

template<
    typename LeftArg,
    typename RightArg,
    typename Dummy = std::enable_if_t<have_lazy_arithmetics_v<LeftArg, RightArg>, void>>
auto
operator*(LeftArg &&cl, RightArg &&cr) {
  return BinaryOp(cl, cr, [](auto &&l, auto &&r) {
    return l * r;
  });
}

template<
    typename LeftArg,
    typename RightArg,
    typename Dummy = std::enable_if_t<have_lazy_arithmetics_v<LeftArg, RightArg>, void>>
auto operator/(LeftArg &&cl, RightArg &&cr) {
  return BinaryOp(cl, cr, [](auto &&l, auto &&r) { return l / r; }, "/");
}

template<
    typename Arg,
    typename Dummy = std::enable_if_t<have_lazy_arithmetics_v<Arg>, void>
>
auto sqrt(Arg &&v) {
  return UnaryOp(v, [](const auto &v) {
    return Sqrt(v);
  }, "sqrt");
}

/* functions to simplify code */
inline
QVec q(std::string name, unsigned int harmonic, EComponent component) {
  return {std::move(name), harmonic, component};
}

template<typename ... Args>
TensorOps::Tensor<QVec> qt(Args &&... args) {
  return TensorOps::tensorize_f_args(
      q,
      std::forward<Args>(args)...);
}

template<typename ... QVs>
Correlation c(TDirectory *folder, QVs &&... qvs) {
  static_assert(sizeof...(QVs) > 1);
  return Correlation(folder, {qvs...});
}

template<typename ... Args>
TensorOps::Tensor<Correlation> ct(TDirectory *folder, Args &&... args) {
  auto function = [folder](auto &&... args) {
    return c(folder, args...);
  };
  return TensorOps::tensorize_f_args(function, std::forward<Args>(args)...);
}


} // namespace CorrelationOps

}

#endif //QNANALYSIS_SRC_QNANALYSISOBSERVABLESEK_CORRELATION_CORRELATION_HPP_
