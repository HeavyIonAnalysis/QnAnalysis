//
// Created by eugene on 02/10/2020.
//

#ifndef QNANALYSIS_SRC_QNANALYSISCORRELATE_CONFIG_HPP
#define QNANALYSIS_SRC_QNANALYSISCORRELATE_CONFIG_HPP

#include "enum.h"
#include <yaml-cpp/yaml.h>

namespace Qn::Analysis::Correlate {

BETTER_ENUM(EQnWeight, int, OBSERVABLE, REFERENCE)

struct Axis {

};


struct YAMLQuery {

};

struct CorrelationTaskArgument {
  YAMLQuery query;
  EQnWeight weight;
};

/* list of arguments, list of actions to apply */
struct CorrelationTask {
  std::vector<CorrelationTaskArgument> arguments;
  std::vector<std::string> actions;
};


}

namespace YAML {

/**
 * @brief Wrapper to convert better-enums
 * @tparam T enumerator
 */
template <typename T>
struct Enum{
  std::optional<T> opt_enum;
  /* default constructor is required for decode */
  Enum() = default;
  Enum(const T &v) { opt_enum = v; }
  Enum(typename T::_integral v) { opt_enum = T::_from_integral(v); }
  operator T const& () const {
    return opt_enum.value();
  }
};

/**
 * @brief Universal case-insensitive encoder-decoder for better-enums
 * @tparam T enumerator
 */
template<typename T>
struct convert<Enum<T>> {
  static bool decode(const Node& node, Enum<T>& e) {
    if (node.IsScalar()) {
      if (!T::_is_valid_nocase(node.Scalar().c_str())) {
        return false;
      }
      e.opt_enum = T::_from_string_nocase(node.Scalar().c_str());
      return true;
    }
    return false;
  }

  static Node encode(const Enum<T>& e) {
    Node result;
    result = e.opt_enum.value()._to_string();
    return result;
  }
};


}

#endif //QNANALYSIS_SRC_QNANALYSISCORRELATE_CONFIG_HPP
