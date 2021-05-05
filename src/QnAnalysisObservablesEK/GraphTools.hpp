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

inline
std::vector<Color_t> GetRainbowPalette() {
  return {
      kRed + 4,
      kRed + 1,
      kOrange + 9,
      kYellow +2,
      kSpring + 3,
      kGreen + 1,
      kTeal - 1,
      kCyan + 3,
      kGreen + 4,
      kBlue,
      kBlue + 3,
      kMagenta,
      kMagenta + 3,
      kGray + 1,
      kYellow -2,
  };
}

inline
std::vector<Color_t> GetRainbowPastelPalette() {
  return {
      kRed - 9,
      kRed - 8,
      kOrange + 6,
      kOrange - 9,
      kSpring + 7,
      kGreen - 9,
      kTeal - 9,
      kCyan - 9,
      kGreen - 8,
      kBlue - 10,
      kBlue - 9,
      kMagenta - 9,
      kMagenta - 6,
      kGray,
      kYellow - 9,
  };
}



}

#endif //QNANALYSIS_SRC_QNANALYSISOBSERVABLESEK_GRAPHTOOLS_HPP_
