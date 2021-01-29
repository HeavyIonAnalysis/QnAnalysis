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
struct ResourceQueryExpr; /// fwd declaration


struct ResourceExprDomain : boost::proto::domain<boost::proto::generator<ResourceQueryExpr>> {};

namespace Details {

struct RegexMatchImpl {
  explicit RegexMatchImpl(const std::string &regex_str) : re_expr(regex_str) {}
  explicit RegexMatchImpl(std::regex expr) : re_expr(std::move(expr)) {}

  typedef bool result_type;

  result_type operator()(const std::string &str) const {
    return std::regex_match(str, re_expr);
  }

  const std::regex re_expr;
};

} /// namespace Details

template<typename Arg, typename Regex>
typename boost::proto::result_of::make_expr<
    boost::proto::tag::function,
    Details::RegexMatchImpl,
    Arg const &
>::type const
IsMatchesRegex(Arg const &arg, Regex && regex_string) {
  namespace proto = boost::proto;
  return proto::make_expr<proto::tag::function>(
      Details::RegexMatchImpl(regex_string), boost::ref(arg));
}

namespace Resource {

struct KeyTag {};

struct MetaTag {};

} // namespace Resource

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
  struct eval_subscript<Expr, ResourceQueryExpr<boost::proto::terminal<Resource::MetaTag>::type>> {
    typedef std::string result_type;

    result_type operator()(Expr &e, const ResourceContext &ctx) const {
      auto path = std::string(boost::proto::value(boost::proto::right(e)));
      auto result = ctx.res_.meta.get<result_type>(path, path + "-NOT-FOUND");
      return result;
    }
  };

  template<typename Expr>
  struct eval<Expr, boost::proto::tag::subscript> : eval_subscript<Expr> {};

};

template<typename Expr>
struct ResourceQueryExpr {

  BOOST_PROTO_BASIC_EXTENDS(Expr, ResourceQueryExpr<Expr>, ResourceExprDomain);

  BOOST_PROTO_EXTENDS_SUBSCRIPT_CONST();

  typedef typename boost::proto::result_of::eval<Expr, ResourceContext>::type result_type;

  ResourceQueryExpr(Expr proto_expr = Expr()) : proto_expr_(proto_expr) {}

  result_type operator()(const ResourceManager::Resource &r) const;

  template <typename Regex>
  auto Matches(Regex && re_expr) const {
    return IsMatchesRegex(*this, std::forward<Regex>(re_expr));
  }

};

namespace Resource {

ResourceQueryExpr<boost::proto::terminal<KeyTag>::type> const KEY;

ResourceQueryExpr<boost::proto::terminal<MetaTag>::type> const META;

}

template<typename Expr>
typename ResourceQueryExpr<Expr>::result_type ResourceQueryExpr<Expr>::operator()(const ResourceManager::Resource &r) const {
  ResourceContext ctx(r);
  return boost::proto::eval(*this, ctx);
}


} /// namespace Predicates

#endif //QNANALYSIS_SRC_QNANALYSISOBSERVABLES_PREDICATES_HPP
