//
// Created by eugene on 07/05/2021.
//

#include "plot_v1_pt_y.hpp"
#include "using.hpp"
#include "Observables.hpp"
#include "v1_centrality.hpp"

inline
TMultiGraph *
ToTMultiGraph(const Qn::DataContainerStatCalculate &data, const std::string &projection_axis_name = "") {
  if (data.GetAxes().size() != 2) {
    std::cout << "N(dim) != 2 " << std::endl;
    return nullptr;
  }

  auto result = new TMultiGraph;
  auto projection_axis = data.GetAxis(projection_axis_name);
  for (decltype(projection_axis.GetNBins()) ibin = 0; ibin < projection_axis.GetNBins(); ++ibin) {
    auto axis_to_select = Qn::AxisD(projection_axis_name, 1,
                                    projection_axis.GetLowerBinEdge(ibin),
                                    projection_axis.GetUpperBinEdge(ibin));
    auto data_selected = data.Select(axis_to_select);
    auto graph_selected = Qn::ToTGraph(data_selected);

    graph_selected->SetTitle(Form("%s #in (%.2f, %.2f)",
                                  projection_axis_name.c_str(),
                                  projection_axis.GetLowerBinEdge(ibin),
                                  projection_axis.GetUpperBinEdge(ibin)));
    result->Add(graph_selected, "lp");
  }

  return result;
}

Qn::DataContainerSystematicError CollectV1Systematics(
    ResourceManager::Resource &resource,
    Qn::AxisD rebin_y_cm = {},
    Qn::AxisD rebin_pt = {}) {

  auto PreprocessData = [rebin_y_cm, rebin_pt] (const DTCalc& data) {
    auto result = data;
    if (!rebin_y_cm.Name().empty()) {
      result = result.Rebin(rebin_y_cm);
    }
    if (!rebin_pt.Name().empty()) {
      result = result.Rebin(rebin_pt);
    }
    return result;
  };
  auto reference_data = resource.As<DTCalc>();

  auto syst_data = Qn::DataContainerSystematicError(PreprocessData(resource.As<DTCalc>()));
  {
    syst_data.AddSystematicSource("component");
    auto fs = v1_centrality_reco_feature_set() - "v1.component";
    auto fs_reference = fs(resource);
    gResourceManager.ForEach(
        [&fs, &fs_reference, &syst_data, &PreprocessData](const StringKey &key,
                                                               ResourceManager::Resource &component) {
          if (fs(component) != fs_reference) return;
          syst_data.AddSystematicVariation("component", PreprocessData(component.As<DTCalc>()));
        },
        META["type"] == "v1_centrality" &&
            (META["v1.component"] == "x1x1" || META["v1.component"] == "y1y1")
    );
  }
  {
    syst_data.AddSystematicSource("psd_reference");
    auto fs = v1_centrality_reco_feature_set() - "v1.ref";
    auto fs_reference = fs(resource);
    gResourceManager.ForEach(
        [&fs, &fs_reference, &syst_data, &PreprocessData](const StringKey &key, ResourceManager::Resource &psd_ref) {
          if (fs(psd_ref) != fs_reference) return;
          syst_data.AddSystematicVariation("psd_reference", PreprocessData(psd_ref.As<DTCalc>()));
        }, META["type"] == "v1_centrality" && META["v1.ref"].Matches("psd[0-9]"));
  }
  return  syst_data;
}



void plot_v1_pt_y() {
/* v1 (y,Pt) Multigraphs */
  ::Tools::ToRoot<TMultiGraph> root_saver("v1_multigraph.root", "RECREATE");
  Qn::AxisD axis_pt("pT", {0., 0.2, 0.6, 1.0, 1.8});
  Qn::AxisD axis_pt_pions("pT", {0., 0.2, 0.6, 1.0, 1.8});
//  Qn::AxisD axis_y_cm("y_cm", {-0.4, 0., 0.4, 0.6, 0.8, 1.0, 1.2, 1.4, 1.6, 1.8});
  Qn::AxisD axis_y_cm("y_cm", {-0.4, 0.4, 0.8, 1.0, 1.8});
  Qn::AxisD axis_y_cm_pions("y_cm", {0., 0.8, 1.6});

  gResourceManager
      .ForEach(
          [=, &root_saver](const StringKey &key, ResourceManager::Resource &resource) {
            /* pT scan, STAT + SYSTEMATIC */
            {
              auto pt_rebin_axis = (META["v1.particle"] == "pion_neg" || META["v1.particle"] == "pion_pos")(resource)?
                                     axis_pt_pions : axis_pt;
              auto syst_data = CollectV1Systematics(resource, {}, pt_rebin_axis);
              auto graph_list = Qn::ToGSE2D(syst_data, "pT", 0.01, 0.01,
                                            1e2, 0.05, 0.05);
              TMultiGraph mg_pt;
              TMultiGraph mg_pt_scan_errors;
              TMultiGraph mg_pt_scan_data;
              int i_slice = 0;
              for (auto obj : *graph_list) {
                auto *gse = (GraphSysErr *) obj;
                if (!gse) {
                  i_slice++;
                  continue;
                }

                auto primary_color = ::Tools::GetRainbowPalette().at(i_slice % size(::Tools::GetRainbowPalette()));
                auto alt_color =
                    ::Tools::GetRainbowPastelPalette().at(i_slice % size(::Tools::GetRainbowPastelPalette()));
                gse->SetDataOption(GraphSysErr::kNormal);
                gse->SetLineColor(primary_color);
                gse->SetMarkerColor(primary_color);
                gse->SetMarkerStyle(kFullCircle);

                gse->SetSumOption(GraphSysErr::kRect);
                gse->SetSumFillStyle(3001);
                gse->SetSumFillColor(alt_color);

                auto plot_title = Form("%.2f < p_{T} < %.2f (GeV/#it{c})",
                                       pt_rebin_axis.GetLowerBinEdge(i_slice),
                                       pt_rebin_axis.GetUpperBinEdge(i_slice));

                auto multi = gse->GetMulti("COMBINED QUAD");
                if (multi) {
                  ::Tools::GraphShiftX(multi, 0.00f);
                  /** collect errors and data independently **/
                  auto errors_graph = (TGraphAsymmErrors *) multi->GetListOfGraphs()->FindObject("error");
                  ::Tools::GraphSetErrorsX(errors_graph, 0.01);
                  auto data_graph = (TGraphAsymmErrors *) multi->GetListOfGraphs()->FindObject("data");
                  ::Tools::GraphSetErrorsX(data_graph, 0.0);
                  data_graph->SetTitle(plot_title);

                  assert(errors_graph && data_graph);
                  mg_pt_scan_errors.Add((TGraph *) errors_graph->Clone(), "20");
                  mg_pt_scan_data.Add((TGraph *) data_graph->Clone(), "lpZ");
                }
                delete multi;
                i_slice++;
              }
              mg_pt.Add((TMultiGraph *) mg_pt_scan_errors.Clone(), "20");
              mg_pt.Add((TMultiGraph *) mg_pt_scan_data.Clone(), "lpZ");
              mg_pt.GetXaxis()->SetTitle("#it{y}_{CM}");
              mg_pt.GetYaxis()->SetTitle("v_{1}");
              root_saver.operator()(BASE_OF(KEY)(resource) + "/v1_y", mg_pt);
              root_saver.operator()(BASE_OF(KEY)(resource) + "/v1_y_data", mg_pt_scan_data);
              root_saver.operator()(BASE_OF(KEY)(resource) + "/v1_y_sys_errors", mg_pt_scan_errors);
            }

            /* pT scan, different contributions */
            {
              auto pt_rebin_axis = (META["v1.particle"] == "pion_neg" || META["v1.particle"] == "pion_pos")(resource)?
                                   axis_pt_pions : axis_pt;
              auto syst_data = CollectV1Systematics(resource, {}, pt_rebin_axis);
              auto graph_list = Qn::ToGSE2D(syst_data, "pT", 0.1, 0.1, 1);

              for (auto obj : *graph_list) {
                auto *gse = (GraphSysErr *) obj;
                auto multi = gse->GetMulti("STACK");
                if (multi) {
                  multi->GetXaxis()->SetTitle("#it{y}_{CM}");
                  multi->GetYaxis()->SetTitle("v_{1}");
                  root_saver.operator()(BASE_OF(KEY)(resource) + "/" + gse->GetName(), *multi);
                }
              }
            }

            /* Y scan, STAT + SYSTEMATIC */
            {
              auto y_cm_rebin_axis = (META["v1.particle"] == "pion_neg" || META["v1.particle"] == "pion_pos")(resource)?
                  axis_y_cm_pions : axis_y_cm;
              auto syst_data = CollectV1Systematics(resource, y_cm_rebin_axis, {});
              auto graph_list = Qn::ToGSE2D(syst_data, "y_cm", 0.01, 0.01,
                                            1e2, 0.05, 0.05);
              TMultiGraph mg_y_scan;
              TMultiGraph mg_y_scan_data;
              TMultiGraph mg_y_scan_errors;

              int i_y_cm_slice = 0;
              for (auto obj : *graph_list) {
                auto *gse = (GraphSysErr *) obj;
                if (!gse) {
                  i_y_cm_slice++;
                  continue;
                }

                auto primary_color = ::Tools::GetRainbowPalette().at(i_y_cm_slice % size(::Tools::GetRainbowPalette()));
                auto alt_color =
                    ::Tools::GetRainbowPastelPalette().at(i_y_cm_slice % size(::Tools::GetRainbowPastelPalette()));
                gse->SetDataOption(GraphSysErr::kConnect);
                gse->SetLineColor(primary_color);
                gse->SetMarkerColor(primary_color);

                gse->SetSumOption(GraphSysErr::kRect);
                gse->SetSumFillStyle(3001);
                gse->SetSumFillColor(alt_color);

                auto plot_title = Form("%.2f < #it{y}_{CM} < %.2f",
                                       y_cm_rebin_axis.GetLowerBinEdge(i_y_cm_slice),
                                       y_cm_rebin_axis.GetUpperBinEdge(i_y_cm_slice));

                auto multi = gse->GetMulti("COMBINED QUAD");
                if (multi) {
                  ::Tools::GraphShiftX(multi, 0.0f);
                  auto errors_graph = (TGraphAsymmErrors *) multi->GetListOfGraphs()->FindObject("error");
                  ::Tools::GraphSetErrorsX(errors_graph, 0.01);
                  auto data_graph = (TGraphAsymmErrors *) multi->GetListOfGraphs()->FindObject("data");
                  ::Tools::GraphSetErrorsX(data_graph, 0.0);
                  data_graph->SetTitle(plot_title);

                  assert(errors_graph && data_graph);
                  mg_y_scan_errors.Add((TGraph *) errors_graph->Clone(), "20");
                  mg_y_scan_data.Add((TGraph *) data_graph->Clone(), "lpZ");
                }

                i_y_cm_slice++;
              }
              mg_y_scan.Add((TMultiGraph *) mg_y_scan_errors.Clone(), "20");
              mg_y_scan.Add((TMultiGraph *) mg_y_scan_data.Clone(), "lpZ");
              mg_y_scan.GetXaxis()->SetTitle("p_{T} (GeV/#it{c})");
              mg_y_scan.GetYaxis()->SetTitle("v_{1}");
              root_saver.operator()(BASE_OF(KEY)(resource) + "/v1_pt  ", mg_y_scan);
              root_saver.operator()(BASE_OF(KEY)(resource) + "/v1_pt_data", mg_y_scan_data);
              root_saver.operator()(BASE_OF(KEY)(resource) + "/v1_pt_sys_errors", mg_y_scan_errors);
            }
            /* Y scan, different contributions */
            {
              auto y_cm_rebin_axis = (META["v1.particle"] == "pion_neg" || META["v1.particle"] == "pion_pos")(resource)?
                                     axis_y_cm_pions : axis_y_cm;
              auto syst_data = CollectV1Systematics(resource, y_cm_rebin_axis, {});
              auto graph_list = Qn::ToGSE2D(syst_data, "y_cm", 0.1, 0.1, 1);

              for (auto obj : *graph_list) {
                auto *gse = (GraphSysErr *) obj;
                auto multi = gse->GetMulti("STACK");
                if (multi) {
                  multi->GetXaxis()->SetTitle("p_{T} (GeV/#it{c})");
                  multi->GetYaxis()->SetTitle("v_{1}");
                  root_saver.operator()(BASE_OF(KEY)(resource) + "/" + gse->GetName(), *multi);
                }
              }
            }
          },
          META["type"] == "v1_centrality" &&
              META["v1.axis"] == "2d" &&
              META["v1.component"] == "combined" &&
              META["v1.ref"] == "combined");
}
