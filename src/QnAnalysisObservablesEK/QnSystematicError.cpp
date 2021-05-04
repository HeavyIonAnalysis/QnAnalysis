//
// Created by eugene on 04/05/2021.
//

#include "QnSystematicError.hpp"
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
      ::std::back_inserter(result),
      [] (const auto& entry) { return entry.first; });
  return result;
}

int Qn::DataContainerSystematicError::GetSystematicSourceId(const std::string &name) const {
  return systematic_sources_ids.at(name);
}

GraphSysErr *Qn::ToGSE(const Qn::DataContainerSystematicError &data,
                       float error_x_scaling,
                       double x_shift) {
  if (data.GetAxes().size() > 1) {
    std::cout << "Data container has more than one dimension. " << std::endl;
    std::cout
        << "Cannot draw as Graph. Use Projection() to make it one dimensional."
        << std::endl;
    return nullptr;
  }

  const std::vector<short> def_color_palette = {
      kRed - 7, kBlue - 8, kCyan -2, kGreen - 1, kOrange + 6, kViolet -5
  };

  auto n_points =
      std::count_if(data.begin(), data.end(), [] (const SystematicError& bin_err) { return bin_err.SumWeights() > 0; });

  auto graph = new GraphSysErr(n_points);

  std::map<int, int> systematic_id_map;
  for (auto &systematic_source_name : data.GetSystematicSources()) {
    auto data_error_id = data.GetSystematicSourceId(systematic_source_name);
    auto pp_id = graph->DeclarePoint2Point(systematic_source_name.c_str(), false);
    systematic_id_map.emplace(data_error_id, pp_id);
  }

  graph->SetDataOption(GraphSysErr::kNormal);

  graph->SetSumLineColor(kRed+2);
  graph->SetSumLineWidth(2);
  graph->SetSumTitle("All errors");
  graph->SetSumOption(GraphSysErr::kHat);

  int ipalette = 0;
  for (auto &sys_errors : systematic_id_map) {
    auto pp_id = sys_errors.second;
    graph->SetSysOption(pp_id, GraphSysErr::kFill);
    graph->SetSysFillStyle(pp_id, 1001);
    graph->SetSysFillColor(pp_id, def_color_palette.at(ipalette % def_color_palette.size()));
    ipalette++;
  }

  unsigned int ibin = 0;
  unsigned int igraph = 0;
  for (const auto &bin : data) {
    if (bin.SumWeights() <= 0) {
      ibin++;
      continue;
    }
    auto y = bin.Mean();
    auto ey_stat = bin.GetStatisticalErrorOfMean();
    auto xhi = data.GetAxes().front().GetUpperBinEdge(ibin);
    auto xlo = data.GetAxes().front().GetLowerBinEdge(ibin);
    auto xhalfwidth = (xhi - xlo)/2.;
    auto x = xlo + xhalfwidth + x_shift;

    graph->SetPoint(igraph, x, y);
    graph->SetPointError(igraph, error_x_scaling*xhalfwidth);
    graph->SetStatError(igraph, ey_stat);
    for (auto &stat_source_element : systematic_id_map) {
      const auto data_error_id = stat_source_element.first;
      const auto pp_id = stat_source_element.second;
      const auto error = bin.GetSystematicalError(data_error_id);
      graph->SetSysError(pp_id, igraph, 0., error);
    }
    ibin++;
    igraph++;
  }
  return graph;
}

TList *Qn::ToGSE2D(const Qn::DataContainerSystematicError &data,
                   const std::string& projection_axis_name,
                   double x_shift) {
  if (data.GetAxes().size() != 2) {
    std::cout << "N(dim) != 2 " << std::endl;
    return nullptr;
  }

  auto result = new TList;
  result->SetOwner();


  auto projection_axis = data.GetAxis(projection_axis_name);
  for (decltype(projection_axis.GetNBins()) ibin = 0; ibin < projection_axis.GetNBins(); ++ibin) {
    double bin_lo = projection_axis.GetLowerBinEdge(ibin);
    double bin_hi = projection_axis.GetUpperBinEdge(ibin);
    auto axis_to_select = Qn::AxisD(projection_axis_name, 1 ,bin_lo, bin_hi);
    auto data_selected = DataContainerSystematicError(data.Select(axis_to_select));
    data_selected.CopySourcesInfo(data);

    auto graph_selected = Qn::ToGSE(data_selected, 0., ibin*x_shift);
    graph_selected->SetName(Form("%s__%.2f_%.2f", projection_axis_name.c_str(), bin_lo, bin_hi));
    graph_selected->SetTitle(Form("%s #in (%.2f, %.2f)", projection_axis_name.c_str(), bin_lo, bin_hi));
    result->Add(graph_selected);
  }

  return result;
}
