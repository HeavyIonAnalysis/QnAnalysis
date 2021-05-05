//
// Created by eugene on 04/05/2021.
//

#ifndef QNANALYSIS_SRC_QNANALYSISOBSERVABLESEK_QNSYSTEMATICERROR_HPP_
#define QNANALYSIS_SRC_QNANALYSISOBSERVABLESEK_QNSYSTEMATICERROR_HPP_

#include "gse.hpp"

#include <DataContainer.hpp>
#include <StatCalculate.hpp>

#include <TMath.h>

#include <map>
#include <vector>
#include <algorithm>

namespace Qn {

class SystematicError {
 public:
  SystematicError() = default;

  explicit SystematicError(const StatCalculate& data) {
    mean = data.Mean();
    statistical_error = data.StandardErrorOfMean();
    sumw = data.SumWeights();
  }
  explicit SystematicError(const StatCollect& data) {
    mean = data.GetStatistics().Mean();
    statistical_error = data.GetStatistics().StandardErrorOfMean();
    sumw = data.GetStatistics().SumWeights();
  }
  void AddSystematicSource(int id) {
    auto emplace_result = variations_means.emplace(id, std::vector<double>());
    assert(emplace_result.second);
  }
  void AddVariation(int id, const StatCalculate& data) {
    variations_means.at(id).emplace_back(data.Mean());
  }
  double GetSystematicalError(int id) const {
    auto &means = variations_means.at(id);
    auto val_max = *max_element(begin(means), end(means));
    auto val_min = *min_element(begin(means), end(means));
    return 0.5*(val_max - val_min);
  }
  double GetSystematicalError() const {
    double sigma2 = 0.0;
    for (auto &syst_source_element : variations_means) {
      auto syst_source_id = syst_source_element.first;
      auto source_sigma = GetSystematicalError(syst_source_id);
      sigma2 += (source_sigma*source_sigma);
    }
    return TMath::Sqrt(sigma2);
  }
  double SumWeights() const {
    return sumw;
  }
  double GetStatisticalErrorOfMean() const {
    return statistical_error;
  }
  double Mean() const {
    return mean;
  }

 private:
  friend SystematicError operator*(const SystematicError &operand, double scale);
  friend SystematicError operator*(double operand, const SystematicError &rhs);

  double mean;
  double statistical_error;
  double sumw;
  /* key = id of the systematic source, value = vector of value for given systematic source */
  std::map<int,std::vector<double>> variations_means;
};



class DataContainerSystematicError : public DataContainer<SystematicError> {
 public:
  template <typename T>
  explicit DataContainerSystematicError(DataContainer<T>& other) : DataContainer<SystematicError>(other) {}
  template<typename T>
  explicit DataContainerSystematicError(DataContainer<T>&&other) : DataContainer<SystematicError>(other) {}

  void AddSystematicSource(const std::string& name);

  void AddSystematicVariation(const std::string& name, const DataContainerStatCalculate& other);

  std::vector<std::string> GetSystematicSources() const;
  int GetSystematicSourceId(const std::string &name) const;

  void CopySourcesInfo(const DataContainerSystematicError& other) {
    this->systematic_sources_ids = other.systematic_sources_ids;
  }

 private:


  std::map<std::string, int> systematic_sources_ids;

};

SystematicError operator*(const SystematicError &operand, double scale);
SystematicError operator*(double operand, const SystematicError &rhs);


GraphSysErr *ToGSE(
    const Qn::DataContainerSystematicError &data,
    float error_x = .0f,
    double min_sumw = 1.,
    double max_sys_error = 0.0);
TList *ToGSE2D(const DataContainerSystematicError& data,
               const std::string& projection_axis_name,
               float error_x = 1.0,
               double min_sumw = 1.0,
               double max_sys_error = 0.0);


} // namespace Qn

#endif //QNANALYSIS_SRC_QNANALYSISOBSERVABLESEK_QNSYSTEMATICERROR_HPP_