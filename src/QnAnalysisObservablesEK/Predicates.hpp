//
// Created by eugene on 10/12/2020.
//

#ifndef QNANALYSIS_SRC_QNANALYSISOBSERVABLES_PREDICATES_HPP
#define QNANALYSIS_SRC_QNANALYSISOBSERVABLES_PREDICATES_HPP

#include <string>
#include <regex>
#include <boost/proto/proto.hpp>
#include <utility>
#include <boost/regex.hpp>

namespace Predicates {

template<typename Expr>
struct ResourceQueryExpr; /// fwd declaration


struct ResourceExprDomain : boost::proto::domain<boost::proto::generator<ResourceQueryExpr>> {};

namespace Impl {

struct RegexMatchImpl {
  explicit RegexMatchImpl(std::string regex_str) : re_expr(regex_str) {}

  typedef bool result_type;

  result_type operator()(const std::string &str) const {
    return boost::regex_match(str, re_expr);
  }

  const boost::regex re_expr;
};

struct MatchGroupImpl {
  MatchGroupImpl(const size_t group_id, std::regex re_expr) : group_id(group_id), re_expr(std::move(re_expr)) {}
  MatchGroupImpl(const size_t group_id, const std::string &re_expr) : group_id(group_id), re_expr(re_expr) {}

  typedef std::string result_type;

  result_type operator()(const std::string &str) const {
    std::smatch match_result;
    auto is_matched = std::regex_search(str, match_result, re_expr);
    if (is_matched && group_id < match_result.size()) {
      return match_result.str(group_id);
    }

    return "NOT-FOUND";
  }

  const size_t group_id;
  const std::regex re_expr;
};

struct BaseOfImpl {
  typedef std::string result_type;

  result_type operator()(const std::string &str) const {
    std::filesystem::path path(str);
    return path.parent_path().string();
  }
};

} /// namespace Details

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

  inline
  result_type operator() (const ResourceManager::ResourcePtr &resource_ptr) const {
    return operator()(*resource_ptr);
  }

  template<typename Regex>
  auto Matches(Regex &&re_expr) const {
    namespace proto = boost::proto;
    return proto::make_expr<proto::tag::function>(
        Impl::RegexMatchImpl(re_expr), boost::ref(*this));
  }

  template<typename Regex>
  auto MatchGroup(std::size_t id, Regex &&re_expr) const {
    namespace proto = boost::proto;
    return proto::make_expr<proto::tag::function>(Impl::MatchGroupImpl(id, re_expr), boost::ref(*this));
  }

};

namespace Resource {

ResourceQueryExpr<boost::proto::terminal<MetaTag>::type> const META;

ResourceQueryExpr<boost::proto::terminal<KeyTag>::type> const KEY;

template<typename Arg>
typename boost::proto::result_of::make_expr<
    boost::proto::tag::function,
    Impl::BaseOfImpl,
    Arg const &>::type const
BASE_OF(Arg const &arg) {
  return boost::proto::make_expr<boost::proto::tag::function>(Impl::BaseOfImpl(), boost::ref(arg));
}

} /// namespace Resource

template<typename Expr>
typename ResourceQueryExpr<Expr>::result_type ResourceQueryExpr<Expr>::operator()(const ResourceManager::Resource &r) const {
  ResourceContext ctx(r);
  return boost::proto::eval(*this, ctx);
}

namespace Impl {

inline
std::string JoinStrings(const std::vector<std::string> &strings, std::string delim = "__") {
  assert(!strings.empty());
  if (strings.size() == 1)
    return strings[0];

  auto result = strings.front();
  for (std::size_t is = 1; is < strings.size(); ++is) {
    result.append(delim).append(strings[is]);
  }
  return result;
}
} // namespace Impl


struct MetaFeatureSet {

  typedef std::string result_type;

  MetaFeatureSet(std::initializer_list<std::string> feature_paths) :
      meta_feature_paths_(feature_paths.begin(), feature_paths.end()) {}
  explicit MetaFeatureSet(const std::vector<std::string> &meta_features) :
      meta_feature_paths_(meta_features) {}

  result_type operator()(const ResourceManager::Resource &r) const {
    std::vector<std::string> feature_list;
    feature_list.reserve(meta_feature_paths_.size());
    for (auto &meta_feature_path: meta_feature_paths_) {
      feature_list.push_back(Resource::META[meta_feature_path](r));
    }
    return Impl::JoinStrings(feature_list);
  }

  MetaFeatureSet operator-(const std::string &feature) const {
    auto new_feature_set = meta_feature_paths_;
    auto feature_pos = std::find(new_feature_set.begin(), new_feature_set.end(), feature);
    assert(feature_pos != new_feature_set.end());
    new_feature_set.erase(feature_pos);
    return MetaFeatureSet(new_feature_set);
  }

  MetaFeatureSet operator+(const std::string &feature) const {
    assert(std::find(meta_feature_paths_.begin(), meta_feature_paths_.end(), feature) == meta_feature_paths_.end());
    auto new_feature_set = meta_feature_paths_;
    new_feature_set.emplace_back(feature);
    return MetaFeatureSet(new_feature_set);
  }

  std::vector<std::string> meta_feature_paths_;

};

struct MetaTemplateGenerator {

  typedef std::string result_type;

  explicit MetaTemplateGenerator(std::string template_str) : template_str(std::move(template_str)) {}

  result_type operator()(const ResourceManager::Resource &r) const {
    const std::regex re_meta_token{R"(\{\{([\w\.-]+)\}\})"};
    using std::sregex_iterator;
    using std::smatch;
    using Resource::META;

    auto result = template_str;
    std::string::difference_type offset = 0;

    for (
        auto it = sregex_iterator(template_str.begin(), template_str.end(), re_meta_token);
        it != sregex_iterator();
        ++it) {
      smatch match = *it;
      auto meta_path = match.str(1);
      auto meta_value = META[meta_path](r);

      auto replace_start = match.position(0) + offset;
      auto replace_length = match.length();
      result.replace(replace_start, replace_length, meta_value);
      offset += meta_value.length() - replace_length;
    }
    return result;
  }

  inline
  result_type operator() (const ResourceManager::ResourcePtr& resource_ptr) const {
    return operator()(*resource_ptr);
  }

  std::string template_str;
};

} /// namespace Predicates

#endif //QNANALYSIS_SRC_QNANALYSISOBSERVABLES_PREDICATES_HPP
