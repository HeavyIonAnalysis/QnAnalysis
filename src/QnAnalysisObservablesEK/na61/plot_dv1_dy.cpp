//
// Created by eugene on 07/05/2021.
//

#include "plot_dv1_dy.hpp"
#include "using.hpp"
#include "Observables.hpp"

#include "v1_reco.hpp"

Qn::SystematicError FromSlope(const Qn::Dv1Dy &dv1dy) {
  Qn::SystematicError result;
  result.SetRef(dv1dy.GetSlope(), dv1dy.GetSlopeError(), dv1dy.IsFitValid() ? 1. : 0.0);
  return result;
}
Qn::DataContainerSystematicError MakeSlopeSysContainer(const Qn::DataContainer<Qn::Dv1Dy> &slope_data) {
  Qn::DataContainerSystematicError result(slope_data.GetAxes());
  std::transform(slope_data.begin(), slope_data.end(), result.begin(), FromSlope);
  return result;
}
Qn::SystematicError FromOffset(const Qn::Dv1Dy &dv1dy) {
  Qn::SystematicError result;
  result.SetRef(dv1dy.GetOffset(), dv1dy.GetOffsetError(), dv1dy.IsFitValid() ? 1. : 0.0);
  return result;
}
Qn::DataContainerSystematicError MakeOffsetSysContainer(const Qn::DataContainer<Qn::Dv1Dy> &slope_data) {
  Qn::DataContainerSystematicError result(slope_data.GetAxes());
  std::transform(slope_data.begin(), slope_data.end(), result.begin(), FromOffset);
  return result;
}

void plot_dv1_dy() {
  float y_fit_lo = -0.2;
  float y_fit_hi = 0.8;
  Qn::AxisD axis_pt("pT", {0., 0.2, 0.6, 1.0, 1.8});

  ::Tools::ToRoot<TMultiGraph> root_saver("dv1_dy_slope.root", "RECREATE");
  gResourceManager
      .ForEach([=,&root_saver](const StringKey &key, ResourceManager::Resource &resources) {

                 auto data = resources.As<DTCalc>().Rebin(axis_pt);
                 auto slope_data = Qn::EvalSlopeND(data, "y_cm", y_fit_lo, y_fit_hi);
                 auto v1_slope_systematics = MakeSlopeSysContainer(slope_data);
                 auto v1_offset_systematics = MakeOffsetSysContainer(slope_data);

                 {
                   v1_slope_systematics.AddSystematicSource("component");
                   v1_offset_systematics.AddSystematicSource("component");
                   auto fs = v1_reco_feature_set() - "v1.component";
                   auto fs_reference = fs(resources);
                   gResourceManager.ForEach(
                       [&fs, &fs_reference,
                           y_fit_lo, y_fit_hi,
                           &v1_slope_systematics, &v1_offset_systematics, axis_pt]
                           (const StringKey &key, ResourceManager::Resource &component) {
                         if (fs(component) != fs_reference) return;
                         auto data = component.As<DTCalc>().Rebin(axis_pt);
                         auto slope_data = Qn::EvalSlopeND(data, "y_cm", y_fit_lo, y_fit_hi);
                         v1_slope_systematics.AddSystematicVariation(
                             "component",
                             slope_data,
                             [](std::size_t ibin, const Qn::Dv1Dy &input, double &value, double &error) -> bool {
                               if (!input.IsFitValid()) return false;
                               value = input.GetSlope();
                               error = input.GetSlopeError();
                               return true;
                             });
                         v1_offset_systematics.AddSystematicVariation(
                             "component",
                             slope_data,
                             [](std::size_t ibin, const Qn::Dv1Dy &input, double &value, double &error) -> bool {
                               if (!input.IsFitValid()) return false;
                               value = input.GetOffset();
                               error = input.GetOffsetError();
                               return true;
                             });
                       },
                       META["type"] == "v1" &&
                           (META["v1.component"] == "x1x1" || META["v1.component"] == "y1y1")
                   );
                 }

                 {
                   v1_slope_systematics.AddSystematicSource("psd_reference");
                   v1_offset_systematics.AddSystematicSource("psd_reference");
                   auto fs = v1_reco_feature_set() - "v1.ref";
                   auto fs_reference = fs(resources);
                   gResourceManager.ForEach(
                       [&fs, &fs_reference,
                           y_fit_lo, y_fit_hi,
                           &v1_slope_systematics, &v1_offset_systematics,axis_pt]
                           (const StringKey &key, ResourceManager::Resource &component) {
                         if (fs(component) != fs_reference) return;
                         auto data = component.As<DTCalc>().Rebin(axis_pt);
                         auto slope_data = Qn::EvalSlopeND(data, "y_cm", y_fit_lo, y_fit_hi);
                         v1_slope_systematics.AddSystematicVariation(
                             "psd_reference",
                             slope_data,
                             [](std::size_t ibin, const Qn::Dv1Dy &input, double &value, double &error) -> bool {
                               if (!input.IsFitValid()) return false;
                               value = input.GetSlope();
                               error = input.GetSlopeError();
                               return true;
                             });
                         v1_offset_systematics.AddSystematicVariation(
                             "psd_reference",
                             slope_data,
                             [](std::size_t ibin, const Qn::Dv1Dy &input, double &value, double &error) -> bool {
                               if (!input.IsFitValid()) return false;
                               value = input.GetOffset();
                               error = input.GetOffsetError();
                               return true;
                             });
                       },
                       META["type"] == "v1" && META["v1.ref"].Matches("psd[0-9]")
                   );
                 }

                 /* pT scan, STAT + SYSTEMATIC */
                 {
                   auto graph_list = Qn::ToGSE2D(v1_slope_systematics, "pT",
                                                 0.4, 0.,
                                                 1., 0.1, 0.1);
                   TMultiGraph mg_pt_scan;
                   TMultiGraph mg_pt_scan_data;
                   TMultiGraph mg_pt_scan_errors;
                   int i_slice = 0;
                   for (auto obj : *graph_list) {
                     auto *gse = (GraphSysErr *) obj;
                     if (!gse) {
                       i_slice++;
                       continue;
                     }

                     auto primary_color = ::Tools::GetRainbowPalette().at(i_slice % size(::Tools::GetRainbowPalette()));
                     auto alt_color = ::Tools::GetRainbowPastelPalette().at(i_slice % size(::Tools::GetRainbowPastelPalette()));
                     gse->SetDataOption(GraphSysErr::kNormal);
                     gse->SetLineColor(primary_color);
                     gse->SetMarkerColor(primary_color);
                     gse->SetMarkerStyle(kFullCircle);

                     gse->SetSumOption(GraphSysErr::kRect);
                     gse->SetSumFillStyle(3001);
                     gse->SetSumFillColor(alt_color);

                     auto plot_title = Form("%.2f < p_{T} < %.2f (GeV/#it{c})",
                                            axis_pt.GetLowerBinEdge(i_slice),
                                            axis_pt.GetUpperBinEdge(i_slice));

                     auto multi = gse->GetMulti("COMBINED QUAD");
                     if (multi) {
                       auto errors_graph = (TGraphAsymmErrors *) multi->GetListOfGraphs()->FindObject("error");
                       auto data_graph = (TGraphAsymmErrors *) multi->GetListOfGraphs()->FindObject("data");
                       assert(errors_graph && data_graph);
                       ::Tools::GraphSetErrorsX(errors_graph, 0.01);
                       ::Tools::GraphSetErrorsX(data_graph, 0.0);
                       data_graph->SetTitle(plot_title);
                       mg_pt_scan_errors.Add((TGraph *) errors_graph->Clone(), "20");
                       mg_pt_scan_data.Add((TGraph *) data_graph->Clone(), "lpZ");
                     }
                     i_slice++;
                   }
                   mg_pt_scan.Add((TMultiGraph *) mg_pt_scan_errors.Clone(), "20");
                   mg_pt_scan.Add((TMultiGraph *) mg_pt_scan_data.Clone(), "lpZ");
                   mg_pt_scan.GetXaxis()->SetTitle("Centrality (%)");
                   mg_pt_scan.GetYaxis()->SetTitle("d v_{1} / d #it{y}_{CM}");
                   root_saver.operator()(BASE_OF(KEY)(resources) + "/dv1_dy__pt", mg_pt_scan);
                   root_saver.operator()(BASE_OF(KEY)(resources) + "/dv1_dy__pt_data", mg_pt_scan_data);
                   root_saver.operator()(BASE_OF(KEY)(resources) + "/dv1_dy__pt_errors", mg_pt_scan_data);
                 }

                 /* Centrality scan, STAT + SYSTEMATIC */
                 {
                   auto graph_list = Qn::ToGSE2D(v1_slope_systematics, "Centrality",
                                                 0.005,0.0,
                                                 1.,0.1, 0.1);
                   TMultiGraph mg_y_scan;
                   int i_y_cm_slice = 0;
                   for (auto obj : *graph_list) {
                     auto *gse = (GraphSysErr *) obj;
                     if (!gse) {
                       i_y_cm_slice++;
                       continue;
                     }

                     auto primary_color = ::Tools::GetRainbowPalette().at(i_y_cm_slice % size(::Tools::GetRainbowPalette()));
                     auto alt_color = ::Tools::GetRainbowPastelPalette().at(i_y_cm_slice % size(::Tools::GetRainbowPastelPalette()));
                     gse->SetDataOption(GraphSysErr::kNormal);
                     gse->SetLineColor(primary_color);
                     gse->SetMarkerColor(primary_color);

                     gse->SetSumOption(GraphSysErr::kRect);
                     gse->SetSumFillStyle(1001);
                     gse->SetSumFillColor(alt_color);

                     auto multi = gse->GetMulti("COMBINED QUAD");
                     if (multi) {
                       ::Tools::GraphShiftX(multi, i_y_cm_slice*0.010f);
                       mg_y_scan.Add(multi);
                     }

                     i_y_cm_slice++;
                   }
                   mg_y_scan.GetXaxis()->SetTitle("p_{T} (GeV/#it{c})");
                   mg_y_scan.GetYaxis()->SetTitle("v_{1}");
                   root_saver.operator()(BASE_OF(KEY)(resources) + "/dv1_dy__centrality  ", mg_y_scan);
                 }
               },
               META["type"] == "v1" &&
                   META["v1.axis"] == "2d" &&
                   META["v1.ref"] == "combined" &&
                   META["v1.component"] == "combined");
}
