//
// Created by eugene on 06/05/2021.
//

#include "QnDv1Dy.hpp"

#include <TGraph.h>
#include <TGraphErrors.h>
#include <TFitResult.h>
#include <DataContainerHelper.hpp>

#include <algorithm>

using namespace Qn;

Dv1Dy Qn::EvalSlope1D(DataContainer<Qn::StatCalculate> &data, double fit_lo, double fit_hi) {
  assert(data.GetAxes().size() == 1);
  // TODO proper runtime error

  auto tmp_graph = std::unique_ptr<TGraph>(Qn::ToTGraph(data));

  Dv1Dy result;
  auto fit_result = tmp_graph->Fit("pol1", "SQFN", "", fit_lo, fit_lo);
  if (fit_result.Get() && fit_result->IsValid()) {
    auto offset = fit_result->Value(0);
    auto offset_erro = fit_result->Value(0);
    auto slope = fit_result->Value(1);
    auto slope_erro = fit_result->Error(1);
    result.fit_valid = true;
    result.offset = offset;
    result.offset_error = offset_erro;
    result.slope = slope;
    result.slope_error = slope_erro;
  }
  return result;
}

DataContainerDv1Dy Qn::EvalSlopeND(DataContainer<StatCalculate> &data, const std::string& fit_axis_name, double fit_lo, double fit_hi) {
  const auto &data_axes = data.GetAxes();
  const auto &fit_axis = data.GetAxis(fit_axis_name);
  const auto fit_axis_pos = std::distance(
      data_axes.begin(),std::find_if(begin(data_axes), end(data_axes), [&fit_axis_name] (const AxisD& ax) {return ax.Name() == fit_axis_name;}));

  auto result_axes = [&data, &fit_axis_name] () {
    std::vector<Qn::AxisD> result;
    std::copy_if(begin(data.GetAxes()), end(data.GetAxes()), back_inserter(result), [&fit_axis_name] (const AxisD &ax) {
      return ax.Name() != fit_axis_name;});
    return result;
  }();
  assert(!result_axes.empty());

  DataContainer<Dv1Dy> result;
  for (auto &ax : result_axes) {
    result.AddAxis(ax);
  }

  DataContainer<StatCalculate> row_1d;
  row_1d.AddAxis(fit_axis);

  std::size_t i_result_bin = 0;
  for (auto &result_bin : result) {
    auto result_index = result.GetIndex(i_result_bin);

    std::size_t i_fit_axis_bin = 0;
    for (auto &row_bin : row_1d) {
      auto data_index = result_index;
      data_index.insert(begin(data_index) + fit_axis_pos, i_fit_axis_bin);
      row_bin = data.At(data_index);
      ++i_fit_axis_bin;
    }

    result_bin = EvalSlope1D(row_1d, fit_lo, fit_hi);
    ++i_result_bin;
  }

  return result;
}

TGraph *Qn::ToTGraphSlope(const DataContainerDv1Dy &data) {
  assert(data.GetAxes().size() == 1);

  const auto& axis = data.GetAxes().front();

  auto result = new TGraphErrors;
  result->SetMarkerStyle(kFullCircle);

  std::size_t ibin = 0;
  int igraph = 0;
  for (const auto & bin : data) {
    if (!bin.IsFitValid()) {
      ibin++;
      continue;
    }

    auto y = bin.GetSlope();
    auto yerr = bin.GetSlopeError();
    auto xlo = axis.GetLowerBinEdge(ibin);
    auto xhi = axis.GetUpperBinEdge(ibin);
    auto xmid = 0.5*(xlo + xhi);
    auto xhalfbin = 0.5*(xhi - xlo);

    result->SetPoint(igraph, xmid, y);
    result->SetPointError(igraph, 0.0, yerr);
    igraph++;
    ibin++;
  }


  return result;
}

TMultiGraph *Qn::ToTMultiGraphSlope(const DataContainerDv1Dy &data,
                                    const std::string& selection_axis_name) {
  assert(data.GetAxes().size() == 2);

  auto result = new TMultiGraph;
  auto selection_axis = data.GetAxis(selection_axis_name);
  for (decltype(selection_axis.GetNBins()) ibin = 0; ibin < selection_axis.GetNBins(); ++ibin) {
    double bin_lo = selection_axis.GetLowerBinEdge(ibin);
    double bin_hi = selection_axis.GetUpperBinEdge(ibin);
    auto axis_to_select = Qn::AxisD(selection_axis_name, 1, bin_lo, bin_hi);
    auto data_selected = data.Select(axis_to_select);
    auto graph_selected = ToTGraphSlope(data_selected);
    if (graph_selected) {
      graph_selected->SetName(Form("%s__%.2f_%.2f", selection_axis_name.c_str(), bin_lo, bin_hi));
      graph_selected->SetTitle(Form("%s #in (%.2f, %.2f)", selection_axis_name.c_str(), bin_lo, bin_hi));
      result->Add(graph_selected, "lp");
    }
  }
  return result;
}
