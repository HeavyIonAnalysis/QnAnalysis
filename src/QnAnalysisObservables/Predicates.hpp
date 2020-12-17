//
// Created by eugene on 10/12/2020.
//

#ifndef QNANALYSIS_SRC_QNANALYSISOBSERVABLES_PREDICATES_HPP
#define QNANALYSIS_SRC_QNANALYSISOBSERVABLES_PREDICATES_HPP

#include <string>
#include <regex>
#include <boost/proto/proto.hpp>

namespace Predicates {

auto AlwaysTrue = [](const std::string &) -> bool { return true; };

struct RegexMatch {
  explicit RegexMatch(const std::string &regex_str) : expr(regex_str) {}
  explicit RegexMatch(std::regex expr) : expr(std::move(expr)) {}

  bool operator () (const std::string &str) const {
    return std::regex_match(str, expr);
  }

  const std::regex expr;
};

} /// namespace Predicates

#endif //QNANALYSIS_SRC_QNANALYSISOBSERVABLES_PREDICATES_HPP
