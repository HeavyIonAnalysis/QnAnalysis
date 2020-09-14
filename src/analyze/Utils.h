#ifndef DATATREEFLOW_UTILS_H
#define DATATREEFLOW_UTILS_H

#include <utility>
#include <vector>
#include <memory>
#include <regex>
#include <functional>

#include <Rtypes.h>
#include <TDirectory.h>
#include <TKey.h>
#include <TPaveText.h>
#include <TFile.h>

#include <QnTools/DataContainer.hpp>

#define FMT_HEADER_ONLY
#include "fmt/core.h"
#include "fmt/format.h"
#include "fmt/printf.h"

using Result = Qn::DataContainerStats;
using ResultPtr = std::shared_ptr<Result>;

static TPaveText *MakePaveText(const std::vector<std::string> &lines, std::vector<Double_t> position) {
  auto *pave = new TPaveText;
  pave->SetX1NDC(position.at(0));
  pave->SetY1NDC(position.at(1));
  pave->SetX2NDC(position.at(2));
  pave->SetY2NDC(position.at(3));

  for (const auto &line : lines) {
    pave->AddText(line.c_str());
  }

  pave->SetBorderSize(0);
  pave->SetFillColor(kWhite);
  pave->SetFillStyle(0); // transparent
  pave->SetTextAlign(11);
  return pave;
}

struct Selection {
  std::string axis_{};
  float min_{0};
  float max_{0};

  Selection() = delete;
  Selection (const std::string& name, float min, float max) :
    axis_(name), min_(min), max_(max) {};

  Result operator()(std::vector<Result> args, const std::vector<std::string> &) const {
    assert(args.size() == 1);
    const auto& r = args[0];

    std::string axis_name{};
    std::vector<std::string> axis_names{};
    for (const Qn::AxisD &ax : r.GetAxes()) {
      if (ax.Name().find(axis_) < ax.Name().size()) {
        axis_name = ax.Name();
      }
      else{
        axis_names.emplace_back(ax.Name());
      }
    }
    if (axis_name.empty()) {
      throw std::runtime_error("No axis found");
    }
    Qn::AxisD newAxis(axis_name, 1, min_, max_);
    return r.Rebin(newAxis).Projection(axis_names);
  }
};

static Result Select(const Result &r, std::string axis, float min, float max) {
  for (const Qn::AxisD &ax : r.GetAxes()) {
    if (ax.Name().find(axis) < ax.Name().size()) {
      axis = ax.Name();
      break;
    }
  }
  if (axis.empty()) {
    throw std::runtime_error("No axis found");
  }
  Qn::AxisD newAxis(axis, 1, min, max);
  return r.Select(newAxis);
}


static Result RebinRapidity(const Result &r, const std::vector<double> &binEdges) {
  std::string axRapidityName;
  // find rapidity axis
  for (const Qn::AxisD &ax : r.GetAxes()) {
    if (ax.Name().find("rapidity") < ax.Name().size()) {
      axRapidityName = ax.Name();
      break;
    }
  }

  if (axRapidityName.empty()) {
    throw std::runtime_error("No rapidity axis found");
  }

  Qn::AxisD newRapidityAxis(axRapidityName, binEdges);

  return r.Rebin(newRapidityAxis);
}

static Result SetReference(const Result &r) {
  Result rNew{r};

  for (Qn::Stats &stats : rNew) {
    stats.SetWeights(Qn::Stats::Weights::REFERENCE);
  }

  return rNew;
}

template<class T>
static bool GetResource(TDirectory *d, const std::string &name, std::shared_ptr<T> &container) {
  auto obj = dynamic_cast<T *>(d->Get(name.c_str()));
  if (!obj) {
    container.reset();
    return false;
  }

  container = std::shared_ptr<T>(obj);
  return true;
}

/**
 *
 * @param mg
 * @param dX
 */
static void ShiftGraphsX(TMultiGraph &mg, double dX = 0.0) {

  for (int ig = 0; ig < mg.GetListOfGraphs()->GetEntries(); ++ig) {
    double xShift = ig * dX;
    if (xShift == 0.) continue;

    auto graph = dynamic_cast<TGraph *>(mg.GetListOfGraphs()->At(ig));

    int n = graph->GetN();
    double *xx = graph->GetX();

    // populate new xx array
    double xx_new[n];
    for (int ix = 0; ix < n; ++ix) {
      xx_new[ix] = xx[ix] + xShift;
    }

    // replace xx with contents of xx_new
    std::memcpy(xx, xx_new, sizeof(double) * n);
  }

}

/**
 * Returns vector of objects with name matching regex pattern
 * from directory
 * @tparam T - type of object
 * @param d - directory containing objects
 * @param pattern - regex pattern string
 * @return vector of tuples with name, vector of group captures and pointer to an object
 */
template<class T>
static std::vector<std::tuple<std::string,
                       std::vector<std::string>,
                       std::shared_ptr<T> > > GetResourcesMatchingName(TDirectory *d, const std::string &pattern) {
  std::regex re(pattern);
  std::vector<std::tuple<std::string, std::vector<std::string>, std::shared_ptr<T> > > result{};

  auto keys = d->GetListOfKeys();
  for (auto objPtr : *keys) {
    auto key = dynamic_cast<TKey *>(objPtr);
    std::string name{key->GetName()};
    std::smatch match;

    if (std::regex_match(name, match, re)) {
      std::vector<std::string> matches{};
      for (unsigned int i = 0; i < match.size(); ++i) {
        matches.push_back(match[i].str());
      }

      auto objTPtr = std::shared_ptr<T>(dynamic_cast<T *> (key->ReadObj()));
      if (objTPtr) {
        result.push_back({std::move(name), std::move(matches), objTPtr});
      } else {
        Warning(__func__, "Object '%s' is not of the required type", name.c_str());
      }
    }
  }

  return result;
}

struct ProfileExporter {

  std::shared_ptr<TFile> outputFile_;
  std::string ouputFileName_;

  ProfileExporter() = default;

  explicit ProfileExporter(const std::string &outputFileName) {
    SaveTo(outputFileName);
  }

  ProfileExporter &SaveTo(const std::string &outputFileName) {
    this->ouputFileName_ = outputFileName;
    return *this;
  }

  std::string saveFolder{""};

  ProfileExporter &Folder(const std::string &_saveFolder = "") {
    saveFolder = _saveFolder;
    return *this;
  }

  std::shared_ptr<Qn::AxisD> rebinAxisPtr{};
  ProfileExporter &Rebin(const Qn::AxisD &_axis) {
    rebinAxisPtr = std::make_shared<Qn::AxisD>(_axis);
    return *this;
  }
  ProfileExporter &Rebin() {
    rebinAxisPtr.reset();
    return *this;
  }

  bool isCorrelatedErrors{false};
  ProfileExporter &CorrelatedErrors(bool _correlatedErrors = true) {
    isCorrelatedErrors = _correlatedErrors;
    return *this;
  }

  bool isUnfold{false};
  ProfileExporter &Unfold(bool _unfold = true) {
    isUnfold = _unfold;
    return *this;
  }

  std::string prefix{""};
  std::string suffix{""};
  std::string mgTitlePattern{"{}-{}%"};

  bool CheckOutputFile() {
    if (outputFile_ && outputFile_->IsOpen()) {
      // do nothing
      return true;
    }

    outputFile_.reset(TFile::Open(ouputFileName_.c_str(), "UPDATE"));
    return bool(outputFile_);
  }

  void operator()(const std::string &resourceName, Result result) {
    using std::string;

    // prepare result for exporting
    if (rebinAxisPtr) {
      result = result.Rebin(*rebinAxisPtr);
    }

    if (isCorrelatedErrors) {
      result.SetSetting(Qn::Stats::Settings::CORRELATEDERRORS);
    }

    // generate new name
    string profileExportName;
    if (!prefix.empty()) {
      profileExportName.append(prefix).append("_");
    }
    profileExportName.append(resourceName);
    if (!suffix.empty()) {
      profileExportName.append("_").append(suffix);
    }

    if (!CheckOutputFile()) {
      throw std::runtime_error("No output file");
    }

    TDirectory *exportDir = outputFile_.get();
    if (!saveFolder.empty()) {
      outputFile_->GetObject(saveFolder.c_str(), exportDir);

      if (!exportDir) {
        exportDir = outputFile_->mkdir(saveFolder.c_str());
      }
    }

    if (result.GetAxes().size() == 2) {
      std::string axisname{"Centrality"};
      auto profileMultigraph = new TMultiGraph();
      Qn::AxisD axis;
      try { axis = result.GetAxis(axisname); } // TODO
      catch (std::exception &) {
        // TODO return?
        throw std::logic_error("axis not found");
      }
      for (unsigned int ibin = 0; ibin < axis.size(); ++ibin) {
        auto subresult = result.Select({axisname, {axis.GetLowerBinEdge(ibin), axis.GetUpperBinEdge(ibin)}});
        // shift subgraph points to the center of bins
        auto subgraph = Qn::DataContainerHelper::ToTGraphShifted(subresult,
                                                                 1,
                                                                 2,
                                                                 Qn::DataContainerHelper::Errors::Yonly);
        subgraph->SetTitle(fmt::format(mgTitlePattern, axis.GetLowerBinEdge(ibin), axis.GetUpperBinEdge(ibin)).c_str());
        subgraph->SetMarkerStyle(kFullCircle);
        profileMultigraph->Add(subgraph);

        if (isUnfold) {
          string graphName = profileExportName + "_" + std::to_string(ibin);
          exportDir->WriteObject(subgraph, graphName.c_str());
          Info(__func__, "Graph '%s' is exported", graphName.c_str());
        }
      }

      exportDir->WriteObject(profileMultigraph, profileExportName.c_str());
      Info(__func__, "'%s' is exported as TMultiGraph '%s'", resourceName.c_str(), profileExportName.c_str());
    } else if (result.GetAxes().size() == 1) {
      auto profileGraph = Qn::DataContainerHelper::ToTGraph(result);
      // 1d graph is shifted such that points are in the bin centers
      // no need to shift it
      exportDir->WriteObject(profileGraph, profileExportName.c_str());
      Info(__func__, "Graph '%s' is exported as '%s'", resourceName.c_str(), profileExportName.c_str());
    }

  }

};


#endif //DATATREEFLOW_UTILS_H
