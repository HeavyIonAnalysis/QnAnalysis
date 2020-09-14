#ifndef FLOW_SRC_ANALYZE_RESOURSEMANAGER_H_
#define FLOW_SRC_ANALYZE_RESOURSEMANAGER_H_

#include <vector>
#include <string>
#include <map>
#include <memory>
#include <functional>

#include "TFile.h"

#include "Utils.h"

class ResourceManager {

 public:

  void LoadFile(const std::string &fileName);

  /**
   * TODO
   * @param query
   * @param fct
   * @return
   */
  bool Define(const std::string &query,
              std::function<Result(std::vector<Result>, const std::vector<std::string> &)> fct) {
    const std::regex re(R"(\w+:\w+(,\w+)*)");
    return false;
  }

  /**
   * @brief Define new object from result of Function evaluation. If a function requires parameters, for example,
   * mixed-harmonics resolution method requires rapidity interval, user must specify them after function.
   * @tparam Function
   * @tparam Parameter
   * @param what
   * @param argnames
   * @param fct
   * @param function_params
   * @return
   */
  template<typename Function, typename ... Parameter>
  bool Define(const std::string &what,
              const std::vector<std::string> &argnames,
              Function && fct, Parameter && ... function_params) {
    using namespace std::placeholders;
    auto fct_wrapper = std::bind(fct, _1, _2, std::forward<Parameter>(function_params)...);
    return Define(what, argnames, fct_wrapper);
  }


  template<typename Function>
  bool Define(const std::string &what,
              const std::vector<std::string> &argnames,
              Function && fct) {

    // vector of copies
    std::vector<Result> args{};

    if (argnames.empty()) {
      return false;
    }

    for (const auto &argname : argnames) {
      auto resIt = resourceMap.find(argname);
      if (resIt == resourceMap.end()) {
        Warning(__func__, "Resource '%s' was not found", argname.c_str());
        return false;
      }

      args.emplace_back(*(*resIt).second);
    }

    auto result = std::make_shared<Result>(fct(std::move(args), argnames));
    Info(__func__, "Adding resource '%s'", what.c_str());
    resourceMap.insert({what, result});
    return true;
  }

  void SaveAs(const std::string &filename);
  void ForMatchingExec(const std::string &pattern, const std::function<void(const std::string &, Result)> &fct) const;
  std::vector<std::string> GetMatchingName(const std::string &pattern);

 private:

  std::map<std::string, std::shared_ptr<Result >> resourceMap{};
};


#endif //FLOW_SRC_ANALYZE_RESOURSEMANAGER_H_
