//
// Created by eugene on 04/05/2021.
//

#include "QnSystematicError.hpp"

Qn::SystematicError::SystematicError(const Qn::StatCalculate &data) {
  mean = data.Mean();
  statistical_error = data.StandardErrorOfMean();
  sumw = data.SumWeights();
}

Qn::SystematicError::SystematicError(const Qn::StatCollect &data) {
  mean = data.GetStatistics().Mean();
  statistical_error = data.GetStatistics().StandardErrorOfMean();
  sumw = data.GetStatistics().SumWeights();
}

void Qn::SystematicError::AddSystematicSource(int id) {
  auto emplace_result = variations_means.emplace(id, std::vector<double>());
  variations_errors.emplace(id, std::vector<double>());
  assert(emplace_result.second);
}

void Qn::SystematicError::AddVariation(int id, const Qn::StatCalculate &data) {
  variations_means.at(id).emplace_back(data.Mean());
  variations_errors.at(id).emplace_back(data.StandardErrorOfMean());
}

double Qn::SystematicError::GetSystematicalError(int id) const {
  /* using uncorrected standard deviation */
  auto &var_v = variations_means.at(id);
  auto &var_sigma_v = variations_errors.at(id);
  assert(var_v.size() > 1);
  assert(var_v.size() == var_sigma_v.size());

  double sum2 = 0;
  int n_passed_barlow = 0;
  for(auto var_it = begin(var_v), var_sigma_it = begin(var_sigma_v);
      var_it != end(var_v); ++var_it, ++var_sigma_it) {
    if (BarlowCriterion(mean, statistical_error, *var_it, *var_sigma_it) > 1) {
      ++n_passed_barlow;
      sum2 += (*var_it - mean)*(*var_it - mean);
    }
  }
  auto sigma = n_passed_barlow > 0? TMath::Sqrt(sum2 / n_passed_barlow) : 0.0;
  return sigma;
}

double Qn::SystematicError::GetSystematicalError() const {
  double sigma2 = 0.0;
  for (auto &syst_source_element : variations_means) {
    auto syst_source_id = syst_source_element.first;
    auto source_sigma = GetSystematicalError(syst_source_id);
    sigma2 += (source_sigma*source_sigma);
  }
  return TMath::Sqrt(sigma2);
}

double Qn::SystematicError::BarlowCriterion(double ref, double sigma_ref, double variation, double sigma_variation) {
  auto sigma2_ref = sigma_ref*sigma_ref;
  auto sigma2_var = sigma_variation*sigma_variation;
  auto barlow = TMath::Abs(ref - variation) / TMath::Sqrt(TMath::Abs(sigma2_ref - sigma2_var));
  return barlow;
}
void Qn::DataContainerSystematicError::AddSystematicVariation(const std::string &name,
                                                              const Qn::DataContainerStatCalculate &other) {
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
void Qn::DataContainerSystematicError::AddSystematicSource(const std::string &name) {
  int source_id = systematic_sources_ids.size();
  for (auto &bin : *this) {
    bin.AddSystematicSource(source_id);
  }
  systematic_sources_ids.emplace(name, source_id);
}
std::vector<std::string> Qn::DataContainerSystematicError::GetSystematicSources() const {
  std::vector<std::string> result;
  std::transform(
      ::std::begin(systematic_sources_ids),
      ::std::end(systematic_sources_ids),
      back_inserter(result),
      [](const auto &entry) { return entry.first; });
  return result;
}
int Qn::DataContainerSystematicError::GetSystematicSourceId(const std::string &name) const {
  return systematic_sources_ids.at(name);
}
GraphSysErr *Qn::ToGSE(
    const Qn::DataContainerSystematicError &data,
    float error_x,
    double min_sumw,
    double max_sys_error) {
  if (data.GetAxes().size() > 1) {
    std::cout << "Data container has more than one dimension. " << std::endl;
    std::cout
        << "Cannot draw as Graph. Use Projection() to make it one dimensional."
        << std::endl;
    return nullptr;
  }

  const std::vector<short> def_color_palette = {
      kRed - 7, kBlue - 8, kCyan - 2, kGreen - 1, kOrange + 6, kViolet - 5
  };

  auto TestPoint = [min_sumw, max_sys_error](const SystematicError &bin_err) {
    return bin_err.SumWeights() >= min_sumw &&
        (max_sys_error <= 0 || bin_err.GetSystematicalError() < max_sys_error);
  };

  auto n_points =
      std::count_if(data.begin(), data.end(), TestPoint);

  auto graph = new GraphSysErr(n_points);

  std::map<int, int> systematic_id_map;
  for (auto &systematic_source_name : data.GetSystematicSources()) {
    auto data_error_id = data.GetSystematicSourceId(systematic_source_name);
    auto pp_id = graph->DeclarePoint2Point(systematic_source_name.c_str(), false);
    systematic_id_map.emplace(data_error_id, pp_id);
  }

  graph->SetDataOption(GraphSysErr::kNormal);

  graph->SetSumLineColor(kRed + 2);
  graph->SetSumLineWidth(2);
  graph->SetSumTitle("All errors");
  graph->SetSumOption(GraphSysErr::kHat);

  int ipalette = 0;
  for (auto &sys_errors : systematic_id_map) {
    auto pp_id = sys_errors.second;
    graph->SetSysOption(pp_id, GraphSysErr::kRect);
    graph->SetSysFillStyle(pp_id, 1001);
    graph->SetSysFillColor(pp_id, def_color_palette.at(ipalette % def_color_palette.size()));
    ipalette++;
  }

  unsigned int ibin = 0;
  unsigned int igraph = 0;
  for (const auto &bin : data) {
    if (!TestPoint(bin)) {
      ibin++;
      continue;
    }

    auto y = bin.Mean();
    auto ey_stat = bin.GetStatisticalErrorOfMean();
    auto xhi = data.GetAxes().front().GetUpperBinEdge(ibin);
    auto xlo = data.GetAxes().front().GetLowerBinEdge(ibin);
    auto xhalfwidth = (xhi - xlo) / 2.;
    auto x = xlo + xhalfwidth;

    graph->SetPoint(igraph, x, y);
    graph->SetPointError(igraph, error_x < 0 ? xhalfwidth : error_x);
    graph->SetStatError(igraph, ey_stat);
    for (auto &stat_source_element : systematic_id_map) {
      const auto data_error_id = stat_source_element.first;
      const auto pp_id = stat_source_element.second;
      const auto error = bin.GetSystematicalError(data_error_id);
      graph->SetSysError(pp_id, igraph, error_x, error);
    }
    ibin++;
    igraph++;
  }
  return graph;
}
TList *Qn::ToGSE2D(const Qn::DataContainerSystematicError &data,
                   const std::string &selection_axis_name,
                   float error_x,
                   double min_sumw,
                   double max_sys_error) {
  if (data.GetAxes().size() != 2) {
    std::cout << "N(dim) != 2 " << std::endl;
    return nullptr;
  }

  auto result = new TList;
  result->SetOwner();

  auto selection_axis = data.GetAxis(selection_axis_name);
  for (decltype(selection_axis.GetNBins()) ibin = 0; ibin < selection_axis.GetNBins(); ++ibin) {
    double bin_lo = selection_axis.GetLowerBinEdge(ibin);
    double bin_hi = selection_axis.GetUpperBinEdge(ibin);
    auto axis_to_select = Qn::AxisD(selection_axis_name, 1, bin_lo, bin_hi);
    auto data_selected = DataContainerSystematicError(data.Select(axis_to_select));
    data_selected.CopySourcesInfo(data);

    auto graph_selected = Qn::ToGSE(data_selected, error_x, min_sumw, max_sys_error);
    if (graph_selected) {
      graph_selected->SetName(Form("%s__%.2f_%.2f", selection_axis_name.c_str(), bin_lo, bin_hi));
      graph_selected->SetTitle(Form("%s #in (%.2f, %.2f)", selection_axis_name.c_str(), bin_lo, bin_hi));
      result->Add(graph_selected);
    }
  }

  return result;
}
Qn::SystematicError Qn::operator*(const SystematicError &operand, double scale) {
  SystematicError result;
  result.mean = operand.mean * scale;
  result.statistical_error = operand.statistical_error * TMath::Abs(scale);
  result.sumw = operand.sumw;
  result.variations_means = operand.variations_means;
  for (auto &sys_error : result.variations_means) {
    for (auto &value : sys_error.second) {
      value *= scale;
    }
  }
  result.variations_errors = operand.variations_errors;
  for (auto &sys_error : result.variations_errors) {
    for (auto &value : sys_error.second) {
      value *= TMath::Abs(scale);
    }
  }
  return result;
}
Qn::SystematicError Qn::operator*(double operand, const Qn::SystematicError &rhs) {
  return rhs * operand;
}
