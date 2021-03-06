//
// Created by eugene on 10/12/2020.
//

#ifndef QNANALYSIS_SRC_QNANALYSISOBSERVABLES_RESOURCEMANAGER_HPP
#define QNANALYSIS_SRC_QNANALYSISOBSERVABLES_RESOURCEMANAGER_HPP

#include <iostream>
#include <map>
#include <set>
#include <tuple>
#include <functional>
#include <string>
#include <utility>
#include <vector>
#include <any>
#include <memory>
#include <filesystem>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include <TError.h>

namespace Details {

template<typename T>
class Singleton {

 public:
  virtual ~Singleton() = default;

  static T &Instance() {
    static T instance;
    return instance;
  }

};

template<typename T>
struct FunctionTraits {};

template<typename R, typename ...Args>
struct FunctionTraits<std::function<R(Args...)>> {
  using ReturnType = R;

  enum { N_ARGS = sizeof...(Args) };

  template<size_t I>
  using ArgType = std::tuple_element_t<I, std::tuple<Args...>>;

  using ArgumentsTuple = std::tuple<std::decay_t<Args>...>;
};

}

namespace Details {

template<typename KeyRepr>
struct Convert;

template<>
struct Convert<std::string> {
  static std::string FromString(const std::string &str) { return str; }
  static std::string ToString(const std::string &str) { return str; }
};

template<>
struct Convert<std::vector<std::string>> {
  static std::vector<std::string> FromString(const std::string &str) {
    std::filesystem::path p(str);
    std::vector<std::string> result;
    for (auto &path_element : p)
      if (path_element != "/")
        result.emplace_back(path_element.string());
    return result;
  }
  static std::string ToString(const std::vector<std::string> &key) {
    std::string result;
    for (auto &key_ele : key)
      result.append("/").append(key_ele);
    return result;
  }
};

}

typedef std::string StringKey;
typedef std::vector<std::string> VectorKey;

class ResourceManager : public Details::Singleton<ResourceManager> {
 public:

  struct Resource; /// fwd
  typedef boost::property_tree::ptree MetaType;
  typedef std::shared_ptr<Resource> ResourcePtr;
  typedef std::string KeyType;

  struct Resource {
    Resource() = default;
    Resource(std::string n, std::any &&o, const MetaType &m) : name(std::move(n)), obj(o), meta(m) {}
    template <typename T>
    Resource(T&& typed_obj, const MetaType &Meta) : obj(typed_obj), meta(Meta) {}

    std::string name;
    std::any obj;
    MetaType meta;

    template<typename T>
    T &As() {
      return std::any_cast<T &>(obj);
    }

    template<typename T>
    const T &As() const {
      return std::any_cast<const T &>(obj);
    }

    template<typename T>
    T *Ptr() {
      return std::any_cast<T>(&obj);
    }

    void Print(std::ostream &os = std::cout) const {
      os << "name:" << name << "\t" << "meta:";
      boost::property_tree::write_json(os, meta, false);
    }
  };

  struct NameTag {
    explicit NameTag(std::string name) : name(std::move(name)) {}

    std::string name;

    operator std::string() const {
      return name;
    }
  };

  template<typename T>
  struct ResTag {};

  struct AlwaysTrue {
    bool operator()(const Resource &) {
      return true;
    }
  };

  struct ResourceAlreadyExists : public std::exception {
    explicit ResourceAlreadyExists(std::string resource_name_) : resource_name(std::move(resource_name_)) {}
    ResourceAlreadyExists(const ResourceAlreadyExists &other) = default;
    const char *what() const
    _GLIBCXX_TXN_SAFE_DYN _GLIBCXX_NOTHROW
    override {
      return resource_name.c_str();
    }
    std::string resource_name;
  };

  struct NoSuchResource : public std::exception {
    explicit NoSuchResource(std::string resource_name_) : resource_name(std::move(resource_name_)) {}
    NoSuchResource(const NoSuchResource &Exception) = default;
    const char *what() const
    _GLIBCXX_TXN_SAFE_DYN _GLIBCXX_NOTHROW
    override {
      return resource_name.c_str();
    }

    std::string resource_name;
  };

  ResourcePtr Add(Resource resource) {
    auto key = resource.name;
    auto emplace_result = resources_.emplace(key, std::make_shared<Resource>(resource));
    if (!emplace_result.second) {
      throw ResourceAlreadyExists(key);
    }
    return (emplace_result.first)->second;
  }

  template<typename KeyRepr, typename T>
  auto Add(const KeyRepr &key, T &&obj, MetaType m = MetaType()) {
    static_assert(std::is_copy_constructible_v<T>, "T must be copy-constructable to be stored in ResourceManager");
    return Add(Resource(Details::Convert<KeyRepr>::ToString(key), std::forward<T>(obj), std::move(m)));
  }

  template<typename KeyRepr, typename T>
  auto Add(const KeyRepr &key, T *ptr, MetaType m = MetaType()) {
    if (!ptr) {
      throw std::runtime_error("nullptr");
    }
    return Add(key, *ptr, std::move(m));
  }

  template<typename KeyRepr>
  bool Has(const KeyRepr &key) const {
    auto it = resources_.find(Details::Convert<KeyRepr>::ToString(key));
    return it != resources_.end();
  }

  template<typename KeyRepr>
  void CheckResource(const KeyRepr &key) {
    if (!Has(key)) {
      throw NoSuchResource(key);
    }
  }

  template<typename KeyRepr, typename T>
  T &Get(const KeyRepr &key, ResTag<T>) {
    CheckResource(key);
    return std::any_cast<std::add_lvalue_reference_t<T>>(
        (resources_[Details::Convert<KeyRepr>::ToString(key)]->obj));
  }

  template<typename KeyRepr>
  NameTag Get(const KeyRepr &key, ResTag<NameTag>) {
    CheckResource(key);
    return NameTag(Details::Convert<KeyRepr>::ToString(key));
  }

  template<typename KeyRepr>
  MetaType &Get(const KeyRepr &key, ResTag<MetaType>) {
    CheckResource(key);
    return resources_[Details::Convert<KeyRepr>::ToString(key)]->meta;
  }

  template<typename KeyRepr>
  Resource &Get(const KeyRepr &key, ResTag<Resource>) {
    CheckResource(key);
    return *resources_[Details::Convert<KeyRepr>::ToString(key)];
  }

  template<typename Predicate>
  bool TestPredicate(Predicate &&predicate, const std::string &key) {
    const auto &resource = Get(key, ResTag<Resource>());
    static_assert(std::is_same_v<decltype(predicate(resource)), bool>);
    return predicate(resource);
  }

  template<typename Predicate = AlwaysTrue>
  std::vector<std::string> GetMatching(Predicate &&predicate = AlwaysTrue()) {
    std::vector<std::string> result;
    for (auto &element : resources_) {
      if (TestPredicate(std::forward<Predicate>(predicate), element.first))
        result.emplace_back(element.first);
    }
    return result;
  }

  template<typename Function, typename Predicate = AlwaysTrue>
  void ForEach(Function &&fct, Predicate &&predicate = AlwaysTrue(), bool warn_bad_cast = false) {
    using Traits = Details::FunctionTraits<decltype(std::function{fct})>;

    auto resources_copy = resources_;
    for (auto &element : resources_copy) {
      if (TestPredicate(std::forward<Predicate>(predicate), element.first)) {
        try {
          static_assert(Traits::N_ARGS == 2);
          using KeyRepr = std::decay_t<typename Traits::template ArgType<0>>;
          using ArgType = std::decay_t<typename Traits::template ArgType<1>>;
          fct(Details::Convert<KeyRepr>::FromString(element.first) /* pass by value to prevent from unintentional change */,
              Get(element.first, ResTag<ArgType>()));
        } catch (std::bad_any_cast &e) {
          if (warn_bad_cast)
            Warning(__func__, "Bad cast for '%s'. Skipping...", element.first.c_str());
        }
      } // predicate
    }
  }

  template<typename MapFunction, typename OIter, typename Predicate = AlwaysTrue>
  void SelectImpl(MapFunction &&fct, OIter &&o, Predicate &&predicate = AlwaysTrue()) {
    auto resources_copy = resources_;
    for (auto &element : resources_copy) {
      if (TestPredicate(std::forward<Predicate>(predicate), element.first)) {
          *o = fct(Get(element.first, ResTag<Resource>()));
      } /// predicate
    } /// resources
  }

  template<typename MapFunction, typename Predicate = AlwaysTrue>
  auto SelectUniq(MapFunction &&feature_function, Predicate &&predicate = AlwaysTrue()) {
    using FeatureType = std::decay_t<decltype(feature_function(std::declval<const Resource&>()))>;
    std::set<FeatureType> features_set;
    SelectImpl(std::forward<MapFunction>(feature_function),
           std::inserter(features_set, std::begin(features_set)),
           std::forward<Predicate>(predicate));
    /* copy to the vector */
    std::vector<FeatureType> features_vector;
    features_vector.reserve(features_set.size());
    std::copy(std::begin(features_set), std::end(features_set), std::back_inserter(features_vector));
    return features_vector;
  }

  template<typename FeatureExtractor, typename ApplyFunct, typename Predicate = AlwaysTrue>
  auto GroupBy(FeatureExtractor && feature_fct, ApplyFunct && funct, Predicate && predicate = AlwaysTrue()) {
    using FeatureType = std::decay_t<decltype(feature_fct(std::declval<const ResourceManager::Resource&>()))>;

    std::unordered_map<FeatureType,std::vector<ResourcePtr>> feature_groups;
    /* collecting features */
    auto resources_copy = resources_;
    for (auto &element : resources_copy) {
      if (TestPredicate(std::forward<Predicate>(predicate), element.first)) {
        auto feature = feature_fct(Get(element.first, ResTag<Resource>()));

        auto && [emplace_it, emplace_ok] =
            feature_groups.template emplace(std::move(feature), std::vector<ResourcePtr>({element.second}));
        if (!emplace_ok) { /* already in the map */
          emplace_it->second.emplace_back(element.second);
        }
      }
    } // resources

    for (auto &[feature, elements] : feature_groups) {
      funct(feature, elements);
    }
  }

  void Print() {
    std::cout << "Keys: " << std::endl;
    for (auto &element : resources_) {
      element.second->Print(std::cout);
    }
  }

 private:

  std::map<KeyType, ResourcePtr>
      resources_; /// Pointers are used to allow simultaneous iteration and definition new objects
};

#define gResourceManager (ResourceManager::Instance())

template<typename KeyRepr, typename T>
auto AddResource(const KeyRepr &key, T &&ref) {
  return ResourceManager::Instance().Add(key, std::forward<T>(ref));
}

template<typename KeyRepr>
auto AddResource(const KeyRepr &key, ResourceManager::Resource res) {
  std::string name = Details::Convert<KeyRepr>::ToString(key);
  if (!res.name.empty()) {
    Warning(__func__, "Name of resource '%s' will be replaced to '%s'",
            res.name.c_str(),
            name.c_str());
  }
  res.name = name;
  return ResourceManager::Instance().Add(res);
}

template<typename T>
auto AddResource(const std::string &name, T *ref) {
  return ResourceManager::Instance().Add(name, ref);
}
#endif //QNANALYSIS_SRC_QNANALYSISOBSERVABLES_RESOURCEMANAGER_HPP
