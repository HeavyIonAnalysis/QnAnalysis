//
// Created by eugene on 10/12/2020.
//

#ifndef QNANALYSIS_SRC_QNANALYSISOBSERVABLES_PREDICATES_HPP
#define QNANALYSIS_SRC_QNANALYSISOBSERVABLES_PREDICATES_HPP

#include <string>
#include <regex>
#include <boost/proto/proto.hpp>
#include <utility>

namespace Predicates {

template<typename Expr>
struct ResourceExpr;

struct ResourceExprDomain : boost::proto::domain<boost::proto::generator<ResourceExpr>> {};

namespace Resource {

struct KeyTag {};

struct MetaTag {};

} // namespace Resource

template<typename Expr>
struct ResourceExpr {

  BOOST_PROTO_BASIC_EXTENDS(Expr, ResourceExpr<Expr>, ResourceExprDomain);

  BOOST_PROTO_EXTENDS_SUBSCRIPT_CONST();

  typedef bool result_type;

  ResourceExpr(Expr proto_expr = Expr()) : proto_expr_(proto_expr) {}

  result_type operator()(const ResourceManager::Resource &r) const;

};

namespace Resource {

ResourceExpr<boost::proto::terminal<KeyTag>::type> const KEY;

ResourceExpr<boost::proto::terminal<MetaTag>::type> const META;

}

struct ResourceContext {

  typedef std::string result_type;

  const ResourceManager::Resource &res_;

  explicit ResourceContext(const ResourceManager::Resource &res) : res_(res) {}

  template<
      typename Expr, typename Tag = typename boost::proto::tag_of<Expr>::type>
  struct eval : boost::proto::default_eval<Expr, ResourceContext const, Tag> {};

  template<
      typename Expr,
      typename Arg0 = typename boost::proto::result_of::value<Expr>::type>
  struct eval_terminal : boost::proto::default_eval<Expr, ResourceContext const> {};

  template<typename Expr>
  struct eval_terminal<Expr, Resource::KeyTag> {
    typedef std::string result_type;

    result_type operator()(Expr &e, const ResourceContext &ctx) const {
      return ctx.res_.name;
    }
  };

  template<typename Expr>
  struct eval<Expr, boost::proto::tag::terminal> : eval_terminal<Expr> {};

  template<
      typename Expr, typename ArgLeft = typename boost::proto::result_of::left<Expr>::type>
  struct eval_subscript : boost::proto::default_eval<Expr, ResourceContext const> {};

  template<typename Expr>
  struct eval_subscript<Expr, std::remove_const_t<decltype(Resource::META)>> {
    typedef std::string result_type;

    result_type operator() (Expr& e, const ResourceContext& ctx) const {
      auto path = std::string(boost::proto::value(boost::proto::right(e)));
      auto result = ctx.res_.meta.get<result_type>(path, path+"-NOT-FOUND");
      return result;
    }
  };


  template<typename Expr>
  struct eval<Expr, boost::proto::tag::subscript> : eval_subscript<Expr> {};

};

template<typename Expr>
bool ResourceExpr<Expr>::operator()(const ResourceManager::Resource &r) const {
  ResourceContext ctx(r);
  return boost::proto::eval(*this, ctx);
}

struct RegexMatch {
  explicit RegexMatch(const std::string &regex_str) : expr(regex_str) {}
  explicit RegexMatch(std::regex expr) : expr(std::move(expr)) {}

  bool operator()(const ResourceManager::NameTag &name) const {
    return std::regex_match(static_cast<const std::string &>(name), expr);
  }

  const std::regex expr;
};

} /// namespace Predicates

#endif //QNANALYSIS_SRC_QNANALYSISOBSERVABLES_PREDICATES_HPP
