//
// Created by eugene on 07/05/2021.
//

#include "plot_v1_2d.hpp"
#include "using.hpp"
#include "Observables.hpp"
#include "v1_centrality.hpp"


void plot_v1_2d() {
/* v1 (y,Pt) Multigraphs */
  ::Tools::ToRoot<TMultiGraph> root_saver("v1_multigraph.root", "RECREATE");
  gResourceManager
      .ForEach(
          [&root_saver](const StringKey &key, ResourceManager::Resource &resource) {
            auto syst_data = Qn::DataContainerSystematicError(resource.As<DTCalc>());
            {
              syst_data.AddSystematicSource("component");
              auto fs = v1_centrality_reco_feature_set() - "v1.component";
              auto fs_reference = fs(resource);
              gResourceManager.ForEach(
                  [&fs, &fs_reference,& syst_data](const StringKey &key,
                                                   ResourceManager::Resource &component) {
                    if (fs(component) != fs_reference) return;
                    syst_data.AddSystematicVariation("component", component.As<DTCalc>());
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
                  [&fs, &fs_reference, &syst_data] (const StringKey& key, ResourceManager::Resource& psd_ref) {
                    if (fs(psd_ref) != fs_reference) return;
                    syst_data.AddSystematicVariation("psd_reference", psd_ref.As<DTCalc>());
                  }, META["type"] == "v1_centrality" && META["v1.ref"].Matches("psd[0-9]"));
            }


            /* pT scan, STAT + SYSTEMATIC */
            {
              auto graph_list = Qn::ToGSE2D(syst_data, "pT", 0.005, 1e2, 0.2);
              TMultiGraph mg_pt_scan;
              int i_slice = 0;
              for (auto obj : *graph_list) {
                auto *gse = (GraphSysErr *) obj;

                auto primary_color = ::Tools::GetRainbowPalette().at(i_slice % size(::Tools::GetRainbowPalette()));
                auto alt_color = ::Tools::GetRainbowPastelPalette().at(i_slice % size(::Tools::GetRainbowPastelPalette()));
                gse->SetDataOption(GraphSysErr::kNormal);
                gse->SetLineColor(primary_color);
                gse->SetMarkerColor(primary_color);
                gse->SetMarkerStyle(kFullCircle);

                gse->SetSumOption(GraphSysErr::kRect);
                gse->SetSumFillStyle(1001);
                gse->SetSumFillColor(alt_color);

                auto multi = gse->GetMulti("COMBINED QUAD");
                if (multi) {
                  ::Tools::GraphShiftX(multi, i_slice*0.010f);
                  mg_pt_scan.Add(multi);
                }
                i_slice++;
              }
              mg_pt_scan.GetXaxis()->SetTitle("#it{y}_{CM}");
              mg_pt_scan.GetYaxis()->SetTitle("v_{1}");
              root_saver.operator()(BASE_OF(KEY)(resource) + "/v1_y", mg_pt_scan);
            }

            /* pT scan, different contributions */
            {
              auto graph_list = Qn::ToGSE2D(syst_data, "pT", 0.1, 1e3);

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
              auto graph_list = Qn::ToGSE2D(syst_data, "y_cm", 0.005,
                                            1e2, 0.2);
              TMultiGraph mg_y_scan;
              int i_y_cm_slice = 0;
              for (auto obj : *graph_list) {
                auto *gse = (GraphSysErr *) obj;

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
              root_saver.operator()(BASE_OF(KEY)(resource) + "/v1_pt  ", mg_y_scan);
            }
            /* Y scan, different contributions */
            {
              auto graph_list = Qn::ToGSE2D(syst_data, "y_cm", 0.1, 1e3);

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
