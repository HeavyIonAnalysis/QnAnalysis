//
// Created by eugene on 21/07/2020.
//

#ifndef DATATREEFLOW_SRC_CORRELATION_CORRELATIONTASK_H
#define DATATREEFLOW_SRC_CORRELATION_CORRELATIONTASK_H

#include <algorithm>
#include <utility>
#include <vector>
#include <string>
#include <filesystem>

#include <QnDataFrame.hpp>
#include <TFile.h>
#include <TTree.h>
#include <boost/program_options.hpp>

#include <yaml-cpp/yaml.h>

#include "Config.hpp"
#include "Utils.hpp"
#include "UserCorrelationAction.hpp"

namespace Qn::Analysis::Correlate {

class CorrelationTaskRunner {

  using QVectorList = std::vector<QVectorTagged>;
  using CorrelationResultPtr = ROOT::RDF::RResultPtr<Qn::Correlation::CorrelationActionBase>;

  struct Correlation {
    QVectorList args_list;
    std::string action_name;

    std::vector<std::string> argument_names;
    std::string correlation_name;
    CorrelationResultPtr result_ptr;
  };

  struct CorrelationTaskInitialized {
    size_t arity{0};
    size_t n_axes{0};
    std::list<Correlation> correlations;
  };

  struct bad_config_file : public std::exception {
    bad_config_file() = default;
    explicit bad_config_file(const std::exception &e) : std::exception(e) {};
  };

public:
  static constexpr size_t MAX_ARITY = 8;
  static constexpr size_t MAX_AXES = 4;

  boost::program_options::options_description GetBoostOptions();

  void Initialize();

  void Run();

private:
  std::unique_ptr<ROOT::RDataFrame> GetRDF();
  void LookupConfiguration();
  bool LoadConfiguration(const std::filesystem::path &path);

  static std::vector<Correlation> GetTaskCombinations(const CorrelationTask &t);

  template<typename Iter>
  static std::string JoinStrings(Iter &&i1, Iter &&i2, std::string delim = ", ") {
    auto n = std::distance(i1, i2);
    if (n < 2) {
      return std::string(*i1);
    }

    std::stringstream str_stream;
    for (auto ii = i1; ii < i2 - 1; ++ii) {
      str_stream << *ii << delim;
    }
    str_stream << *(i2 - 1);
    return str_stream.str();
  }

  static Qn::AxisD ToQnAxis(const AxisConfig &c);

  static std::string ToQVectorFullName(const QVectorTagged &qv);
  /*
   * All machinery below is needed because Lucas uses static
   * polymorphism in correlation task base on DataFrame, that
   * makes dynamic parametrization very difficult to implement.
   *
   * Vectors of axes and vectors of arguments must be somehow converted
   * to arrays or passed to template function
   */
  template<std::size_t N, std::size_t... Seq>
  static
  constexpr std::index_sequence<N + Seq ...>
  add(std::index_sequence<Seq...>) { return {}; }

  template<std::size_t Min, std::size_t Max>
  using make_index_range = decltype(add<Min>(std::make_index_sequence<Max - Min>()));

  template<size_t ... IAxis>
  static auto MakeAxisConfig(const std::vector<Qn::AxisD> &axes, std::index_sequence<IAxis...>) {
    return Qn::MakeAxes(axes.at(IAxis)...);
  }

  template<size_t Arity>
  static bool PredicateArity(const CorrelationTask &t) { return t.arguments.size() == Arity; }

  template<size_t NAxes>
  static bool PredicateNAxes(const CorrelationTask &t) { return t.axes.size() == NAxes; }



  /**
   * @brief Takes task config and initializes IO
   *        TODO shrink as possible
   * @tparam Arity
   * @tparam NAxis
   * @param t
   * @return
   */
  template<size_t Arity, size_t NAxis>
  std::shared_ptr<CorrelationTaskInitialized> InitializeTask(const CorrelationTask &t) {
    using Qn::Correlation::UseWeights;
    /* this is a function pointer */
    const auto GetActionRegistry = ::Qn::Analysis::Correlate::Action::GetActionRegistry<Arity>;

    /* Qn::MakeAxes() */
    std::vector<Qn::AxisD> axes_qn;
    std::transform(t.axes.begin(), t.axes.end(),
                   std::back_inserter(axes_qn), ToQnAxis);
    auto axes_config = MakeAxisConfig(axes_qn, std::make_index_sequence<NAxis>());

    /* weights */
    auto use_weights = t.weight_type == EQnWeight(EQnWeight::OBSERVABLE) ? UseWeights::Yes : UseWeights::No;
    auto weight_function_name = t.weights_function;
    auto weight_function = GetActionRegistry().Get(weight_function_name);

    auto result = std::make_shared<CorrelationTaskInitialized>();
    /* init RDataFrame */
    auto df = GetRDF()->Range(0, 100);
    auto df_sampled = Qn::Correlation::Resample(df, t.n_samples);

    for (auto &correlation : GetTaskCombinations(t)) {
      auto action_name = correlation.action_name;
      auto correlation_function = GetActionRegistry().Get(action_name);

      auto args_list = correlation.args_list;
      std::array<std::string, Arity> args_list_array;
      std::copy(std::begin(correlation.argument_names), std::end(correlation.argument_names),
                std::begin(args_list_array));


      auto booked_action = Qn::MakeAverageHelper(Qn::Correlation::MakeCorrelationAction(
          correlation.correlation_name,
          correlation_function,
          weight_function,
          use_weights,
          args_list_array,
          axes_config,
          t.n_samples)).BookMe(df_sampled);

      correlation.result_ptr = booked_action;

      result->correlations.emplace_back(correlation);
      Info(__func__, "%s", correlation.correlation_name.c_str());
    }

    result->arity = Arity;
    result->n_axes = NAxis;

    return result;
  }

  template<typename OIter, size_t Arity, size_t NAxes>
  void InitializeTasksArityNAxes(OIter &&o, const std::vector<CorrelationTask> &tasks) {
    std::vector<CorrelationTask> tasks_naxes;
    std::copy_if(tasks.begin(), tasks.end(), std::back_inserter(tasks_naxes), PredicateNAxes<NAxes>);
    for (auto &t : tasks_naxes) {
      o = InitializeTask<Arity, NAxes>(t);
    }
  }

  template<typename OIter, typename Container, size_t Arity, size_t ...INAxes>
  void InitializeTasksArity(OIter &&o, Container &&c) {
    std::vector<typename std::remove_reference_t<Container>::value_type> tasks_arity;
    std::copy_if(std::begin(c), std::end(c), std::back_inserter(tasks_arity), PredicateArity<Arity>);
    (InitializeTasksArityNAxes<OIter, Arity, INAxes>(std::forward<OIter>(o), tasks_arity),...);
  }

  template<typename OIter, typename Container, size_t...IArity, size_t...INAxis>
  void InitializeTasksImpl(OIter &&o, Container &&c, std::index_sequence<IArity...>, std::index_sequence<INAxis...>) {
    /* CXX17 folding */
    (InitializeTasksArity<OIter, Container, IArity, INAxis...>(std::forward<OIter>(o), std::forward<Container>(c)),...);
  }

  auto InitializeTasks() {
    initialized_tasks_.clear();
    InitializeTasksImpl(std::back_inserter(initialized_tasks_),
                               config_tasks_,
                               make_index_range<1, MAX_ARITY>(), make_index_range<1, MAX_AXES>());
  }

  std::filesystem::path configuration_file_path_{};
  std::string configuration_node_name_{};
  std::string output_file_;
  std::filesystem::path input_file_;
  std::string input_tree_;

  std::vector<CorrelationTask> config_tasks_;
  std::vector<std::shared_ptr<CorrelationTaskInitialized>> initialized_tasks_;
};

}

#endif  // DATATREEFLOW_SRC_CORRELATION_CORRELATIONTASK_H
