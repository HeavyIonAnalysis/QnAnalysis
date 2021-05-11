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
  explicit SystematicError(const StatCalculate& data);
  explicit SystematicError(const StatCollect& data);
  void SetRef(const StatCalculate& data);
  void SetRef(const StatCollect& data);
  void SetRef(double value, double error, double sumw);
  void AddSystematicSource(int id);
  void AddVariation(int id, double value, double error);
  void AddVariation(int id, const StatCalculate& data);
  double GetSystematicalError(int id) const;
  double GetSystematicalError() const;
  double SumWeights() const {
    return sumw;
  }
  double GetStatisticalErrorOfMean() const {
    return statistical_error;
  }
  double Mean() const {
    return mean;
  }

  static
  double BarlowCriterion(double ref, double sigma_ref, double variation, double sigma_variation);


 private:
  friend SystematicError operator*(const SystematicError &operand, double scale);
  friend SystematicError operator*(double operand, const SystematicError &rhs);

  double mean{0.};
  double statistical_error{-1.};
  double sumw{-1.};
  /* key = id of the systematic source, value = vector of value for given systematic source */
  std::map<int,std::vector<double>> variations_means;
  std::map<int,std::vector<double>> variations_errors;
};



class DataContainerSystematicError : public DataContainer<SystematicError> {
 public:
  DataContainerSystematicError();

  DataContainerSystematicError(const std::vector<AxisD> &axes);
  template <typename T>
  explicit DataContainerSystematicError(DataContainer<T>& other) : DataContainer<SystematicError>(other) {}
  template<typename T>
  explicit DataContainerSystematicError(DataContainer<T>&&other) : DataContainer<SystematicError>(other) {}

  void AddSystematicSource(const std::string& name);

  template<typename Function, typename T>
  void AddSystematicVariation(const std::string& name, const DataContainer<T>& other, Function && f) {
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
      double mean;
      double error;
      if (f(ibin, other[ibin], mean, error)) {
        (*this)[ibin].AddVariation(source_id, mean, error);
      }
    }
  }

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


GraphSysErr *ToGSE(const Qn::DataContainerSystematicError &data,
                   float err_x_data,
                   float err_x_sys,
                   double min_sumw,
                   double max_abs_sys_error,
                   double max_abs_stat_error);
TList *ToGSE2D(const DataContainerSystematicError& data,
               const std::string& selection_axis_name,
               float err_x_data = 0.0,
               float err_x_sys = 0.0,
               double min_sumw = 1.0,
               double max_sys_error = 0.0,
               double max_stat_error = 0.0);


} // namespace Qn

#endif //QNANALYSIS_SRC_QNANALYSISOBSERVABLESEK_QNSYSTEMATICERROR_HPP_
