//
// Created by eugene on 24/11/2020.
//

#include <DataContainer.hpp>

#include <filesystem>
#include <utility>
#include <map>
#include <any>
#include <tuple>

#include <TFile.h>
#include <TKey.h>
#include <TDirectory.h>
#include <TClass.h>

#include <QnAnalysisTools/QnAnalysisTools.hpp>

#include <DataContainer.hpp>
#include <StatCollect.hpp>

namespace Details {

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

class ResourceManager : public Details::Singleton<ResourceManager> {
public:

  typedef std::string KeyType;

  template<typename T>
  void Add(const KeyType &key, T &&obj) {
    auto emplace_result = resources_.template emplace(key, obj);
    if (!emplace_result.second) {
      throw Details::ResourceAlreadyExists(key);
    }
  }

  template<typename T>
  void Add(const KeyType &key, T *ptr) {
    Add(key, *ptr);
  }

  bool Has(const KeyType &key) const {
    auto it = resources_.find(key);
    return it != resources_.end();
  }

  template<typename T>
  T &GetRef(const KeyType &key) {
    if (!Has(key)) {
      throw Details::NoSuchResource(key);
    }
    return std::any_cast<std::add_lvalue_reference_t<T>>(resources_[key]);
  }

  template<typename Function>
  void ForEach(Function &&fct, bool warn_bad_cast = true) {
    using Traits = Details::FunctionTraits<decltype(std::function{fct})>;
    for (auto &element : resources_) {
      try {
        static_assert(Traits::N_ARGS == 2);
        using ArgType = typename Traits::ArgType<1>;
        fct(element.first, std::any_cast<ArgType>(element.second));
      } catch (std::bad_any_cast &e) {
        if (warn_bad_cast)
          Warning(__func__, "Bad cast for '%s'. Skipping...", element.first.c_str());
      }
    }
  }

  void Print() {
    std::cout << "Keys: " << std::endl;
    for (auto &element : resources_) {
      std::cout << "\t" << element.first << std::endl;
    }
  }

private:

  std::map<KeyType, std::any> resources_;
};

namespace Details {

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
void SetArgI(const std::vector<std::string> &arg_names, Tuple &tuple) {
  using ArgT = std::tuple_element_t<IArg, Tuple>;
  auto &manager = ResourceManager::Instance();
  try {
    std::get<IArg>(tuple) = manager.GetRef<ArgT>(arg_names[IArg]);
  } catch (std::bad_any_cast &e) {
    throw Details::NoSuchResource(arg_names[IArg]);
  }
}

template<typename Tuple, std::size_t ... IArg>
void SetArgTupleImpl(const std::vector<std::string> &arg_names, Tuple &tuple, std::index_sequence<IArg...>) {
  (SetArgI<Tuple, IArg>(arg_names, tuple), ...);
}

template<typename ... Args>
void SetArgTuple(const std::vector<std::string> &arg_names, ::std::tuple<Args...> &tuple) {
  SetArgTupleImpl(arg_names, tuple, std::make_index_sequence<sizeof...(Args)>());
}

}

template<typename T>
void AddResource(std::string name, T &&ref) {
  ResourceManager::Instance().Add(name, std::forward<T>(ref));
}

template<typename T>
void AddResource(const std::string &name, T *ref) {
  ResourceManager::Instance().Add(name, ref);
}

template<typename T>
void LoadFile(const std::string &file_name, const std::string &manager_prefix = "") {
  TFile f(file_name.c_str(), "READ");

  for (const auto &path : Details::FindTDirectory(f)) {
    auto ptr = f.Get<T>(path.c_str());
    if (ptr) {
      auto manager_path = manager_prefix.empty() ? path : "/" + manager_prefix + path;
      std::cout << "Adding path '" << manager_path << "'" << std::endl;
      AddResource(manager_path, ptr);
    }
  }
}

template<typename T>
void ExportToROOT(const char *filename, const char *mode = "RECREATE") {
  TFile f(filename, mode);
  ResourceManager::Instance().ForEach([&f](const std::string &name, T &value) {
    using std::filesystem::path;
    path p(name);

    auto dname = p.parent_path().relative_path();
    auto bname = p.filename();
    auto save_dir = f.GetDirectory(dname.c_str(), true);
    if (!save_dir) {
      std::cout << "mkdir " << f.GetName() << ":" << dname.c_str() << std::endl;
      f.mkdir(dname.c_str(), "");
      save_dir = f.GetDirectory(dname.c_str(), true);
    }
    save_dir->WriteTObject(&value, bname.c_str());
  }, false);
}

template<typename Function>
void Define(const std::string &name, Function &&fct, std::vector<std::string> arg_names, bool warn_at_missing = true) {
  using ArgsTuple = typename Details::FunctionTraits<decltype(std::function{fct})>::ArgumentsTuple;
  ArgsTuple args;
  try {
    Details::SetArgTuple(arg_names, args);
    AddResource(name, std::apply(fct, args));
  } catch (Details::NoSuchResource &e) {
    if (warn_at_missing) {
      Warning(__func__, "Resource '%s' required for '%s' is missing, new resource won't be added",
              e.what(), name.c_str());
    } else {
      throw e; /* rethrow */
    }
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
  namespace Tools = Qn::Analysis::Tools;

  TFile f("correlation.root", "READ");
  LoadFile<Qn::DataContainerStatCollect>(f.GetName(), "raw");

  ResourceManager::Instance().ForEach([](const std::string &name, Qn::DataContainerStatCollect collect) {
    AddResource("/calc" + name, Qn::DataContainerStatCalculate(collect));
  });

  ResourceManager::Instance().Print();

  std::list<std::tuple<string, string, string>> args_list = {
      {"psd1", "psd2", "psd3"},
      {"psd2", "psd1", "psd3"},
      {"psd3", "psd1", "psd2"},
  };
  std::list<std::string> components = {"x1x1", "y1y1"};

  const string RECENTERED = "_RECENTERED";

  for (auto component : {"x1x1", "y1y1"})
    for (auto &ref_args : args_list) {
      auto arg1_name =
          "/calc/raw/QQ/" + get<0>(ref_args) + RECENTERED + "." + get<1>(ref_args) + RECENTERED + "." + component;
      auto arg2_name =
          "/calc/raw/QQ/" + get<0>(ref_args) + RECENTERED + "." + get<2>(ref_args) + RECENTERED + "." + component;
      auto arg3_name =
          "/calc/raw/QQ/" + get<1>(ref_args) + RECENTERED + "." + get<2>(ref_args) + RECENTERED + "." + component;
      auto resolution = "/resolution/3sub/RES_" + get<0>(ref_args) + "_" + component;
      Define(resolution, Resolution3S, {arg1_name, arg2_name, arg3_name});
    }

  ExportToROOT<Qn::DataContainerStatCalculate>("correlation_proc.root");

  return 0;
}

