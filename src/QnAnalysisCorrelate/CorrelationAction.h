//
// Created by eugene on 04/08/2020.
//

#ifndef DATATREEFLOW_SRC_CORRELATION_CORRELATIONACTION_H
#define DATATREEFLOW_SRC_CORRELATION_CORRELATIONACTION_H

#include <functional>

#include <QnTools/QnDataFrame.hpp>

#include "StaticRegistry.h"

#ifndef REGISTER_ACTION_RAWFUN
#define REGISTER_ACTION_RAWFUN(FUNCTION, NAME)                                \
  namespace {                                                                 \
  class ActionWrapper_##NAME                                                  \
      : public ::Correlation::Action::BaseActionWrapper<decltype(FUNCTION)> { \
   public:                                                                    \
    FunctionWrap GetWrapped() noexcept override { return &(FUNCTION); }       \
                                                                              \
   private:                                                                   \
    static size_t flag;                                                       \
  };                                                                          \
  size_t ActionWrapper_##NAME::flag = ::Correlation::Register(                \
      std::string(#NAME), ActionWrapper_##NAME().GetWrapped());               \
  }
#endif

namespace Correlation::Action {

namespace Details {

template <size_t I>
using ActionIndexedArgument_t = const Qn::QVector&;

template <size_t... I>
auto GetActionRegistryImpl(std::index_sequence<I...>) {
  using FunctionWrapSignature =
      std::function<float(ActionIndexedArgument_t<I>...)>;
  return StaticRegistry<FunctionWrapSignature, std::string>::Instance();
}

}  // namespace Details

template <typename Function>
class BaseActionWrapper {
 public:
  typedef decltype(std::function(
      std::declval<typename std::add_pointer_t<Function>>())) FunctionWrap;

  virtual FunctionWrap GetWrapped() noexcept = 0;
};

struct X {};
struct Cos{};
struct Y {};
struct Sin{};

template <typename Component, size_t Harmonic>
struct Q {
  static inline float Eval(const Qn::QVector& v) {
    return EvalImpl(v, Component());
  }

  static inline float EvalImpl(const Qn::QVector& v, X) {
    return v.x(Harmonic);
  }
  static inline float EvalImpl(const Qn::QVector& v, Y) {
    return v.y(Harmonic);
  }
  static inline float EvalImpl(const Qn::QVector& v, Cos) {
    return v.x(Harmonic) / v.mag(Harmonic);
  }
  static inline float EvalImpl(const Qn::QVector& v, Sin) {
    return v.y(Harmonic) / v.mag(Harmonic);
  }
};

template <size_t Harmonic>
using Qx = Q<X, Harmonic>;

template <size_t Harmonic>
using CosQ = Q<Cos, Harmonic>;

using Qx1 = Qx<1>;
using Qx2 = Qx<2>;
using Qx3 = Qx<3>;

template <size_t Harmonic>
using Qy = Q<Y, Harmonic>;

template <size_t Harmonic>
using SinQ = Q<Sin, Harmonic>;

using Qy1 = Qy<1>;
using Qy2 = Qy<2>;
using Qy3 = Qy<3>;

template <typename... Q>
struct Builder {
  template <typename T>
  using Arg_t = const Qn::QVector&;

  static inline float F(Arg_t<Q>... arg) {
    return EvalImpl<Q...>(std::forward<Arg_t<Q>>(arg)...);
  }

  template <typename Q1, typename... Qn>
  static inline float EvalImpl(Arg_t<Q1> arg1, Arg_t<Qn>... args) {
    return Q1::Eval(arg1) * EvalImpl<Qn...>(std::forward<Arg_t<Qn>>(args)...);
  }

  template <typename Q1>
  static inline float EvalImpl(Arg_t<Q1> arg1) {
    return Q1::Eval(arg1) * 1.;
  }
};

template <size_t Arity>
auto GetRegistry() {
  return Details::GetActionRegistryImpl(std::make_index_sequence<Arity>());
}

}  // namespace Correlation::Action

namespace Correlation::Action::Library {

/* Two-particle correlations */
template <size_t Harmonic>
float ScalarProduct(const Qn::QVector& q1, const Qn::QVector& q2) {
    return Qn::ScalarProduct(q1, q2, Harmonic);
}
REGISTER_ACTION_RAWFUN(ScalarProduct<1>, scalar_product);

REGISTER_ACTION_RAWFUN((Builder<Qx1, Qx1>::F), xx)
REGISTER_ACTION_RAWFUN((Builder<Qx1, Qx1>::F), x1x1) // Alias!
REGISTER_ACTION_RAWFUN((Builder<Qx1, Qy1>::F), xy)
REGISTER_ACTION_RAWFUN((Builder<Qx1, Qy1>::F), x1y1)
REGISTER_ACTION_RAWFUN((Builder<Qy1, Qx1>::F), yx)
REGISTER_ACTION_RAWFUN((Builder<Qy1, Qx1>::F), y1x1)
REGISTER_ACTION_RAWFUN((Builder<Qy1, Qy1>::F), yy)
REGISTER_ACTION_RAWFUN((Builder<Qy1, Qy1>::F), y1y1)

REGISTER_ACTION_RAWFUN((Builder<CosQ<1>,CosQ<1>>::F), xx_ep)
REGISTER_ACTION_RAWFUN((Builder<CosQ<1>,SinQ<1>>::F), xy_ep)
REGISTER_ACTION_RAWFUN((Builder<SinQ<1>,CosQ<1>>::F), yx_ep)
REGISTER_ACTION_RAWFUN((Builder<SinQ<1>,SinQ<1>>::F), yy_ep)

}  // namespace Correlation::Action::Library


#endif  // DATATREEFLOW_SRC_CORRELATION_CORRELATIONACTION_H
