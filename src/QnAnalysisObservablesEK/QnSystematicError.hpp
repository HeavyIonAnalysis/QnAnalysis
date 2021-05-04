//
// Created by eugene on 04/05/2021.
//

#ifndef QNANALYSIS_SRC_QNANALYSISOBSERVABLESEK_QNSYSTEMATICERROR_HPP_
#define QNANALYSIS_SRC_QNANALYSISOBSERVABLESEK_QNSYSTEMATICERROR_HPP_

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
    value = data.Mean();
    statistical_error = data.StandardErrorOfMean();
  }
  explicit SystematicError(const StatCollect& data) {
    value = data.GetStatistics().Mean();
    statistical_error = data.GetStatistics().StandardErrorOfMean();
  }
  void AddSystematicSource(int id) {
    auto emplace_result = variations_means.emplace(id, std::vector<double>());
    assert(emplace_result.second);
  }
  void AddVariation(int id, const StatCalculate& data) {
    variations_means.at(id).emplace_back(data.Mean());
  }
  double GetStatisticalError() const {
    return statistical_error;
  }
  double GetSystematicalError(int id) const {
    auto &means = variations_means.at(id);
    return *max(begin(means), end(means)) - *min(begin(means), end(means));
  }
  double GetSystematicalError() const {
    auto sigma2 = 0.0;
    for (auto &syst_source_element : variations_means) {
      auto syst_source_id = syst_source_element.first;
      auto source_sigma = GetSystematicalError(syst_source_id);
      sigma2 += (source_sigma*source_sigma);
    }
    return TMath::Sqrt(sigma2);
  }

 private:
  double value;
  double statistical_error;
  /* key = id of the systematic source, value = vector of value for given systematic source */
  std::map<int,std::vector<double>> variations_means;
};

class DataContainerSystematicError : public DataContainer<SystematicError> {
 public:
  template <typename T>
  explicit DataContainerSystematicError(DataContainer<T>& other) : DataContainer<SystematicError>(other) {}

  void AddSystematicSource(const std::string& name) {
    int source_id = systematic_sources_ids.size();
    for (auto &bin : *this) {
      bin.AddSystematicSource(source_id);
    }
    systematic_sources_ids.emplace(name, source_id);
  }

  void AddSystematicVariation(const std::string& name, const DataContainerStatCalculate& other) {
    auto source_id_it = systematic_sources_ids.find(name);
    if (source_id_it == systematic_sources_ids.end()) {
      AddSystematicSource(name);
      source_id_it = systematic_sources_ids.find(name);
    }
    auto source_id = source_id_it->second;

    /* consistency check between this and other */
    assert(this->size() == other.size());
    // TODO other bin consistency checks

    using size_type = decltype(this->size());
    for (size_type ibin = 0; ibin < this->size(); ++ibin) {
      this->operator[](ibin).AddVariation(source_id, other.operator[](ibin));
    }
  }

 private:
  std::map<std::string, int> systematic_sources_ids;

};




} // namespace Qn

#endif //QNANALYSIS_SRC_QNANALYSISOBSERVABLESEK_QNSYSTEMATICERROR_HPP_
