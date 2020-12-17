//
// Created by eugene on 10/12/2020.
//

#ifndef QNANALYSIS_SRC_QNANALYSISOBSERVABLES_PREDICATES_HPP
#define QNANALYSIS_SRC_QNANALYSISOBSERVABLES_PREDICATES_HPP

#include <string>
#include <regex>
#include <boost/proto/proto.hpp>

namespace Predicates {

namespace Resource {
  struct NameTag {};

  constexpr static boost::proto::terminal<NameTag>::type const NAME = {{}};
}

struct ResourceContext : boost::proto::callable_context<ResourceContext const> {
  typedef std::string result_type;

  const ResourceManager::Resource &res_;

  explicit ResourceContext(const ResourceManager::Resource &res) : res_(res) {}

  result_type operator () (boost::proto::tag::terminal, Resource::NameTag) const {
    return res_.name;
  }

};

template <typename Expr>
struct Builder {

  explicit Builder(Expr && expr) : expr(expr) {}

  bool operator () (const ResourceManager::Resource& res) {
    ResourceContext ctx(res);
    return boost::proto::eval(expr, ctx);
  }

  Expr expr;
};



struct RegexMatch {
  explicit RegexMatch(const std::string &regex_str) : expr(regex_str) {}
  explicit RegexMatch(std::regex expr) : expr(std::move(expr)) {}

  bool operator () (const ResourceManager::NameTag &name) const {
    return std::regex_match(static_cast<const std::string&>(name), expr);
  }

  const std::regex expr;
};

} /// namespace Predicates

#endif //QNANALYSIS_SRC_QNANALYSISOBSERVABLES_PREDICATES_HPP
