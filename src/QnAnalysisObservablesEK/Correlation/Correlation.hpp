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

#include <QnTools/DataContainer.hpp>
#include <QnTools/StatCalculate.hpp>

namespace C4 {

namespace TensorOps {

typedef std::map<std::string, size_t> TensorAxes;
typedef std::map<std::string, size_t> TensorIndex;
typedef std::size_t TensorLinearIndex;

inline
TensorAxes
mergeAxes(const TensorAxes &lhs, const TensorAxes &rhs) {
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

template <typename ... Rest>
TensorAxes
mergeAxes(const TensorAxes &lhs, const Rest & ... rest) {
  return mergeAxes(lhs, mergeAxes(rest...));
}


/**
 * @brief
 * @tparam T
 */
template<typename T>
struct Tensor {
  typedef T value_type;

  typedef std::function<value_type(const TensorIndex &)> factory_function_type;

  Tensor(TensorAxes axes, factory_function_type factory_function)
      : axes_(std::move(axes)), factory_function_(factory_function) {
    for (auto && [axis_name, _] : axes_) {
      ax_names_.emplace_back(axis_name);
    }
  }


  TensorIndex getIndex(TensorLinearIndex idx) const {
    TensorIndex result;
    for (auto && axis_name : ax_names_) {
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
    for (auto && axis_name : r_ax_names) {
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

  auto begin();
  auto end();

  template<typename OT, typename BinaryFunction>
  auto
  applyBinary(BinaryFunction &&binary_function, const OT &other_arg) const {
    auto new_factory_function = [
        lhs = factory_function_,
        rhs = other_arg,
        outer = binary_function
    ](const TensorIndex &index) {
      return outer(lhs(index), rhs);
    };
    return Tensor{axes_, new_factory_function};
  }

  template<typename OT, typename BinaryFunction>
  auto
  applyBinary(BinaryFunction &&binary_function, const Tensor<OT> &other_tensor) const {
    auto new_axes = mergeAxes(axes_, other_tensor.axes_);
    auto new_factory_function = [
        lhs = factory_function_,
        rhs = other_tensor.factory_function_,
        outer = std::forward<BinaryFunction>(binary_function)](const TensorIndex &index) {
      return outer(lhs(index), rhs(index));
    };
    return Tensor{new_axes, new_factory_function};
  }

  template<typename OT>
  auto operator * (const OT& other) const {
    return applyBinary([] (auto && l, auto && r) { return l*r; }, other);
  }
  template<typename OT>
  auto operator / (const OT& other) const {
    return applyBinary([] (auto && l, auto && r) { return l / r; }, other);
  }

  template<typename UnaryFunction>
  auto applyUnary(UnaryFunction &&unary_function) const {
    auto new_factory_function = [
        inner = factory_function_,
        outer = unary_function](const TensorIndex &ind) {
      return outer(inner(ind));
    };
    return Tensor(axes_, factory_function_);
  }

  void checkIndex(const TensorIndex &ind) const {
    for (auto &&[ax_name, size] : axes_) {
      if (ind.at(ax_name) >= size) {
        throw std::out_of_range("out of range for '" + ax_name + "'");
      }
    }
  }



  std::vector<std::string> ax_names_;
  TensorAxes axes_;
  factory_function_type factory_function_;
};


template<typename Function> Tensor(TensorAxes, Function &&) ->
Tensor<std::decay_t<std::invoke_result_t<Function, const TensorIndex &>>>;

template<typename Left, typename Right>
auto operator * (const Left& l, const Tensor<Right> & r) {
  return r.operator*(l);
}


/**
 * @brief
 * @tparam T
 */
template<typename T>
struct Enumeration {

  template<typename Container>
  Enumeration(std::string name, Container &&c) : name_(std::move(name)) {
    TensorLinearIndex value_index = 0ul;
    for (T &v : c) {
      index_.emplace(value_index, v);
      ++value_index;
    }
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

  std::string name_;
  std::map<TensorLinearIndex, T> index_;
};

template<typename Container> Enumeration(std::string name,
                                         Container &&c) -> Enumeration<typename Container::value_type>;


template<typename T>
auto enumerate(std::string name, std::initializer_list<T> args) {
  return Enumeration(name, std::vector<T>(args));
}

template <typename T>
TensorAxes getAxes(const T& t) { return {}; }
template <typename T>
TensorAxes getAxes(const Enumeration<T>& r) { return {{r.name_, r.size()}}; }
template<typename T>
TensorAxes getAxes(const Tensor<T>& t) { return t.axes_; }

template <typename T>
auto eval(const T& t, const TensorIndex& /* */) { return t; }
template <typename T>
auto eval(const Enumeration<T> &e, const TensorIndex& index) { return e.tensor().at(index); }
template <typename T>
auto eval(const Tensor<T> &t, const TensorIndex& index) { return t.at(index); }


template <typename Function, typename ... Args>
auto tensorize_f_args(Function && f, Args && ... args) {
  static_assert(sizeof...(Args) > 0);
  auto axes = mergeAxes(getAxes(std::forward<Args>(args))...);
  return Tensor(axes, [=] (const TensorIndex& index) {
    return f(eval(args, index)...);
  });
}

} // namespace TensorOps

namespace Correlations {

enum class EComponent {
  X, Y
};

struct QVec {
  std::string name_;
  QVec(std::string name, unsigned int harmonic, EComponent component)
      : name_(std::move(name)), harmonic_(harmonic), component_(component) {}
  unsigned int harmonic_;
  EComponent component_;

  bool operator==(const QVec &rhs) const {
    return name_ == rhs.name_ &&
        harmonic_ == rhs.harmonic_ &&
        component_ == rhs.component_;
  }

  bool operator!=(const QVec &rhs) const {
    return !(rhs == *this);
  }
};

struct Correlation {
  typedef Qn::DataContainerStatCalculate result_type;

  explicit Correlation(std::vector<QVec> q_vectors) : q_vectors_(std::move(q_vectors)) {}

  result_type value() const {
    std::cout << nameInFile() << std::endl;
    return {};
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

  std::vector<QVec> q_vectors_;
};

template<typename Left, typename Right, typename Function>
struct BinaryOp {
  typedef std::decay_t<
      std::invoke_result_t<
          Function,
          const typename Left::result_type &,
          const typename Right::result_type &>
  > result_type;

  BinaryOp(Left lhs, Right rhs, Function function) : lhs(lhs), rhs(rhs), function_(function) {}

  result_type value() const {
    return function_(lhs.value(), rhs.value());
  }

  Left lhs;
  Right rhs;
  Function function_;
};

template<typename Arg, typename Function>
struct UnaryOp {
  typedef std::decay_t<
      std::invoke_result_t<
          Function,
          const typename Arg::result_type &>
  > result_type;

  UnaryOp(Arg value, Function function) : arg_(value), function_(function) {}

  result_type value() const {
    return function_(arg_.value());
  }

  template<typename Function1>
  auto apply(Function1 &&f) {
    return UnaryOp(*this, std::forward<Function1>(f));
  }

  Arg arg_;
  Function function_;
};

template<typename T>
struct Value {
  typedef T result_type;

  Value(T value) : value_(value) {}

  T value() const {
    return value_;
  }

  T value_;
};

template<typename L, typename R>
auto operator*(const L &cl, const R &cr) {
  return BinaryOp(cl, cr, [](const auto &l, const auto &r) {
    return l * r;
  });
}

template<typename L, typename R>
auto operator/(const L &cl, const R &cr) {
  return BinaryOp(cl, cr, [](const auto &l, const auto &r) {
    return l / r;
  });
}

template<typename V>
auto sqrt(const V &v) {
  return UnaryOp(v, [](const auto &v) {
    return Sqrt(v);
  });
}

/* functions to simplify code */
QVec q(std::string name, unsigned int harmonic, EComponent component) {
  return {std::move(name), harmonic, component};
}

template <typename ... Args>
TensorOps::Tensor<QVec> qt(Args ... args) {
  return TensorOps::tensorize_f_args(
      q,
      std::forward<Args>(args)...);
}

template<typename ... QVs>
Correlation c(QVs && ... qvs) {
  static_assert(sizeof...(QVs) > 1);
  return Correlation({qvs...});
}

template <typename ... Args>
TensorOps::Tensor<Correlation> ct(Args ... args) {
  std::cout << __PRETTY_FUNCTION__  << std::endl;
  return TensorOps::tensorize_f_args(
      c<typename Args::value_type...>,
      args...);
}

}

}

#endif //QNANALYSIS_SRC_QNANALYSISOBSERVABLESEK_CORRELATION_CORRELATION_HPP_
