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

#include <QnTools/DataContainer.hpp>
#include <QnTools/StatCalculate.hpp>

namespace C4 {

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
  auto apply(Function1 && f) {
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

template<typename ... QVs>
Correlation C(QVs ... qvs) {
  return Correlation({qvs...});
}

}

#endif //QNANALYSIS_SRC_QNANALYSISOBSERVABLESEK_CORRELATION_CORRELATION_HPP_
