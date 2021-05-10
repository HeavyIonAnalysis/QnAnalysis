//
// Created by eugene on 05/05/2021.
//

#ifndef QNANALYSIS_SRC_QNANALYSISOBSERVABLESEK_GRAPHTOOLS_HPP_
#define QNANALYSIS_SRC_QNANALYSISOBSERVABLESEK_GRAPHTOOLS_HPP_

#include <TColor.h>
#include <TGraph.h>
#include <TMultiGraph.h>

namespace Tools {

inline void GraphShiftX(TGraph* graph, float shift_x) {
  auto n = graph->GetN();
  for (int ipoint = 0; ipoint < n; ++ipoint) {
    auto old_x = graph->GetX()[ipoint];
    auto new_x = old_x + shift_x;
    graph->GetX()[ipoint] = new_x;
  }
}

inline void GraphShiftX(TMultiGraph* multigraph, float shift_x) {
  auto graphs = multigraph->GetListOfGraphs();
  for (int i_graph = 0; i_graph < graphs->GetEntries(); ++i_graph) {
    auto graph = (TGraph *) graphs->At(i_graph);
    GraphShiftX(graph, shift_x);
  }
}

inline void GraphSetErrorsX(TGraphAsymmErrors *graph, float error_x) {
  auto n = graph->GetN();
  for (int ipoint = 0; ipoint < n; ++ipoint) {
    auto ylo = graph->GetErrorYlow(ipoint);
    auto yhi = graph->GetErrorYhigh(ipoint);
    graph->SetPointError(ipoint, error_x, error_x, ylo, yhi);
  }
}


inline
std::vector<Color_t> GetRainbowPalette() {
  return {kRed+1,
          kOrange+7,
          kYellow+1,
          kGreen+2,
          kAzure+1,
          kBlue+1};
}

inline
std::vector<Color_t> GetRainbowPastelPalette() {
  return {
    kRed-7,
    kOrange+1,
    kYellow-0,
    kGreen-7,
    kAzure+6,
    kBlue-7
  };
}



}

#endif //QNANALYSIS_SRC_QNANALYSISOBSERVABLESEK_GRAPHTOOLS_HPP_
