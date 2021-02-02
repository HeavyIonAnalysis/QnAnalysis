//
// Created by eugene on 04/01/2021.
//

#ifndef QNANALYSIS_SRC_QNANALYSISOBSERVABLES_TOOLS_HPP_
#define QNANALYSIS_SRC_QNANALYSISOBSERVABLES_TOOLS_HPP_

#include <DataContainer.hpp>
#include <DataContainerHelper.hpp>
#include <filesystem>
#include <utility>
#include <TClass.h>
#include <TFile.h>
#include <TKey.h>

namespace Tools {

template<typename T>
struct ToRoot {
  explicit ToRoot(std::string filename, std::string mode = "RECREATE")
      : filename(std::move(filename)), mode(std::move(mode)) {}
  ToRoot(const ToRoot<T> &other) {
    filename = other.filename;
    mode = other.mode;
  }
  ToRoot(ToRoot<T> &&other) noexcept = default;
  ~ToRoot() {
    std::cout << "Exported to '" << filename << "'" << std::endl;
  }

  void operator()(const std::string &fpathstr, T &obj) {
    if (!file) {
      file.reset(TFile::Open(filename.c_str(), mode.c_str()));
    }

    using std::filesystem::path;
    path p(fpathstr);

    auto dname = p.parent_path().relative_path();
    auto bname = p.filename();
    auto save_dir = file->GetDirectory(dname.c_str(), true);

    if (!save_dir) {
      std::cout << "mkdir " << file->GetName() << ":" << dname.c_str() << std::endl;
      file->mkdir(dname.c_str(), "");
      save_dir = file->GetDirectory(dname.c_str(), true);
    }
    save_dir->WriteTObject(&obj, bname.c_str());
  }

  std::string filename;
  std::string mode{"RECREATE"};
  std::unique_ptr<TFile> file{};
};


namespace Details {

template<typename Tuple, std::size_t IArg>
void SetArgI(const std::vector<std::string> &arg_names, Tuple &tuple) {
  using ArgT = std::tuple_element_t<IArg, Tuple>;
  auto &manager = ResourceManager::Instance();
  try {
    std::get<IArg>(tuple) = manager.Get(arg_names[IArg], ResourceManager::ResTag<ArgT>());
  } catch (std::bad_any_cast &e) {
    throw ResourceManager::NoSuchResource(arg_names[IArg]);
  }
}
template<typename Tuple, std::size_t... IArg>
void SetArgTupleImpl(const std::vector<std::string> &arg_names, Tuple &tuple, std::index_sequence<IArg...>) {
  (SetArgI<Tuple, IArg>(arg_names, tuple), ...);
}
template<typename... Args>
void SetArgTuple(const std::vector<std::string> &arg_names, std::tuple<Args...> &tuple) {
  SetArgTupleImpl(arg_names, tuple, std::make_index_sequence<sizeof...(Args)>());
}

ResourceManager::MetaType MergeMeta(const ResourceManager::MetaType &lhs, const ResourceManager::MetaType &rhs) {
  using Meta = ResourceManager::MetaType;

  Meta result = lhs;
  for (auto &element : rhs) {
    auto element_name = element.first;

    if (element.second.empty()) { /// element is value
      result.put_child(element_name, element.second);
    } else { /// element is node
      auto lhs_child = lhs.get_child(element_name, Meta());
      result.put_child(element_name, MergeMeta(lhs_child, element.second));
    }

  }
  return result;
}

template<typename T>
ResourceManager::Resource MakeResource(T&& obj, const ResourceManager::MetaType& meta) {
  return ResourceManager::Resource(std::forward<T>(obj), meta);
}

template<>
ResourceManager::Resource MakeResource<ResourceManager::Resource>(ResourceManager::Resource&& r,
                                                                  const ResourceManager::MetaType& meta) {
  return {r.obj, MergeMeta(r.meta, meta)};
}

template <typename KeyGenerator>
auto EvalKey(const KeyGenerator & key_generator,
             const ResourceManager::Resource& res) {
  return key_generator(res);
}

template<>
auto EvalKey<StringKey>(const StringKey &key,
                        const ResourceManager::Resource&) {
  return key;
}

template<>
auto EvalKey<VectorKey>(const VectorKey &key,
                        const ResourceManager::Resource&) {
  return key;
}

}// namespace Details



enum EDefineMissingPolicy {
  kSilent,
  kWarn,
  kRethrow
};

template<typename KeyGenerator, typename Function>
auto Define(KeyGenerator &&key_generator,
            Function &&fct,
            std::vector<std::string> arg_names,
            const ResourceManager::MetaType& meta_to_override = ResourceManager::MetaType(),
            EDefineMissingPolicy policy = EDefineMissingPolicy::kSilent) {
  using ArgsTuple = typename ::Details::FunctionTraits<decltype(std::function{fct})>::ArgumentsTuple;
  ArgsTuple args;
  try {
    Details::SetArgTuple(arg_names, args);
    auto result = Details::MakeResource(std::apply(fct, args), meta_to_override);
    auto key = Details::EvalKey(std::forward<KeyGenerator>(key_generator), result);
    return AddResource(std::move(key), std::move(result));
  } catch (ResourceManager::NoSuchResource &e) {
    if (policy == EDefineMissingPolicy::kWarn) {
//      Warning(__func__, "Resource '%s' is required for '%s' is missing, new resource won't be added",
//              e.what(), ::Details::Convert<KeyGenerator>::ToString(key_generator).c_str());
//    FIXME
      return ResourceManager::ResourcePtr();
    } else if (policy == EDefineMissingPolicy::kRethrow) {
      /* this is the right way of rethrowing exceptions
       * see: https://stackoverflow.com/questions/37227300/why-doesnt-c-use-stdnested-exception-to-allow-throwing-from-destructor/37227893#37227893
       */
      std::rethrow_exception(std::current_exception());
    } else {
      return ResourceManager::ResourcePtr();
    }
  }
}

template<typename KeyRepr, typename Function>
void Define1(const KeyRepr &key,
             Function &&fct,
             std::vector<std::string> arg_names,
             ResourceManager::MetaType meta_to_override = ResourceManager::MetaType(),
             bool warn_at_missing = true) {
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

}// namespace Tools

#endif //QNANALYSIS_SRC_QNANALYSISOBSERVABLES_TOOLS_HPP_
