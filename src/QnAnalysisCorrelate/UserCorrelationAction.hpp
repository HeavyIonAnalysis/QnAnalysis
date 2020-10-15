//
// Created by eugene on 04/08/2020.
//

#ifndef DATATREEFLOW_SRC_CORRELATION_CORRELATIONACTION_H
#define DATATREEFLOW_SRC_CORRELATION_CORRELATIONACTION_H

#include <functional>
#include <utility>

#include <QnDataFrame.hpp>

#include "StaticRegistry.h"

#ifndef REGISTER_ACTION
#define REGISTER_ACTION(FUNCTION, NAME)                                \
  namespace {                                                                 \
  size_t Action_##NAME = ::Qn::Analysis::Correlate::Register(                \
      std::string(#NAME),                                                     \
      ::Qn::Analysis::Correlate::Action::Details::WrapFunction(FUNCTION));               \
  }
#endif // REGISTER_ACTION

namespace Qn::Analysis::Correlate::Action {

namespace Details {

template<typename T>
struct Arity : Arity<decltype(&T::operator())> {};

/* Raw function reference */
template<typename R, typename... Args>
struct Arity<R(&)(Args...)> : std::integral_constant<std::size_t, sizeof...(Args)> {};
/* Raw function pointer */
template<typename R, typename... Args>
struct Arity<R(*)(Args...)> : std::integral_constant<std::size_t, sizeof...(Args)> {};
/* Member functions: */
template<typename R, typename C, typename... Args>
struct Arity<R(C::*)(Args...)> :
    std::integral_constant<std::size_t, sizeof...(Args)> {
};
/* with const modifier */
template<typename R, typename C, typename... Args>
struct Arity<R(C::*)(Args...) const> :
    std::integral_constant<std::size_t, sizeof...(Args)> {
};

template<size_t I>
using ActionArgI = const Qn::QVector &;

/**
 * @brief Interface to wrapper with proper signature of () op-r.
 * @tparam IArg
 */
template<size_t ... IArg>
struct FunctionWrapperBase {
  static constexpr std::size_t ARITY = sizeof...(IArg);
  virtual float operator()(ActionArgI<IArg>...args) const = 0;
};

template<size_t...IArg>
struct FunctionWrapperPtr : public FunctionWrapperBase<IArg...> {
  explicit FunctionWrapperPtr(std::shared_ptr<FunctionWrapperBase<IArg...>> ptr) : ptr_(std::move(ptr)) {}
  explicit FunctionWrapperPtr(FunctionWrapperBase<IArg...> *ptr) {
    ptr_.reset(ptr);
  }
  float operator()(ActionArgI<IArg>... args) const override {
    return (*ptr_).operator()(args...);
  }
  auto &operator*() { return *ptr_; }
  std::shared_ptr<FunctionWrapperBase<IArg...>> ptr_;
};

template<typename Function, size_t ... IArg>
struct FunctionWrapperImpl : public FunctionWrapperBase<IArg...> {
  explicit FunctionWrapperImpl(Function &&F) : f(F) {}
  float operator()(ActionArgI<IArg>... args) const override {
    return f(args...);
  }
  Function f;
};

template<typename Function, size_t...IArg>
auto WrapFunctionImpl(Function &&f, std::index_sequence<IArg...>) {
  return FunctionWrapperPtr<IArg...>(
      std::make_shared<FunctionWrapperImpl<Function, IArg...>>(std::forward<Function>(f)));
}

template<size_t... I>
auto GetActionRegistryImpl(std::index_sequence<I...>) {
  return StaticRegistry<FunctionWrapperPtr<I...>, std::string>::Instance();
}

template<typename Function>
auto WrapFunction(Function &&f) {
  constexpr std::size_t function_arity = Arity<Function>::value;
  return WrapFunctionImpl(std::forward<Function>(f), std::make_index_sequence<function_arity>());
}

}  // namespace Details

struct X {};
struct Cos {};
struct Y {};
struct Sin {};

template<typename Component, size_t Harmonic>
struct Q {
  static inline float Eval(const Qn::QVector &v) {
    return EvalImpl(v, Component());
  }

  static inline float EvalImpl(const Qn::QVector &v, X) {
    return v.x(Harmonic);
  }
  static inline float EvalImpl(const Qn::QVector &v, Y) {
    return v.y(Harmonic);
  }
  static inline float EvalImpl(const Qn::QVector &v, Cos) {
    return v.x(Harmonic) / v.mag(Harmonic);
  }
  static inline float EvalImpl(const Qn::QVector &v, Sin) {
    return v.y(Harmonic) / v.mag(Harmonic);
  }
};

template<size_t Harmonic>
using Qx = Q<X, Harmonic>;

template<size_t Harmonic>
using CosQ = Q<Cos, Harmonic>;

using Qx1 = Qx<1>;
using Qx2 = Qx<2>;
using Qx3 = Qx<3>;

template<size_t Harmonic>
using Qy = Q<Y, Harmonic>;

template<size_t Harmonic>
using SinQ = Q<Sin, Harmonic>;

using Qy1 = Qy<1>;
using Qy2 = Qy<2>;
using Qy3 = Qy<3>;

template<typename... Q>
struct Builder {
  template<typename T>
  using Arg_t = const Qn::QVector &;

  static inline float F(Arg_t<Q>... arg) {
    return (Q::Eval(arg) * ... * 1);
  }
};

template<size_t ActionArity>
auto GetActionRegistry() {
  return Details::GetActionRegistryImpl(std::make_index_sequence<ActionArity>());
}

}  // namespace Correlation::Action

namespace Qn::Analysis::Correlate::Action {

/* Two-particle correlations */
template<size_t Harmonic>
float ScalarProduct(const Qn::QVector &q1, const Qn::QVector &q2) {
  return Qn::ScalarProduct(q1, q2, Harmonic);
}
REGISTER_ACTION(ScalarProduct<1>, scalar_product);

REGISTER_ACTION((Builder<Qx1, Qx1>::F), xx)
REGISTER_ACTION((Builder<Qx1, Qx1>::F), x1x1) // Alias!
REGISTER_ACTION((Builder<Qx1, Qy1>::F), xy)
REGISTER_ACTION((Builder<Qx1, Qy1>::F), x1y1)
REGISTER_ACTION((Builder<Qy1, Qx1>::F), yx)
REGISTER_ACTION((Builder<Qy1, Qx1>::F), y1x1)
REGISTER_ACTION((Builder<Qy1, Qy1>::F), yy)
REGISTER_ACTION((Builder<Qy1, Qy1>::F), y1y1)

REGISTER_ACTION((Builder<CosQ<1>, CosQ<1>>::F), xx_ep)
REGISTER_ACTION((Builder<CosQ<1>, SinQ<1>>::F), xy_ep)
REGISTER_ACTION((Builder<SinQ<1>, CosQ<1>>::F), yx_ep)
REGISTER_ACTION((Builder<SinQ<1>, SinQ<1>>::F), yy_ep)

}  // namespace Correlation::Action::Library


#endif  // DATATREEFLOW_SRC_CORRELATION_CORRELATIONACTION_H
