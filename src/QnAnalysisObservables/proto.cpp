//
// Created by eugene on 24/11/2020.
//

#include <DataContainer.hpp>
#include <DataContainerHelper.hpp>

#include <filesystem>
#include <utility>
#include <map>
#include <any>
#include <regex>
#include <tuple>

#include <TFile.h>
#include <TKey.h>
#include <TDirectory.h>
#include <TClass.h>

#include <QnAnalysisTools/QnAnalysisTools.hpp>

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

namespace Predicates {

auto AlwaysTrue = [](const std::string &) -> bool { return true; };

struct RegexMatch {
  explicit RegexMatch(const std::string &expr_str) : expr(expr_str) {}
  explicit RegexMatch(std::regex re_expr) : expr(std::move(re_expr)) {}

  bool operator () (const std::string &str) const {
    return std::regex_match(str, expr);
  }

  const std::regex expr;
};

} /// namespace Predicates

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
  void Add(const KeyRepr &key, T &&obj) {
    static_assert(std::is_copy_constructible_v<T>, "T must be copy-constructible to be stored in ResourceManager");
    auto emplace_result = resources_.template emplace(
        Details::Convert<KeyRepr>::ToString(key),
        std::make_shared<std::any>(obj));
    if (!emplace_result.second) {
      throw ResourceAlreadyExists(Details::Convert<KeyRepr>::ToString(key));
    }
  }

  template<typename KeyRepr, typename T>
  void Add(const KeyRepr &key, T *ptr) {
    Add(key, *ptr);
  }

  template<typename KeyRepr>
  bool Has(const KeyRepr &key) const {
    auto it = resources_.find(Details::Convert<KeyRepr>::ToString(key));
    return it != resources_.end();
  }

  template<typename KeyRepr, typename T>
  T &Get(const KeyRepr &key) {
    if (!Has(key)) {
      throw NoSuchResource(key);
    }
    return std::any_cast<std::add_lvalue_reference_t<T>>(
        (*resources_[Details::Convert<KeyRepr>::ToString(key)]));
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
          using ArgType = typename Traits::template ArgType<1>;
          fct(Details::Convert<KeyRepr>::FromString(element.first) /* pass by value to prevent from unintentional change */,
              std::any_cast<ArgType>(*element.second));
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

  std::map<KeyType, std::shared_ptr<std::any>> resources_;
};

namespace Tools {

namespace Details {

template<typename Tuple, std::size_t IArg>
void SetArgI(const std::vector<std::string> &arg_names, Tuple &tuple) {
  using ArgT = std::tuple_element_t<IArg, Tuple>;
  auto &manager = ResourceManager::Instance();
  try {
    std::get<IArg>(tuple) = manager.Get<std::string, ArgT>(arg_names[IArg]);
  } catch (std::bad_any_cast &e) {
    throw ResourceManager::NoSuchResource(arg_names[IArg]);
  }
}
template<typename Tuple, std::size_t ... IArg>
void SetArgTupleImpl(const std::vector<std::string> &arg_names, Tuple &tuple, std::index_sequence<IArg...>) {
  (SetArgI<Tuple, IArg>(arg_names, tuple), ...);
}
template<typename ... Args>
void SetArgTuple(const std::vector<std::string> &arg_names, std::tuple<Args...> &tuple) {
  SetArgTupleImpl(arg_names, tuple, std::make_index_sequence<sizeof...(Args)>());
}
}

template<typename KeyRepr, typename Function>
void Define(const KeyRepr &key, Function &&fct, std::vector<std::string> arg_names, bool warn_at_missing = true) {
  using ArgsTuple = typename ::Details::FunctionTraits<decltype(std::function{fct})>::ArgumentsTuple;
  ArgsTuple args;
  try {
    Details::SetArgTuple(arg_names, args);
    AddResource(key, std::apply(fct, args));
  } catch (ResourceManager::NoSuchResource &e) {
    if (warn_at_missing) {
      Warning(__func__, "Resource '%s' required for '%s' is missing, new resource won't be added",
              e.what(), key.c_str());
    } else {
      throw e; /* rethrow */
    }
  }
}

template<typename KeyRepr, typename Function>
void Define1(const KeyRepr &key, Function &&fct, std::vector<std::string> arg_names, bool warn_at_missing = true) {
  using Traits = ::Details::FunctionTraits<decltype(std::function{fct})>;
  using Container = std::decay_t<typename Traits::template ArgType<0>>;
  using Entity = typename Container::value_type;
  Container args_container;
  /* lookup arguments */
  auto args_back_inserter = std::back_inserter(args_container);
  for (auto &arg : arg_names) {
    try {
      *args_back_inserter = ResourceManager::Instance().template Get<std::string, Entity>(arg);
    } catch (ResourceManager::NoSuchResource &e) {
      if (warn_at_missing) {
        Warning(__func__, "Resource '%s' required for '%s' is missing, new resource won't be added",
                e.what(), key.c_str());
        return;
      } else {
        throw e;
      }
    }
  }
  AddResource(key, fct(args_container));
}

template<typename T>
void ExportToROOT(const char *filename, const char *mode = "RECREATE") {
  TFile f(filename, mode);
  ResourceManager::Instance().ForEach([&f](const std::string &name, T &value) {
    using ::std::filesystem::path;
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
  }, Predicates::AlwaysTrue, false);
}
} /// namespace Tools

namespace Methods {

Qn::DataContainerStatCalculate Resolution3S(const Qn::DataContainerStatCalculate &nom1,
                                            const Qn::DataContainerStatCalculate &nom2,
                                            const Qn::DataContainerStatCalculate &denom) {
  auto nom = nom1 * nom2;
  nom = 2 * nom;
  return Qn::Sqrt(nom / denom);
}

} /// namespace Methods

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

}

template<typename KeyRepr, typename T>
void AddResource(const KeyRepr &key, T &&ref) {
  ResourceManager::Instance().Add(key, std::forward<T>(ref));
}

template<typename T>
void AddResource(const std::string &name, T *ref) {
  ResourceManager::Instance().Add(name, ref);
}

template<typename T>
void LoadROOTFile(const std::string &file_name, const std::string &manager_prefix = "") {
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

int main() {
  using std::string;
  using std::get;
  namespace Tools = Qn::Analysis::Tools;
  using ::Tools::Define;
  using ::Tools::Define1;

  TFile f("correlation.root", "READ");
  LoadROOTFile<Qn::DataContainerStatCollect>(f.GetName(), "raw");

  /* Convert everything to Qn::DataContainerStatCalculate */
  ResourceManager::Instance().ForEach([](std::vector<std::string> key, Qn::DataContainerStatCollect &collect) {
    key[0] = "calc";
    AddResource(key, Qn::DataContainerStatCalculate(collect));
  });
  /* To check compatibility with old stuff, multiply raw correlations by factor of 2 */
  ResourceManager::Instance().ForEach([](std::vector<std::string> key, Qn::DataContainerStatCalculate calc) {
    key[0] = "x2";
    auto x2 = 2 * calc;
    x2.SetErrors(Qn::StatCalculate::ErrorType::PROPAGATION);
    AddResource(key, x2);
  });

  ResourceManager::Instance().Print();

  std::vector<std::tuple<string, string, string>> args_list = {
      {"psd1", "psd2", "psd3"},
      {"psd2", "psd1", "psd3"},
      {"psd3", "psd1", "psd2"},
  };
  std::vector<std::string> components = {"x1x1", "y1y1"};

  const string RECENTERED = "_RECENTERED";

  for (auto && [component, ref_args] : Tools::Combination(components, args_list)) {
    auto arg1_name =
        "/calc/QQ/" + get<0>(ref_args) + RECENTERED + "." + get<1>(ref_args) + RECENTERED + "." + component;
    auto arg2_name =
        "/calc/QQ/" + get<0>(ref_args) + RECENTERED + "." + get<2>(ref_args) + RECENTERED + "." + component;
    auto arg3_name =
        "/calc/QQ/" + get<1>(ref_args) + RECENTERED + "." + get<2>(ref_args) + RECENTERED + "." + component;
    auto resolution = "/resolution/3sub/RES_" + get<0>(ref_args) + "_" + component;
    ::Tools::Define(resolution, Methods::Resolution3S, {arg1_name, arg2_name, arg3_name});
  }

  /* export everything to TGraph */
  ResourceManager::Instance().ForEach([](const std::string &name, Qn::DataContainerStatCalculate &calc) {
    auto graph = Qn::ToTGraph(calc);
    AddResource("/profiles" + name, graph);
  }, [](const std::string &key) { return std::regex_match(key, std::regex("^(/x2/QQ/|/resolution).*$")); });
  /* export PSD correlations to TGraph for comparison */
  ResourceManager::Instance().ForEach([](const std::string &name, Qn::DataContainerStatCalculate &calc) {
    const std::regex re(".*(psd[1-3])_RECENTERED.(psd[1-3])_RECENTERED.([a-z])1([a-z])1");
    std::smatch match_results;
    auto graph = Qn::ToTGraph(calc);
    auto asymmgraph = new TGraphAsymmErrors(graph->GetN(),
                                            graph->GetX(),
                                            graph->GetY(),
                                            graph->GetEX(),
                                            graph->GetEX(),
                                            graph->GetEY(),
                                            graph->GetEY());
    if(std::regex_search(name, match_results, re)) {
      auto Q1 = match_results.str(1);
      auto Q2 = match_results.str(2);
      auto I1 = match_results.str(3);
      auto I2 = match_results.str(4);
      asymmgraph->SetTitle(("Q^{" + Q1 + "}_{1," + I1 + "} " +  "Q^{" + Q2 + "}_{1," + I2 + "} new soft").c_str());
      asymmgraph->GetXaxis()->SetTitle("PSD Centrality (%)");
      AddResource("/raw/" + Q1 + "_" + Q2 + "_" +
        string{(char)std::toupper(I1[0])} +
        string{(char)std::toupper(I2[0])}, asymmgraph);
    }
  }, ::Predicates::RegexMatch("^/x2.*psd[1-3]_RECENTERED.psd[1-3]_RECENTERED.*"));
  ::Tools::ExportToROOT<Qn::DataContainerStatCalculate>("correlation_proc.root");
  ::Tools::ExportToROOT<TGraphErrors>("prof.root");
  ::Tools::ExportToROOT<TGraphAsymmErrors>("prof.root", "UPDATE");

  return 0;
}

