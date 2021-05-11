//
// Created by eugene on 11/05/2021.
//
#include "Observables.hpp"
#include "using.hpp"

#include <boost/lexical_cast.hpp>

#include <string>
#include <map>

#include "TFile.h"
#include "TCanvas.h"
#include "TLegend.h"
#include "TPaveText.h"
#include "TLine.h"
#include "TSystem.h"

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

template <typename T>
struct TFileObjectPtr {
  std::shared_ptr<TFile> src{nullptr};
  T *ptr{nullptr};
  T* operator->() { return ptr; }
  T& operator* () { return *ptr; }
};

template<typename T>
void LoadROOTFile(const std::string &file_name, const std::string &manager_prefix = "") {
  auto f = std::make_shared<TFile>(file_name.c_str(), "READ");

  for (const auto &path : Details::FindTDirectory(*f)) {
    auto raw_ptr = f->Get<T>(path.c_str());
    if (raw_ptr) {
      auto manager_path = manager_prefix.empty() ? path : "/" + manager_prefix + path;
      std::cout << "Adding path '" << manager_path << "'" << std::endl;
      TFileObjectPtr<T> obj_ptr;
      obj_ptr.src = f;
      obj_ptr.ptr = raw_ptr;
      AddResource(manager_path, obj_ptr);
    }
  }
}

int main() {
  using ::Predicates::Resource::KEY;

  LoadROOTFile<TMultiGraph>("v1_multigraph.root", "");

  std::string base_dir = "v1_centrality";
  gSystem->mkdir(base_dir.c_str());
  /* /v1_centrality/reco/pion_neg/AX_2d/centrality_0-5/systematics/SET_standard/RES_4sub_opt2/REF_combined/v1_y */
  const std::string re_string("/v1_centrality/reco/"
                        "(pion_neg|pion_pos|protons)/AX_2d/centrality_([\\d\\.]+-[\\d\\.]+)/systematics/SET_(standard|preliminary)/RES_4sub_opt2/REF_combined/v1_(y|pt)");
  gResourceManager.GroupBy(
      "v1_" + KEY.MatchGroup(4, re_string) + "__" + KEY.MatchGroup(1, re_string) + "__" + KEY.MatchGroup(3, re_string),
      [=] (auto feature, std::vector<ResourceManager::ResourcePtr> &data) {

        const std::map<std::string, std::pair<double, double>> y_ranges = {
            {"protons", {-0.1, 0.6}},
            {"pion_neg", {-0.4, 0.3}},
            {"pion_pos", {-0.4, 0.3}},
        };

        const std::map<std::string, std::pair<double, double>> x_ranges = {
            {"y", {-0.6, 2.2}},
            {"pt", {0.0, 2.2}}
        };

        TFile canvases_root((base_dir + "/" + feature + ".root").c_str(), "recreate");

        auto canvas_ptr = std::make_unique<TCanvas>();
        canvas_ptr->SetBatch(true);
        canvas_ptr->SetCanvasSize(800, 600);

        std::string pdf_path = base_dir + "/" + feature + ".pdf";
        canvas_ptr->Print((pdf_path + "(").c_str(), "pdf");

        for (auto &centrality_class_data : data) {
          auto centrality_string = KEY.MatchGroup(2, re_string)(*centrality_class_data);
          boost::regex re_centrality_string(R"(([\d\.]+)-([\d\.]+))");
          boost::smatch match_results;
          assert(boost::regex_search(centrality_string, match_results, re_centrality_string));

          auto centrality_lo = boost::lexical_cast<double>(match_results[1].str());
          auto centrality_hi = boost::lexical_cast<double>(match_results[2].str());
          auto particle = KEY.MatchGroup(1, re_string)(data.front());
          auto axis = KEY.MatchGroup(4, re_string)(data.front());

          canvas_ptr->Clear();
          auto mg = centrality_class_data->As<TFileObjectPtr<TMultiGraph>>();


          auto legend = [=,&mg] () {
            auto result = new TLegend;
            auto graphs = mg->GetListOfGraphs();
            for (auto o : *graphs) {
              if (std::string(o->GetTitle()) == "All errors")
                continue;
              result->AddEntry(o, o->GetTitle(), "lp");
            }
            result->SetBorderSize(0);
            return result;
          }();

          auto description = [=] () {
            auto pave = new TPaveText(0.15, 0.75, 0.45, 0.85, "ndc");
            pave->SetBorderSize(0);
            pave->AddText(Form("v_{1}, SP, %s", particle.c_str()));
            pave->AddText(Form("Centrality: %.1f-%.1f", centrality_lo, centrality_hi));
            return pave;
          } ();

          TLine line(x_ranges.at(axis).first, 0.0, x_ranges.at(axis).second, 0.0);
          line.SetLineColor(kBlack);
          line.SetLineStyle(kDashed);
          line.SetLineWidth(2.0);


          canvas_ptr->cd();
          canvas_ptr->Clear();
          mg->GetHistogram()->Draw("AXIS");
          mg->GetXaxis()->SetLimits(x_ranges.at(axis).first, x_ranges.at(axis).second);
          mg->GetYaxis()->SetRangeUser(y_ranges.at(particle).first, y_ranges.at(particle).second);
          gPad->Modified();
          gPad->Update();
          line.DrawClone();
          mg->DrawClone("");
          legend->Draw();
          description->Draw();
          canvas_ptr->Print(pdf_path.c_str());
          canvases_root.WriteObject(canvas_ptr.get(), centrality_string.c_str());
        }

        canvas_ptr->Print((pdf_path + ")").c_str(), "pdf");
      }, KEY.Matches(re_string));

  return 0;
}