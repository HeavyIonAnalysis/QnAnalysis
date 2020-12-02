//
// Created by eugene on 24/11/2020.
//

#include <DataContainer.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/proto/proto.hpp>

#include <filesystem>
#include <utility>
#include <map>
#include <tuple>

#include <TFile.h>
#include <TKey.h>
#include <TDirectory.h>
#include <TClass.h>

#include <QnAnalysisTools/QnAnalysisTools.hpp>

#include <DataContainer.hpp>
#include <StatCollect.hpp>

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
class ResourceManager : public Singleton<ResourceManager<T>> {
public:
  typedef std::string KeyType;
  typedef T ResourceType;
  typedef T *PointerType;
  typedef T &ReferenceType;

  struct resource_exists_exception : public std::exception {
    explicit resource_exists_exception(const char *ResourceName) : resource_name(ResourceName) {}
    resource_exists_exception(const resource_exists_exception &other) = default;
    const char *resource_name;
  };

  void Add(const KeyType &key, PointerType pointer) {
    auto emplace_result = resources_.emplace(key, pointer);
    if (!emplace_result.second) {
      throw resource_exists_exception(key.c_str());
    }
  }
  void Add(const KeyType &key, ReferenceType ref) {
    Add(key, new ResourceType(ref));
  }

  bool Has(const KeyType &key) const {
    auto it = resources_.find(key);
    return it != resources_.end();
  }

  ReferenceType GetRef(const KeyType &key) const {
    return *resources_.at(key);
  }

  PointerType GetPtr(const KeyType &key) const {
    return resources_.at(key);
  }

  template<typename Function>
  void ForEach(Function &&f) {
    for (auto[key, value] : resources_) {
      f(key, GetRef(key));
    }
  }

  void Print() {
    std::cout << "Keys: " << std::endl;
    for (auto &element : resources_) {
      std::cout << "\t" << element.first << std::endl;
    }
  }

private:

  std::map<KeyType, PointerType> resources_;
};

namespace Details {

template<typename T>
struct FunctionTraits {
  template<typename R, typename ... As>
  static ::std::tuple<std::decay_t<As>...> pro_args(std::function<R(As...)>);

  using ArgumentsTuple = decltype(pro_args(std::function{std::declval<T>()}));
};

std::vector<std::string> FindTDirectory(const TDirectory &dir, const std::string &cwd = "") {
  std::vector<std::string> result;

  for (auto o : *dir.GetListOfKeys()) {
    auto key_ptr = static_cast<TKey *>(o);
    if (TClass::GetClass(key_ptr->GetClassName())->InheritsFrom(TDirectory::Class())) {
      auto nested_dir = static_cast<TDirectory *>(key_ptr->ReadObj());
      auto nested_contents = FindTDirectory(*nested_dir, cwd + "/" + nested_dir->GetName());
      std::move(std::begin(nested_contents), std::end(nested_contents), std::back_inserter(result));
    } else {
      result.emplace_back(cwd + "/" + key_ptr->GetName());
    }
  }
  return result;
}

template<typename Tuple, std::size_t IArg>
bool SetArgI(const std::vector<std::string> &arg_names, Tuple &tuple) {
  using ArgT = std::tuple_element_t<IArg, Tuple>;
  auto &manager = ResourceManager<ArgT>::Instance();
  if (manager.Has(arg_names[IArg])) {
    std::get<IArg>(tuple) = manager.GetRef(arg_names[IArg]);
    return true;
  }
  return false;
}

template<typename Tuple, std::size_t ... IArg>
bool SetArgTupleImpl(const std::vector<std::string> &arg_names, Tuple &tuple, std::index_sequence<IArg...>) {
  std::vector<bool> result{SetArgI<Tuple, IArg>(arg_names, tuple)...};
  return std::all_of(result.begin(), result.end(), [](bool is_ok) { return is_ok; });
}

template<typename ... Args>
bool SetArgTuple(const std::vector<std::string> &arg_names, ::std::tuple<Args...> &tuple) {
  return SetArgTupleImpl(arg_names, tuple, std::make_index_sequence<sizeof...(Args)>());
}

}

template<typename T>
void AddResource(std::string name, T &&ref) {
  ResourceManager<T>::Instance().Add(name, ref);
}

template<typename T>
void AddResource(std::string name, T *ref) {
  ResourceManager<T>::Instance().Add(name, ref);
}

template<typename T>
void LoadFile(const char *file_name) {
  TFile f(file_name, "READ");

  for (const auto &path : Details::FindTDirectory(f)) {
    auto ptr = f.Get<T>(path.c_str());
    if (ptr) {
      std::cout << "Adding path '" << path << "'" << std::endl;
      AddResource(path, ptr);
    }
  }
}

template<typename T>
void ExportToROOT(const char *filename) {
  TFile f(filename, "RECREATE");
  ResourceManager<T>::Instance().ForEach([&f](const std::string &name, const T &value) {
    using std::filesystem::path;
    path p(name);
    if (!p.is_absolute()) {
      throw std::runtime_error("Path must be absolute");
    }

    auto dname = p.parent_path().relative_path();
    auto bname = p.filename();
    std::cout << dname << " ::: " << bname << std::endl;
    auto save_dir = f.GetDirectory(dname.c_str());
    if (!save_dir) {
      save_dir = f.mkdir(dname.c_str());
    }
    save_dir->WriteTObject(&value, bname.c_str());
  });
}

template<typename Function>
void Define(const std::string &name, Function &&function, std::vector<std::string> arg_names) {
  using ArgsTuple = typename Details::FunctionTraits<Function>::ArgumentsTuple;
  ArgsTuple args;
  if (Details::SetArgTuple(arg_names, args)) {
    AddResource(name, std::apply(function, args));
  } else {
    Warning(__func__, "Some arguments for '%s' are missing", name.c_str());
  }
}

Qn::DataContainerStatCalculate Resolution3S(const Qn::DataContainerStatCalculate &nom1,
                                            const Qn::DataContainerStatCalculate &nom2,
                                            const Qn::DataContainerStatCalculate &denom) {
  auto nom = nom1 * nom2;
  nom = 2 * nom;
  return Qn::Sqrt(nom / denom);
}

int main() {
  using std::string;
  using std::get;

  TFile f("correlation.root", "READ");
  LoadFile<Qn::DataContainerStatCollect>(f.GetName());

  ResourceManager<Qn::DataContainerStatCollect>::Instance().ForEach([](const std::string &name,
                                                                       Qn::DataContainerStatCollect collect) {
    AddResource(name, Qn::DataContainerStatCalculate(collect));
  });

  ResourceManager<Qn::DataContainerStatCalculate>::Instance().Print();

  std::list<std::tuple<string, string, string>> args_list = {
      {"psd1", "psd2", "psd3"},
      {"psd2", "psd1", "psd3"},
      {"psd3", "psd1", "psd2"},
  };

  const string RECENTERED = "_RECENTERED";

  for (auto &component : {"x1x1", "y1y1"})
    for (auto &args : args_list) {
      auto arg1_name = "/QQ/" + get<0>(args) + RECENTERED + "." + get<1>(args) + RECENTERED + "." + component;
      auto arg2_name = "/QQ/" + get<0>(args) + RECENTERED + "." + get<2>(args) + RECENTERED + "." + component;
      auto arg3_name = "/QQ/" + get<1>(args) + RECENTERED + "." + get<2>(args) + RECENTERED + "." + component;
      auto resolution = "/resolution/RES_" + get<0>(args) + "_" + component;
      Define(resolution, Resolution3S, {arg1_name, arg2_name, arg3_name});
    }

  ExportToROOT<Qn::DataContainerStatCalculate>("correlation_proc.root");

  return 0;
}

