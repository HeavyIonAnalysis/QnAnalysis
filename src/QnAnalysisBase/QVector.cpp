#include "QVector.hpp"

ClassImp(Qn::Analysis::Base::QVectorConfig)

    namespace Qn::Analysis::Base {

  std::vector<AnalysisTree::Variable> QVectorTrack::GetListOfVariables() const {
    std::vector<AnalysisTree::Variable> vars{phi_, weight_};
    for (const auto& cut : cuts_) {
      vars.emplace_back(cut.GetVariable());
    }
    for (const auto& axis : axes_) {
      vars.emplace_back(axis.GetVariable());
    }
    for (const auto & histogram : qa_histograms_) {
      for (const auto & axis : histogram.axes) {
        vars.emplace_back(axis.GetVariable());
      }
    }
    // remove Ones and Filled
    auto new_end = std::remove_if(vars.begin(), vars.end(),
                                  [](const AnalysisTree::Variable& var) {
                                    return EndsWith(var.GetName(), "Ones") || EndsWith(var.GetName(), "Filled");
                                  });
    vars.erase(new_end, vars.end());
    return vars;
  }

  std::vector<Qn::AxisD> QVectorTrack::GetAxes() const {
    std::vector<Qn::AxisD> axes{};
    for (const auto& axis : axes_) {
      axes.emplace_back(axis.GetQnAxis());
    }
    return axes;
  }
}