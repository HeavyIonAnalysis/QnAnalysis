//
// Created by eugene on 04/08/2020.
//

#ifndef DATATREEFLOW_SRC_CORRELATION_STATICREGISTRY_H
#define DATATREEFLOW_SRC_CORRELATION_STATICREGISTRY_H

#include <map>
#include <vector>

namespace Qn::Analysis::Correlate {

namespace Details {

template <typename T>
class Singleton {
 public:
  virtual ~Singleton<T>() = default;

  static T& Instance() {
    static T* instance = nullptr;

    if (!instance) {
      instance = new T;
    }

    return *instance;
  }
};

}  // namespace Details

template <typename ObjectType, typename KeyType>
class StaticRegistry
    : public Details::Singleton<StaticRegistry<ObjectType, KeyType>> {
 public:
  size_t Emplace(const KeyType& key, ObjectType&& obj) {
    objects.emplace(key, std::forward<ObjectType>(obj));
    return objects.size();
  }

  ObjectType& Get(const KeyType& key) { return objects.at(key); }

  std::vector<KeyType> ListKeys() const {
    std::vector<KeyType> result;
    for (auto & o : objects) {
      result.push_back(o.first);
    }
    return result;
  }



 private:
  std::map<KeyType, ObjectType> objects;
};

template <typename K, typename V>
size_t Register(const K& k, V&& v) {
  return StaticRegistry<V, K>::Instance().Emplace(k, std::forward<V>(v));
}

}  // namespace Correlation

#endif  // DATATREEFLOW_SRC_CORRELATION_STATICREGISTRY_H
