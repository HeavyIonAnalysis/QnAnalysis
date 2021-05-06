//
// Created by eugene on 24/11/2020.
//

#include "remap_axis.hpp"
#include "resolution_3sub.hpp"
#include "resolution_mc.hpp"
#include "resolution_4sub.hpp"

#include "v1_reco.hpp"
#include "v1_mc.hpp"
#include "v1_centrality.hpp"
#include "v1_combine.hpp"

#include "using.hpp"

#include <DataContainer.hpp>
#include <DataContainerHelper.hpp>
#include <StatCalculate.hpp>

#include <any>
#include <filesystem>
#include <tuple>
#include <utility>

#include <TClass.h>
#include <TDirectory.h>
#include <TFile.h>
#include <TKey.h>
#include <TSystem.h>
#include <TGraph2DErrors.h>

#include <QnAnalysisTools/QnAnalysisTools.hpp>

#include "Observables.hpp"

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <TCanvas.h>
#include <TLegend.h>
#include <TFitResult.h>

namespace Qn {

inline
TGraph2DErrors *
ToTGraph2D(const Qn::DataContainerStatCalculate &data, double min_sumw = 1e3) {
  if (data.GetAxes().size() != 2) {
    std::cout << "Data container different from 2 dimensions. " << std::endl;
    return nullptr;
  }

  auto graph_2d = new TGraph2DErrors;
  int ibin = 0;
  for (const auto &bin : data) {
    if (bin.SumWeights() < min_sumw) {
      ibin++;
      continue;
    }
    auto z = bin.Mean();
    auto ez = bin.StandardErrorOfMean();
    auto get_bin_center = [&data, &ibin](int axis) {
      auto multi_index = data.GetIndex(ibin);
      auto xhi = data.GetAxes()[axis].GetUpperBinEdge(multi_index[axis]);
      auto xlo = data.GetAxes()[axis].GetLowerBinEdge(multi_index[axis]);
      return 0.5 * (xlo + xhi);
    };
    graph_2d->SetPoint(graph_2d->GetN(), get_bin_center(0), get_bin_center(1), z);
    graph_2d->SetPointError(graph_2d->GetN() - 1, 0., 0., ez);
    ibin++;
  }
  return graph_2d;

}

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

} // namespace Qn

namespace Details {

std::vector<std::string> FindTDirectory(const TDirectory &dir, const std::string &cwd = "") {
  std::vector<std::string> result;

  for (auto o : *dir.GetListOfKeys()) {
    auto key_ptr = dynamic_cast<TKey *>(o);
    if (TClass::GetClass(key_ptr->GetClassName())->InheritsFrom(TDirectory::Class())) {
      auto nested_dir = dynamic_cast<TDirectory *>(key_ptr->ReadObj());
      auto nested_contents = FindTDirectory(*nested_dir, cwd + "/" + nested_dir->GetName());
      std::move(std::begin(nested_contents), std::end(nested_contents), std::back_inserter(result));
    } else {
      result.emplace_back(cwd + "/" + key_ptr->GetName());
    }
  }
  return result;
}

}// namespace Details

template<typename T>
void LoadROOTFile(const std::string &file_name, const std::string &manager_prefix = "") {
  TFile f(file_name.c_str(), "READ");

  for (const auto &path : Details::FindTDirectory(f)) {
    auto ptr = f.Get<T>(path.c_str());
    if (ptr) {
      auto manager_path = manager_prefix.empty() ? path : "/" + manager_prefix + path;
      std::cout << "Adding path '" << manager_path << "'" << std::endl;
      AddResource(manager_path, ptr);
    }
  }
}

inline
std::string
PlotTitle(const ResourceManager::Resource &r) {
  using Predicates::Resource::META;
  using ::Tools::Format;

  const std::map<std::string, std::string> map_particle{
      {"proton", "p"},
      {"protons", "p"},
      {"pion_neg", "#pi-"},
  };
  const std::map<std::string, std::string> map_component{
      {"x1x1", "X"},
      {"y1y1", "Y"},
      {"combined", "X+Y"},
  };

  auto src = META["v1.src"](r);
  auto particle = META["v1.particle"](r);
  auto component = META["v1.component"](r);
  if (src == "mc") {
    return (Format("v_{1,%2%}^{%1%} (#Psi_{RP})")
        % map_particle.at(particle)
        % map_component.at(component)).str();
  } else if (src == "reco") {
    auto reference = META["v1.ref"](r);
    auto part_1 = (Format("v_{1,%2%}^{%1%} (%3%)")
        % map_particle.at(particle)
        % map_component.at(component)
        % reference).str();
    std::string part_2 = META["v1.resolution.title"](r);
    auto part_3 = "setup:" + META["v1.set"](r);
    return part_1.append(" ").append(part_2).append(" ").append(part_3);
  }
  return "";
}

void SaveCanvas(TCanvas &c, const std::string &base_name) {
//  c.Print((base_name + ".png").c_str(), "png");
//  c.Print((base_name + ".pdf").c_str(), "pdf");
//  c.Print((base_name + ".C").c_str());
}

TGraph *CalcSignificance(const TGraphErrors *case_graph, const TGraphErrors *ref_graph) {
  assert(case_graph->GetN() == ref_graph->GetN());
  auto signi_graph = new TGraph(ref_graph->GetN());
  for (int i = 0; i < ref_graph->GetN(); ++i) {
    auto ref_value = ref_graph->GetPointY(i);
    auto ref_err = ref_graph->GetErrorY(i);
    auto case_value = case_graph->GetPointY(i);
    auto case_err = case_graph->GetErrorY(i);
    auto significance = (case_value - ref_value) / TMath::Sqrt(case_err * case_err + ref_err * ref_err);
    if (std::isnan(significance) || std::isinf(significance)) {
      significance = 0.;
    }
    signi_graph->SetPoint(i, ref_graph->GetPointX(i), significance);
  }
  return signi_graph;
}

int main() {

  using std::get;
  using std::string;
  using ::Tools::Define;
  using ::Tools::Define1;
  using ::Tools::Format;
  namespace Tools = Qn::Analysis::Tools;




  TFile f("correlation.root", "READ");
  LoadROOTFile<DTColl>(f.GetName(), "raw");

  /* Convert everything to Qn::DataContainerStatCalculate */
  gResourceManager.ForEach([](VectorKey key, DTColl &collect) {
    /* replacing /raw with /calc */
    key[0] = "calc";
    AddResource(key, Qn::DataContainerStatCalculate(collect));
  });

  remap_axes();


  /* label correlations */
  gResourceManager.ForEach([](const StringKey &key, ResourceManager::Resource &r) {
    using std::filesystem::path;
    using boost::algorithm::split;
    path key_path(key);

    auto obj_name = key_path.filename().string();

    std::vector<std::string> tokens;
    split(tokens, obj_name, boost::is_any_of("."));

    /* args are all tokens but last */
    for (size_t iarg = 0; iarg < tokens.size() - 1; ++iarg) {
      const std::regex arg_re("^(\\w+)_(PLAIN|RECENTERED|RESCALED)$");
      std::smatch match_results;
      std::regex_search(tokens[iarg], match_results, arg_re);
      auto arg_name_raw = match_results.str(0);
      auto arg_name = match_results.str(1);
      auto arg_cstep = match_results.str(2);
      r.meta.put("arg" + std::to_string(iarg) + ".raw-name", arg_name_raw);
      r.meta.put("arg" + std::to_string(iarg) + ".name", arg_name);
      r.meta.put("arg" + std::to_string(iarg) + ".c-step", arg_cstep);
    }
    r.meta.put("component", tokens.back());
  });

  {
    const std::string
        uQ_reco_expr("^/calc/uQ/(pion_neg|pion_pos|protons)_(pt|2d|y)_set_(\\w+)_(PLAIN|RECENTERED|TWIST|RESCALED).*$");
    gResourceManager.ForEach([&uQ_reco_expr](const StringKey &key, ResourceManager::Resource &r) {
      auto particle = KEY.MatchGroup(1, uQ_reco_expr)(r);
      auto axis = KEY.MatchGroup(2, uQ_reco_expr)(r);
      auto set = KEY.MatchGroup(3, uQ_reco_expr)(r);
      auto correction_step = KEY.MatchGroup(4, uQ_reco_expr)(r);
      r.meta.put("type", "uQ");
      r.meta.put("u.particle", particle);
      r.meta.put("u.axis", axis);
      r.meta.put("u.set", set);
      r.meta.put("u.src", "reco");
      r.meta.put("u.correction-step", correction_step);
    }, KEY.Matches(uQ_reco_expr));
  }
  {
    const std::string u_mc_expr("^/calc/uQ/mc_(pion_neg|pion_pos|protons)_(pt|2d|y)_PLAIN.*$");
    gResourceManager.ForEach([&u_mc_expr](const StringKey &key, ResourceManager::Resource &r) {
      auto particle = KEY.MatchGroup(1, u_mc_expr)(r);
      auto axis = KEY.MatchGroup(2, u_mc_expr)(r);
      r.meta.put("type", "uQ");
      r.meta.put("u.particle", particle);
      r.meta.put("u.axis", axis);
      r.meta.put("u.src", "mc");
    }, KEY.Matches(u_mc_expr));
  }


  /* To check compatibility with old stuff, multiply raw correlations by factor of 2 */
  gResourceManager.ForEach([](VectorKey key, const ResourceManager::Resource &r) {
    auto meta = r.meta;
    key[0] = "x2";
    meta.put("type", "x2");
    auto result = 2 * r.As<DTCalc>();
    AddResource(key, ResourceManager::Resource(result, meta));
  });

  {
    /* Projection _y correlations to pT axis  */
    gResourceManager.ForEach([](const StringKey &name, DTCalc &calc) {
                               calc = calc.Projection({"Centrality", "y_cm"});
                             },
                             META["u.axis"] == "y");
    /* Projection _pT correlations to 'y' axis  */
    gResourceManager.ForEach([](const StringKey &name, DTCalc &calc) {
                               calc = calc.Projection({"Centrality", "pT"});
                             },
                             META["u.axis"] == "pt");
  }
  {
//    /* Rebin y  */
//    gResourceManager.ForEach([](const StringKey &name, DTCalc &calc) {
//                               calc = calc.Rebin(Qn::AxisD("y_cm",
//                                                           {-0.6, -0.4, -0.2, 0.0, 0.2, 0.4, 0.6, 0.8, 1.0, 1.2, 1.4, 1.6, 2.0, 2.4}));
//                             },
//                             META["u.axis"] == "y");
//    /* Rebin pT  */
//    gResourceManager.ForEach([](const StringKey &name, DTCalc &calc) {
//                               calc = calc.Rebin(Qn::AxisD("pT",
//                                                           {0., 0.2, 0.4, 0.6, 0.8, 1.0, 1.2, 1.4, 1.8, 2.4, 3.0}));
//                             },
//                             META["u.axis"] == "pt");
//    /* Rebin centrality  */
//    gResourceManager.ForEach([](const StringKey &name, DTCalc &calc) {
//      calc = calc.Rebin(Qn::AxisD("Centrality",
//                                  {0., 10., 25., 45., 80.}));
//    });
  }

  /* processing */

  /* resolution */
  resolution_3sub();
  resolution_mc();
  resolution_4sub();

  /* v1 */
  v1();
  v1_mc();
  v1_centrality();
  v1_combine();



  const auto v1_reco_centrality_feature_set = v1_reco_feature_set() + "centrality.key";
  const auto v1_mc_centrality_feature_set = v1_mc_feature_set() + "centrality.key";


/****************** DRAWING *********************/

  // All to TGraph
  gResourceManager
      .ForEach([](const StringKey &name, DTCalc calc) {
                 auto cwd = gDirectory;
                 gDirectory = nullptr;
                 auto graph = Qn::ToTGraph(calc);
                 if (graph)
                   AddResource("/profiles" + name, graph);
                 gDirectory = cwd;
               },
               KEY.Matches("^/x2/QQ/.*$"));

  {
    const auto resolution_filter = META["type"] == "resolution";

    ::Predicates::MetaFeatureSet resolution_feature_list{
        "resolution.meta_key",
        "resolution.ref_alias",
        "resolution.component",
    };

    const std::string save_dir = "resolution";
    gSystem->mkdir(save_dir.c_str());

    gResourceManager.GroupBy(META["resolution.meta_key"], [&save_dir](
        auto feature,
        const std::vector<ResourceManager::ResourcePtr> &resolutions_list) {

      const std::map<std::string, int> colors = {
          {"psd1", kRed + 1},
          {"psd2", kGreen + 2},
          {"psd3", kBlue}
      };
      const std::map<std::string, int> markers = {
          {"X", kFullSquare},
          {"Y", kOpenSquare},
      };

      TMultiGraph mg;
      for (auto &r : resolutions_list) {
        auto resolution_calc = r->template As<DTCalc>();
        auto resolution_graph = Qn::ToTGraph(resolution_calc);
        resolution_graph->SetLineColor(colors.at(META["resolution.ref_alias"](*r)));
        resolution_graph->SetMarkerColor(colors.at(META["resolution.ref_alias"](*r)));
        resolution_graph->SetMarkerStyle(markers.at(META["resolution.component"](*r)));
        resolution_graph->SetTitle((Format("R_{1,%1%} (%2%)")
            % META["resolution.component"](r)
            % META["resolution.ref_alias"](r)).str().c_str());
        mg.Add(resolution_graph, "pl");

        {
          auto cwd = gDirectory;
          gDirectory = nullptr;
          /// place resolution to the resource manager
          auto res_profile_key = Tmpltor("/profiles/resolution/"
                                         "{{resolution.meta_key}}/"
                                         "RES_{{resolution.ref_alias}}_{{resolution.component}}")(r);
          auto meta = r->meta;
          meta.put("type", "profile_resolution");
          auto obj = (TGraphErrors *) resolution_graph->Clone("");
          AddResource(res_profile_key, ResourceManager::Resource(*obj, meta));
          gDirectory = cwd;
        }

      }

      TCanvas c;
      c.SetCanvasSize(1280, 1024);
      mg.Draw("A");
      mg.GetYaxis()->SetRangeUser(0., .4);
      mg.GetYaxis()->SetTitle("R_{1}");
      mg.GetXaxis()->SetTitle("Centrality (%)");
      auto legend = c.BuildLegend(0.15, 0.7, 0.55, 0.9);
      legend->SetHeader(feature.c_str());
      SaveCanvas(c, save_dir + "/" + feature);

      TFile f((save_dir + "/multigraphs.root").c_str(), "update");
      f.WriteTObject(&mg, ("mg_" + feature).c_str(), "Overwrite");
    }, resolution_filter);

    gResourceManager
        .GroupBy(
            resolution_feature_list - "resolution.meta_key",
            [&save_dir](auto feature, const std::vector<ResourceManager::ResourcePtr> &resolution_ptrs) {
              const std::map<std::string, int> colors = {
                  {"psd_mc", kBlack},
                  {"psd90_mc", kOrange + 1},
                  {"3sub_standard", kRed},
                  {"3sub_psd90", kGreen + 2},
                  {"4sub_preliminary", kBlue},
                  {"4sub_opt1", kMagenta},
                  {"4sub_opt2", kCyan + 1},
              };

              TMultiGraph mg;
              for (auto &r : resolution_ptrs) {
                auto resolution_calc = r->template As<DTCalc>();
                auto resolution_graph = Qn::ToTGraph(resolution_calc);
                resolution_graph->SetLineColor(colors.at(META["resolution.meta_key"](*r)));
                resolution_graph->SetMarkerColor(colors.at(META["resolution.meta_key"](*r)));
                resolution_graph->SetLineWidth(2.);
                resolution_graph->SetTitle(META["resolution.meta_key"](r).c_str());
                mg.Add(resolution_graph, "pZ");
              }

              TCanvas c;
              c.SetCanvasSize(1280, 1024);
              mg.Draw("A");
              mg.GetYaxis()->SetRangeUser(0., .3);
              mg.GetYaxis()->SetTitle("R_{1}");
              mg.GetXaxis()->SetTitle("Centrality (%)");
              auto legend = c.BuildLegend(0.15, 0.2, 0.65, 0.35);
              legend->SetHeader(feature.c_str());
              SaveCanvas(c, save_dir + "/" + "comp_" + feature);

              TFile f_multigraphs((save_dir + "/" + "multigraphs.root").c_str(), "update");
              f_multigraphs.WriteTObject(&mg, ("comp__" + feature).c_str(), "Overwrite");
            }, resolution_filter);

  }

  /* v1 profiles */
  // /profiles/v1/<particle>/<axis>/<centrality range>/
  gResourceManager
      .ForEach(
          [](VectorKey key, ResourceManager::Resource &resource) {
            auto cwd = gDirectory;
            gDirectory = nullptr;
            const std::map<std::string, std::string> remap_axis_name = {
                {"pT", "p_{T} (GeV/#it{c})"},
                {"y_cm", "#it{y}_{CM}"},
            };
            const std::map<std::string, int> colors_map = {
                {"psd1", kRed},
                {"psd2", kGreen + 2},
                {"psd3", kBlue},
                {"combined", kBlack},
                {"psi_rp", kBlack}
            };

            ResourceManager::Resource profile_resource;

            auto data = resource.As<DTCalc>();

            if (data.GetAxes().size() == 1) {
              auto selected_graph = Qn::ToTGraph(data);
              if (selected_graph) {
                selected_graph->SetName(key.back().c_str());
                selected_graph->GetXaxis()->SetTitle(remap_axis_name.at(resource.As<DTCalc>().GetAxes()[0].Name()).c_str());
                profile_resource.obj = *selected_graph;
              } else {
                return;
              }
            } else if (data.GetAxes().size() == 2) {
              auto selected_graph = Qn::ToTGraph2D(data);
              if (selected_graph) {
                selected_graph->SetName(key.back().c_str());
                selected_graph->GetXaxis()->SetTitle(remap_axis_name.at(resource.As<DTCalc>().GetAxes()[0].Name()).c_str());
                selected_graph->GetYaxis()->SetTitle(remap_axis_name.at(resource.As<DTCalc>().GetAxes()[1].Name()).c_str());
                selected_graph->SetMaximum(0.4);
                selected_graph->SetMinimum(-0.4);
                profile_resource.obj = *selected_graph;
              } else {
                return;
              }

            }

            VectorKey new_key(key.begin(), key.end());
            new_key.insert(new_key.begin(), "profiles");
            auto meta = resource.meta;
            if (META["type"](resource) == "v1_centrality") {
              meta.put("type", "profile_v1");
            } else if (META["type"](resource) == "ratio_v1") {
              meta.put("type", "profile_ratio");
            }
            profile_resource.meta = meta;
            AddResource(new_key, profile_resource);
            gDirectory = cwd;
          },
          META["type"] == "v1_centrality" ||
              META["type"] == "c1_centrality");


  {
    /* v1 (y,Pt) Multigraphs */
    ::Tools::ToRoot<TMultiGraph> root_saver("v1_multigraph.root", "RECREATE");
    gResourceManager
        .ForEach(
            [&root_saver, &v1_reco_centrality_feature_set](const StringKey &key, ResourceManager::Resource &resource) {
              auto syst_data = Qn::DataContainerSystematicError(resource.As<DTCalc>());
              {
                syst_data.AddSystematicSource("component");
                auto fs = v1_reco_centrality_feature_set - "v1.component";
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
                auto fs = v1_reco_centrality_feature_set - "v1.ref";
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
                  gse->SetDataOption(GraphSysErr::kHat);
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
                auto graph_list = Qn::ToGSE2D(syst_data, "pT", 0.0, 1e3);

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
                auto syst_data_inverted = syst_data;
                int ibin = 0;
                const auto i_ax_y_cm = [&syst_data_inverted] () {
                  auto &axes = syst_data_inverted.GetAxes();
                  return std::distance(begin(axes), find_if(begin(axes), end(axes), [] (const Qn::AxisD& ax) {
                    return ax.Name() == "y_cm";
                  }));
                }();
                for (auto &bin : syst_data_inverted) {
                  auto index = syst_data_inverted.GetIndex(ibin);
                  auto i_y_cm = index.at(i_ax_y_cm);
                  auto y_cm_lo = syst_data_inverted.GetAxes()[i_ax_y_cm].GetLowerBinEdge(i_y_cm);
                  auto y_cm_up = syst_data_inverted.GetAxes()[i_ax_y_cm].GetUpperBinEdge(i_y_cm);
                  auto y_cm_mid = 0.5*(y_cm_lo + y_cm_up);
                  if (y_cm_mid < 0) {
                    bin = bin * (-1.);
                  }
                  ibin++;
                }

                auto graph_list = Qn::ToGSE2D(syst_data_inverted, "y_cm", 0.005,
                                              1e2, 0.2);
                TMultiGraph mg_y_scan;
                int i_y_cm_slice = 0;
                for (auto obj : *graph_list) {
                  auto *gse = (GraphSysErr *) obj;

                  bool is_backward = [&syst_data_inverted, &i_y_cm_slice] () {
                    auto y_cm_lo = syst_data_inverted.GetAxis("y_cm").GetLowerBinEdge(i_y_cm_slice);
                    auto y_cm_up = syst_data_inverted.GetAxis("y_cm").GetUpperBinEdge(i_y_cm_slice);
                    auto y_cm_mid = 0.5*(y_cm_lo + y_cm_up);
                    return y_cm_mid < 0;
                  }();

                  int i_abs_ycm = [&syst_data_inverted, &i_y_cm_slice] () -> int {
                    const auto &y_cm_axis = syst_data_inverted.GetAxis("y_cm");
                    auto y_cm_lo = y_cm_axis.GetLowerBinEdge(i_y_cm_slice);
                    auto y_cm_up = y_cm_axis.GetUpperBinEdge(i_y_cm_slice);
                    auto y_cm_mid = 0.5*(y_cm_lo + y_cm_up);
                    return y_cm_axis.FindBin(TMath::Abs(y_cm_mid)) - y_cm_axis.FindBin(0);
                  }();

                  auto primary_color = ::Tools::GetRainbowPalette().at(i_abs_ycm % size(::Tools::GetRainbowPalette()));
                  auto alt_color = ::Tools::GetRainbowPastelPalette().at(i_abs_ycm % size(::Tools::GetRainbowPastelPalette()));
                  gse->SetDataOption(GraphSysErr::kHat);
                  gse->SetLineColor(primary_color);
                  gse->SetMarkerColor(primary_color);
                  gse->SetMarkerStyle(is_backward? kOpenCircle : kFullCircle);

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
                auto graph_list = Qn::ToGSE2D(syst_data, "y_cm", 0.0, 1e3);

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

  /*********** v1 systematics (component) ********************/
  {
    std::string base_dir = "v1_systematics/component";

    gResourceManager
        .GroupBy(
            v1_reco_centrality_feature_set - "v1.component",
            [&base_dir](auto feature, const std::vector<ResourceManager::ResourcePtr> &resources) {
              const std::map<std::string, int> colors = {
                  {"x1x1", kRed},
                  {"y1y1", kBlue + 1},
                  {"combined", kBlack},
              };
              const std::map<std::string, std::pair<double, double>> y_ranges{
                  {"protons__pt", {-0.1, 0.25}},
                  {"protons__y", {-0.05, 0.4}},
                  {"pion_neg__pt", {-0.15, 0.15}},
                  {"pion_neg__y", {-0.15, 0.15}},
                  {"pion_pos__pt", {-0.05, 0.1}},
                  {"pion_pos__y", {-0.1, 0.1}},
              };
              const Tmpltor save_dir_tmpl("{{v1.particle}}__{{v1.axis}}/"
                                          "{{centrality.key}}/{{v1.set}}");
              const Tmpltor file_name_tmpl(
                  "RES_{{v1.resolution.meta_key}}");

              auto customize_graph = [&colors](
                  const ResourceManager::ResourcePtr &r,
                  TGraph *graph) {
                graph->SetLineColor(colors.at(META["v1.component"](*r)));
                graph->SetMarkerColor(colors.at(META["v1.component"](*r)));
                graph->GetXaxis()->SetTitle(r->template As<DTCalc>().GetAxes()[0].Name().c_str());
                graph->SetTitle(META["v1.component"](r).c_str());
              };

              TMultiGraph mg;
              for (auto &r : resources) {
                auto &dt_calc = r->template As<DTCalc>();
                auto graph = Qn::ToTGraph(dt_calc);
                customize_graph(r, graph);
                mg.Add(graph, META["v1.component"](*r) == "combined" ? "lpZ" : "pZ");
              } // resources

              /*********** reference from MC **********/
              auto mc_ref_keys = gResourceManager.template GetMatching(
                  META["type"] == "v1_centrality" &&
                      META["centrality.no"] == META["centrality.no"](resources.front()) &&
                      META["v1.src"] == "mc" &&
                      META["v1.particle"] == META["v1.particle"](resources.front()) &&
                      META["v1.axis"] == META["v1.axis"](resources.front()) &&
                      META["v1.component"] == "combined");
              if (!mc_ref_keys.empty()) {
                auto mc_ref = gResourceManager.template Get(mc_ref_keys.front(),
                                                            ResourceManager::ResTag<ResourceManager::Resource>());
                auto mc_ref_graph = Qn::ToTGraph(mc_ref.template As<DTCalc>());
                mc_ref_graph->SetLineStyle(kDashed);
                mc_ref_graph->SetLineColor(kBlack);
                mc_ref_graph->SetTitle("MC");
                mc_ref_graph->SetMarkerStyle(kOpenSquare);
                mg.Add(mc_ref_graph, "lpZ");
              }

              auto save_dir = base_dir + "/" + save_dir_tmpl(resources.front());
              auto filename = file_name_tmpl(resources.front());

              gSystem->mkdir(save_dir.c_str(), true);
              TCanvas c;
              c.SetCanvasSize(1280, 1024);
              mg.Draw("A");
              auto ranges = y_ranges.at(
                  META["v1.particle"](*resources.front()) + "__" +
                      META["v1.axis"](*resources.front()));
              mg.GetYaxis()->SetRangeUser(ranges.first, ranges.second);
              mg.GetXaxis()->SetTitle(resources.front()->template As<DTCalc>().GetAxes()[0].Name().c_str());
              c.BuildLegend();
              SaveCanvas(c, save_dir + "/" + filename);

              TFile f_multigraphs((save_dir + "/multigraphs.root").c_str(), "update");
              f_multigraphs.WriteTObject(&mg, filename.c_str(), "Overwrite");


              /************* ratios & significance plot *************/
              auto ref_it = std::find_if(resources.begin(), resources.end(),
                                         META["v1.component"] == "combined");

              if (ref_it != resources.end()) {
                auto ref_v1 = (*ref_it)->template As<DTCalc>();

                TMultiGraph ratio_mg;
                for (const auto &r : resources) {
                  auto ratio = r->template As<DTCalc>() / ref_v1;
                  auto ratio_graph = Qn::ToTGraph(ratio);
                  customize_graph(r, ratio_graph);
                  ratio_graph->SetLineWidth(2.);
                  ratio_mg.Add(ratio_graph, META["v1.component"](*r) == "combined" ? "lpZ" : "pZ");
                }
                c.Clear();
                ratio_mg.Draw("A");
                ratio_mg.GetYaxis()->SetRangeUser(0., 2.);
                ratio_mg.GetXaxis()->SetTitle(resources.front()->template As<DTCalc>().GetAxes()[0].Name().c_str());
                c.BuildLegend();
                SaveCanvas(c, save_dir + "/" + "ratio_" + filename);
                f_multigraphs.WriteTObject(&ratio_mg, ("ratio_" + filename).c_str(), "Overwrite");

                TMultiGraph signi_mg;
                for (const auto &r: resources) {
                  /* calculate significance */
                  auto ref_graph = Qn::ToTGraph(ref_v1);
                  auto case_graph = Qn::ToTGraph(r->template As<DTCalc>());
                  auto signi_graph = CalcSignificance(case_graph, ref_graph);
                  customize_graph(r, signi_graph);
                  signi_graph->SetLineWidth(2.);
                  signi_mg.Add(signi_graph, META["v1.component"](*r) == "combined" ? "l" : "lp");
                }
                f_multigraphs.WriteTObject(&signi_mg, ("signi_" + filename).c_str(), "Overwrite");
              } // ref exists

            },
            META["type"] == "v1_centrality" &&
                (META["v1.axis"] == "y" || META["v1.axis"] == "pt") &&
//                META["centrality.no"] == "2" &&
                META["v1.ref"] == "combined");
  }

  {
    std::string base_dir = "v1_systematics/reference";

    gResourceManager
        .GroupBy(
            v1_reco_centrality_feature_set - "v1.ref",
            [&base_dir](auto feature, const std::vector<ResourceManager::ResourcePtr> &resources) {
              const std::map<std::string, int> colors = {
                  {"psd1", kRed},
                  {"psd2", kGreen + 2},
                  {"psd3", kBlue},
                  {"combined", kBlack},
              };
              const std::map<std::string, std::pair<double, double>> y_ranges{
                  {"protons__pt", {-0.1, 0.25}},
                  {"protons__y", {-0.05, 0.4}},
                  {"pion_neg__pt", {-0.15, 0.15}},
                  {"pion_neg__y", {-0.15, 0.15}},
                  {"pion_pos__pt", {-0.05, 0.1}},
                  {"pion_pos__y", {-0.1, 0.1}},
              };
              const Tmpltor save_dir_tmpl("{{v1.particle}}__{{v1.axis}}/"
                                          "{{centrality.key}}/{{v1.set}}");
              const Tmpltor file_name_tmpl(
                  "RES_{{v1.resolution.meta_key}}");

              TMultiGraph mg;

              for (auto &r : resources) {
                auto &dt_calc = r->template As<DTCalc>();
                auto graph = Qn::ToTGraph(dt_calc);
                graph->SetLineColor(colors.at(META["v1.ref"](r)));
                graph->SetMarkerColor(colors.at(META["v1.ref"](r)));
                graph->SetTitle(META["v1.ref"](r).c_str());
                graph->GetXaxis()->SetTitle(dt_calc.GetAxes()[0].Name().c_str());
                mg.Add(graph, META["v1.ref"](r) == "combined" ? "lpZ" : "pZ");
              }

              auto save_dir = base_dir + "/" + save_dir_tmpl(resources.front().operator*());
              auto filename = file_name_tmpl(resources.front().operator*());

              /*********** reference from MC **********/
              auto mc_ref_keys = gResourceManager.template GetMatching(
                  META["type"] == "v1_centrality" &&
                      META["centrality.no"] == META["centrality.no"](resources.front()) &&
                      META["v1.src"] == "mc" &&
                      META["v1.particle"] == META["v1.particle"](resources.front()) &&
                      META["v1.axis"] == META["v1.axis"](resources.front()) &&
                      META["v1.component"] == "combined");
              if (!mc_ref_keys.empty()) {
                auto mc_ref = gResourceManager.template Get(mc_ref_keys.front(),
                                                            ResourceManager::ResTag<ResourceManager::Resource>());
                auto mc_ref_graph = Qn::ToTGraph(mc_ref.template As<DTCalc>());
                mc_ref_graph->SetLineStyle(kDashed);
                mc_ref_graph->SetLineColor(kBlack);
                mc_ref_graph->SetTitle("MC");
                mc_ref_graph->SetMarkerStyle(kOpenSquare);
                mg.Add(mc_ref_graph, "lpZ");
              }

              gSystem->mkdir(save_dir.c_str(), true);
              TCanvas c;
              c.SetCanvasSize(1280, 1024);
              mg.Draw("A");
              auto ranges = y_ranges.at(
                  META["v1.particle"](*resources.front()) + "__" +
                      META["v1.axis"](*resources.front()));
              mg.GetYaxis()->SetRangeUser(ranges.first, ranges.second);
              auto legend = c.BuildLegend();
              SaveCanvas(c, save_dir + "/" + filename);
              TFile f_multigraphs((save_dir + "/multigraphs.root").c_str(), "update");
              f_multigraphs.WriteTObject(&mg, filename.c_str(), "Overwrite");

              /************* ratios & significance plot *************/
              auto ref_it = std::find_if(resources.begin(), resources.end(),
                                         META["v1.ref"] == "combined");
              if (ref_it != resources.end()) {
                auto ref_v1 = (*ref_it)->template As<DTCalc>();

                TMultiGraph ratio_mg;
                for (auto &r : resources) {
                  auto ratio = r->template As<DTCalc>() / ref_v1;
                  auto ratio_graph = Qn::ToTGraph(ratio);
                  ratio_graph->SetLineColor(colors.at(META["v1.ref"](*r)));
                  ratio_graph->SetMarkerColor(colors.at(META["v1.ref"](*r)));
                  ratio_graph->SetTitle(META["v1.ref"](*r).c_str());
                  ratio_graph->GetXaxis()->SetTitle(ratio.GetAxes()[0].Name().c_str());
                  ratio_graph->SetLineWidth(2.0);
                  ratio_mg.Add(ratio_graph, META["v1.ref"](*r) == "combined" ? "lpZ" : "pZ");
                }

                c.Clear();
                ratio_mg.Draw("A");
                ratio_mg.GetYaxis()->SetRangeUser(0.0, 2.0);
                auto legend = c.BuildLegend();
                Tmpltor header_tmpl{"RES: {{v1.resolution.meta_key}}"};
                legend->SetHeader(header_tmpl(resources.front()).c_str());
                SaveCanvas(c, save_dir + "/" + "ratio_" + filename);
                f_multigraphs.WriteTObject(&ratio_mg, ("ratio_" + filename).c_str(), "Overwrite");

                TMultiGraph signi_mg;
                for (auto &r : resources) {
                  auto ref_graph = Qn::ToTGraph(ref_v1);
                  auto case_graph = Qn::ToTGraph(r->template As<DTCalc>());
                  auto signi_graph = CalcSignificance(case_graph, ref_graph);
                  signi_graph->SetLineColor(colors.at(META["v1.ref"](r)));
                  signi_graph->SetMarkerColor(colors.at(META["v1.ref"](r)));
                  signi_graph->SetLineWidth(2.);
                  signi_graph->SetTitle(META["v1.ref"](r).c_str());
                  signi_mg.Add(signi_graph, META["v1.ref"](*r) == "combined" ? "l" : "lp");
                }
                f_multigraphs.WriteTObject(&signi_mg, ("signi_" + filename).c_str(), "Overwrite");
              } // reference exists
            },
            META["type"] == "v1_centrality" &&
                (META["v1.axis"] == "y" || META["v1.axis"] == "pt") &&
//                META["centrality.no"] == "2" &&
                META["v1.src"] == "reco" &&
                META["v1.component"] == "combined");
  }

  {
    std::string base_dir = "v1_systematics/resolution_method";

    gResourceManager
        .GroupBy(
            v1_reco_centrality_feature_set - "v1.resolution.meta_key",
            [&base_dir](auto feature, const std::vector<ResourceManager::ResourcePtr> &resources) {
              const std::map<std::string, int> colors = {
                  {"psd_mc", kBlack},
                  {"psd90_mc", kOrange + 1},
                  {"3sub_standard", kRed},
                  {"3sub_psd90", kGreen + 2},
                  {"4sub_preliminary", kBlue},
                  {"4sub_opt1", kMagenta},
                  {"4sub_opt2", kCyan + 1},
              };

              const Tmpltor save_dir_tmpl("{{v1.particle}}__{{v1.axis}}/"
                                          "{{centrality.key}}/{{v1.set}}");
              auto save_dir = base_dir + "/" + save_dir_tmpl(resources.front());

              gSystem->mkdir(save_dir.c_str(), true);
              TFile f_multigraphs((save_dir + "/multigraphs.root").c_str(), "update");

              TMultiGraph mg;
              for (auto &r : resources) {
                auto calc = r->template As<DTCalc>();
                auto graph = Qn::ToTGraph(calc);
                graph->SetTitle(META["v1.resolution.meta_key"](r).c_str());
                graph->SetLineColor(colors.at(META["v1.resolution.meta_key"](r)));
                graph->SetMarkerColor(colors.at(META["v1.resolution.meta_key"](r)));
                graph->SetLineWidth(2.);
                mg.Add(graph, "lpZ");
              }

              f_multigraphs.WriteTObject(&mg, "mg", "Overwrite");
            },
            META["type"] == "v1_centrality" &&
                (META["v1.axis"] == "y" || META["v1.axis"] == "pt") &&
//                META["centrality.no"] == "2" &&
                META["v1.src"] == "reco" &&
                META["v1.component"] == "combined" &&
                META["v1.ref"] == "combined"
        );
  }

  {
    float y_fit_lo = -0.4;
    float y_fit_hi = 0.4;

    ::Tools::ToRoot<TMultiGraph> root_saver("dv1_dy_slope.root", "RECREATE");
    gResourceManager
      .ForEach([y_fit_lo, y_fit_hi, &root_saver] (const StringKey &key, ResourceManager::Resource &resources) {
        auto data = resources.As<DTCalc>();
        auto slope_data = Qn::EvalSlopeND(data, "y_cm", y_fit_lo, y_fit_hi);
        auto slope_multigraph_pt = Qn::ToTMultiGraphSlope(slope_data, "pT");
        root_saver.operator()(key + "_pt", *slope_multigraph_pt);
        auto slope_multigraph_centrality = Qn::ToTMultiGraphSlope(slope_data, "Centrality");
        root_saver.operator()(key + "_centrality", *slope_multigraph_centrality);
      }, META["type"] == "v1" && META["v1.axis"] == "2d");
  }

  {
    float y_fit_lo = -0.4;
    float y_fit_hi = 0.4;
    std::string base_dir = "dv1_dy";
    /* dv1/dy */
//    TFile dv1dy_file("dv1dy.root", "recreate");
    gResourceManager
        .GroupBy(v1_reco_centrality_feature_set - "centrality.key",
                 [y_fit_lo, y_fit_hi](auto feature, const std::vector<ResourceManager::ResourcePtr> &resources) {
                   auto cwd = gDirectory;
                   gDirectory = nullptr;
                   auto slope_graph = new TGraphErrors;
                   slope_graph->SetName(("slope__" + feature).c_str());
                   auto offset_graph = new TGraphErrors;
                   offset_graph->SetName(("offset__" + feature).c_str());

                   int i_point = 0;
                   int n_set_points = 0;
                   for (auto &res_centrality : resources) {
                     auto centrality_lo = res_centrality->meta.template get<float>("centrality.lo");
                     auto centrality_hi = res_centrality->meta.template get<float>("centrality.hi");
                     auto centrality_mid = 0.5 * (centrality_lo + centrality_hi);
                     auto v1_y_graph = Qn::ToTGraph(res_centrality->template As<DTCalc>());
                     // "S" for "The result of the fit is returned in the TFitResultPtr (see below Access to the Fit Result) "
                     // "Q" for quiet mode
                     // "F" for "If fitting a polN, use the minuit fitter"
                     // "N" for "Do not store the graphics function, do not draw"
                     auto fit_result = v1_y_graph->Fit("pol1", "SQFN", "", y_fit_lo, y_fit_hi);
                     if (fit_result.Get() && fit_result->IsValid()) {
                       auto offset = fit_result->Value(0);
                       auto offset_erro = fit_result->Value(0);
                       auto slope = fit_result->Value(1);
                       auto slope_erro = fit_result->Error(1);
                       slope_graph->SetPoint(i_point, centrality_mid, slope);
                       slope_graph->SetPointError(i_point, 0.0, slope_erro);
                       offset_graph->SetPoint(i_point, centrality_mid, offset);
                       offset_graph->SetPointError(i_point, 0.0, offset_erro);
                       n_set_points++;
                     }
                     i_point++;
                     delete v1_y_graph;
                   }

                   if (n_set_points > 0) {
                     slope_graph->Sort();
                     offset_graph->Sort();

                     ResourceManager::Resource resource_slope(*slope_graph, {});
                     resource_slope.meta.put("type", "v1_slope");
                     resource_slope.meta.put_child("v1_slope", resources.front()->meta.get_child("v1"));
                     auto v1_slope_name = Tmpltor(
                         "/profiles/{{type}}/{{v1_slope.src}}/{{v1_slope.particle}}"
                         "/systematics/SET_{{v1_slope.set}}/RES_{{v1_slope.resolution.meta_key}}/REF_{{v1_slope.ref}}/{{v1_slope.component}}")
                         (resource_slope);
                     AddResource(v1_slope_name, resource_slope);
                   }

//                   dv1dy_file.cd();
//                   slope_graph.Write();
//                   offset_graph.Write();

                   gDirectory = cwd;
                 },
                 META["type"] == "v1_centrality" &&
                     META["v1.axis"] == "y"
//                     META["v1.component"] == "combined" &&
//                     META["v1.ref"] == "combined"
        );
  }


  /* Compare MC vs Data */
  {
    auto meta_key_gen = META["v1.particle"] + "__" + META["v1.axis"];
    auto centrality_predicate = META["centrality.key"] == "10-25";

    gResourceManager
        .GroupBy(
            meta_key_gen,
            [=](
                const std::string &meta_key,
                const std::vector<ResourceManager::ResourcePtr> &mc_correlations) {
              auto c_overview = new TCanvas;
              c_overview->SetCanvasSize(1280, 1024);

              std::map<std::string, TCanvas *> c_ratio_map{
                  {"x1x1", new TCanvas()},
                  {"y1y1", new TCanvas()},
                  {"combined", new TCanvas()},
              };
              std::map<std::string, std::pair<double, double>> ranges_map{
                  {"protons__pt", {-0.1, 0.25}},
                  {"protons__y", {-0.05, 0.4}},
                  {"pion_neg__pt", {-0.15, 0.15}},
                  {"pion_neg__y", {-0.15, 0.15}},
              };
              for (auto &&[component, canv_ptr] : c_ratio_map) {
                canv_ptr->SetCanvasSize(1280, 1024);
              }

              bool overview_first = true;
              for (auto &mc_ref : mc_correlations) {
                auto mc_ref_calc = mc_ref->As<DTCalc>();

                /* plot to overview */
                auto mc_ref_graph = Qn::ToTGraph(mc_ref_calc);

                const auto component = META["v1.component"](*mc_ref);

                auto line_style = kSolid;
                auto draw_opts = overview_first ? "Al" : "l";

                mc_ref_graph->GetXaxis()->SetTitle(mc_ref_calc.GetAxes()[0].Name().c_str());
                mc_ref_graph->GetYaxis()->SetTitle("v_{1}");
                mc_ref_graph->GetYaxis()->SetRangeUser(ranges_map.at(meta_key).first, ranges_map.at(meta_key).second);
                mc_ref_graph->SetLineStyle(line_style);
                mc_ref_graph->SetTitle(PlotTitle(*mc_ref).c_str());
                mc_ref_graph->SetLineWidth(2.);

                c_overview->cd();
                mc_ref_graph->DrawClone(draw_opts);
                overview_first = false;

                auto ratio_self_calc = mc_ref_calc / mc_ref_calc;
                auto ratio_self_graph = Qn::ToTGraph(ratio_self_calc);
                ratio_self_graph->SetTitle(PlotTitle(*mc_ref).c_str());
                ratio_self_graph->SetLineStyle(kSolid);
                ratio_self_graph->SetLineWidth(2.);
                ratio_self_graph->SetLineColor(kBlack);
                for (auto&&[component, canvas_ptr] : c_ratio_map) {
                  canvas_ptr->cd();
                  ratio_self_graph->Draw("Al");
                  ratio_self_graph->GetXaxis()->SetTitle(mc_ref_calc.GetAxes()[0].Name().c_str());
                  ratio_self_graph->GetYaxis()->SetRangeUser(0.0, 2.0);
                }

                gResourceManager.ForEach(
                    [=](const StringKey &key, ResourceManager::Resource &reco) {
                      const std::map<std::string, int> ref_rgb_colors{
                          {"psd1", kRed + 1},
                          {"psd1_90", kRed + 1},
                          {"psd2", kGreen + 2},
                          {"psd2_90", kGreen + 2},
                          {"psd3", kBlue},
                          {"psd3_90", kBlue},
                          {"combined", kBlack}
                      };

                      const auto reco_ref = META["v1.ref"](reco);
                      const auto reco_component = META["v1.component"](reco);
                      const auto setup = META["v1.set"](reco);
                      const auto resolution_key = META["v1.resolution.meta_key"](reco);

                      const auto draw_color = ref_rgb_colors.at(reco_ref);
                      const auto draw_marker_style = [reco_component, reco_ref]() {
                        if (reco_component == "x1x1")
                          return kFullCircle;
                        else if (reco_component == "y1y1")
                          return kOpenCircle;
                        else if (reco_component == "combined")
                          return kFullSquare;

                        assert(false);
                      }();

                      auto reco_calc = reco.As<DTCalc>();
                      auto reco_graph = Qn::ToTGraph(reco_calc);

                      auto ratio_calc = reco_calc / mc_ref_calc;
                      ratio_calc.SetErrors(Qn::StatCalculate::ErrorType::BOOTSTRAP);

                      auto ratio_graph = Qn::ToTGraph(ratio_calc);

                      ratio_graph->SetTitle(PlotTitle(reco).c_str());
                      ratio_graph->SetLineColor(draw_color);
                      ratio_graph->SetMarkerColor(draw_color);
                      ratio_graph->SetMarkerStyle(draw_marker_style);

                      reco_graph->SetTitle(PlotTitle(reco).c_str());
                      reco_graph->SetLineColor(draw_color);
                      reco_graph->SetMarkerColor(draw_color);
                      reco_graph->SetMarkerStyle(draw_marker_style);


                      /* ratio */
                      c_ratio_map.at(reco_component)->cd();
                      ratio_graph->DrawClone("pZ");

                      c_overview->cd();
                      reco_graph->DrawClone("pZ");
                      delete ratio_graph;
                      delete reco_graph;
                    },
                    meta_key_gen == meta_key &&
                        META["type"] == "v1_centrality" &&
                        META["v1.src"] == "reco" &&
                        centrality_predicate &&
                        (
//                            (META["v1.set"] == "standard") ||
                        (META["v1.set"] == "standard" && META["v1.resolution.meta_key"] == "psd_mc")
                    ) &&
                        (
                            META["v1.component"].Matches("^(x1x1|y1y1)$") && META["v1.ref"].Matches("psd[0-9]") ||
                                META["v1.component"] == "combined" && META["v1.ref"] == "combined"
                        ));
              }

              c_overview->BuildLegend();
              c_overview->Print((meta_key + ".png").c_str(), "png");
              c_overview->Print((meta_key + ".C").c_str(), "C");
              for (auto &ratio: c_ratio_map) {
                ratio.second->BuildLegend();
                ratio.second->Print((meta_key + "__" + ratio.first + ".png").c_str(), "png");
                ratio.second->Print((meta_key + "__" + ratio.first + ".C").c_str(), "C");
                delete ratio.second;
              }
              delete c_overview;
            },
            META["type"] == "v1_centrality" &&
                META["v1.src"] == "mc" &&
                centrality_predicate &&
                META["v1.component"] == "combined");
  }


  /***************** SAVING OUTPUT *****************/
  {
    using ::Tools::ToRoot;
    gResourceManager.ForEach(ToRoot<Qn::DataContainerStatCalculate>("correlation_proc.root"));
    gResourceManager.ForEach(ToRoot<TGraphErrors>("prof.root"));
    gResourceManager.ForEach(ToRoot<TGraph2DErrors>("prof.root", "UPDATE"));
    gResourceManager.ForEach(ToRoot<TGraphAsymmErrors>("prof.root", "UPDATE"));
  }
  return 0;
}
