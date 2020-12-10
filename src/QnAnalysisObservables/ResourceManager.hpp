//
// Created by eugene on 10/12/2020.
//

#ifndef QNANALYSIS_SRC_QNANALYSISOBSERVABLES_RESOURCEMANAGER_HPP
#define QNANALYSIS_SRC_QNANALYSISOBSERVABLES_RESOURCEMANAGER_HPP

#include <tuple>
#include <functional>
#include <string>
#include <utility>
#include <vector>
#include <any>
#include <memory>

#include <boost/property_tree/ptree.hpp>

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

class ResourceManager : public Details::Singleton<ResourceManager> {
public:
  struct Resource; /// fwd
  typedef boost::property_tree::ptree MetaType;
  typedef std::shared_ptr<Resource> ResourcePtr;

  struct Resource {
    Resource(std::any o, const MetaType &m) : obj(std::move(o)), meta(m) {}
    std::any obj;
    MetaType meta;
  };

  template <typename T>
  struct ResTag {};


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

  typedef std::string KeyType;

  template<typename KeyRepr, typename T>
  void Add(const KeyRepr &key, T &&obj, MetaType m = MetaType()) {
    static_assert(std::is_copy_constructible_v<T>, "T must be copy-constructable to be stored in ResourceManager");
    auto emplace_result = resources_.template emplace(
        Details::Convert<KeyRepr>::ToString(key),
        std::make_shared<Resource>(obj, m));
    if (!emplace_result.second) {
      throw ResourceAlreadyExists(Details::Convert<KeyRepr>::ToString(key));
    }
  }

  template<typename KeyRepr, typename T>
  void Add(const KeyRepr &key, T *ptr, MetaType m = MetaType()) {
    Add(key, *ptr, m);
  }

  template<typename KeyRepr>
  bool Has(const KeyRepr &key) const {
    auto it = resources_.find(Details::Convert<KeyRepr>::ToString(key));
    return it != resources_.end();
  }

  template<typename KeyRepr, typename T>
  T &Get(const KeyRepr &key, ResTag<T>) {
    if (!Has(key)) {
      throw NoSuchResource(key);
    }
    return std::any_cast<std::add_lvalue_reference_t<T>>(
        (resources_[Details::Convert<KeyRepr>::ToString(key)]->obj));
  }

  template<typename KeyRepr>
  MetaType &Get(const KeyRepr& key, ResTag<MetaType>) {
    if (!Has(key)) {
      throw NoSuchResource(key);
    }
    return resources_[Details::Convert<KeyRepr>::ToString(key)]->meta;
  }

  template<typename Function, typename Predicate = decltype(Predicates::AlwaysTrue)>
  void ForEach(Function &&fct, Predicate predicate = Predicates::AlwaysTrue, bool warn_bad_cast = true) {
    using Traits = Details::FunctionTraits<decltype(std::function{fct})>;

    auto resources_copy = resources_;
    for (auto &element : resources_copy) {
      if (predicate(element.first)) {
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

  void Print() {
    std::cout << "Keys: " << std::endl;
    for (auto &element : resources_) {
      std::cout << "\t" << element.first << std::endl;
    }
  }

private:

  std::map<KeyType, ResourcePtr> resources_; /// Pointers are used to allow simultaneous iteration and definition new objects
};

#endif //QNANALYSIS_SRC_QNANALYSISOBSERVABLES_RESOURCEMANAGER_HPP
