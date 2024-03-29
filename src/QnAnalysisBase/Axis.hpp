#ifndef FLOW_SRC_BASE_AXISCONFIG_H_
#define FLOW_SRC_BASE_AXISCONFIG_H_

#include <TObject.h>

#include <QnTools/Axis.hpp>

#include <QnAnalysisBase/Variable.hpp>
#include "AnalysisTree.hpp"

namespace Qn::Analysis::Base {

struct AxisConfig : public TObject {
  enum EAxisType { RANGE,
                   BIN_EDGES };

  VariableConfig variable;

  EAxisType type{RANGE};
  int nb{0};
  double lo{0.};
  double hi{0.};

  std::vector<double> bin_edges;

  ClassDef(Qn::Analysis::Base::AxisConfig, 2);
};

struct Axis {
  Axis() = default;
  Axis(ATVariable var, int nbins, float min, float max) : var_(std::move(var)),
                                                                      axis_(Qn::AxisD(var_.GetName(), nbins, min, max)) {}

  Axis(ATVariable var, std::vector<double> bin_edges) : var_(std::move(var)),
                                                                    axis_(Qn::AxisD(var_.GetName(), std::move(bin_edges))) {}

  const ATVariable& GetVariable() const { return var_; }
  const Qn::AxisD& GetQnAxis() const { return axis_; }

  ATVariable var_{};
  Qn::AxisD axis_{};
};

}// namespace Qn::Analysis::Base

#endif//FLOW_SRC_BASE_AXISCONFIG_H_
