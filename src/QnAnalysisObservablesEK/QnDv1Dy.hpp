//
// Created by eugene on 06/05/2021.
//

#ifndef QNANALYSIS_SRC_QNANALYSISOBSERVABLESEK_QNDV1DY_HPP_
#define QNANALYSIS_SRC_QNANALYSISOBSERVABLESEK_QNDV1DY_HPP_

#include <DataContainer.hpp>
#include <StatCalculate.hpp>
#include <TGraph.h>

namespace Qn {

class Dv1Dy {
 public:
  double GetOffset() const {
    return offset;
  }
  double GetOffsetError() const {
    return offset_error;
  }
  double GetSlope() const {
    return slope;
  }
  double GetSlopeError() const {
    return slope_error;
  }
  bool IsFitValid() const {
    return fit_valid;
  }

 private:
  friend Dv1Dy EvalSlope1D(DataContainer<Qn::StatCalculate>& data, double fit_lo, double fit_hi);

  double offset{0.0};
  double offset_error{0.0};
  double slope{0.0};
  double slope_error{0.0};
  bool fit_valid{false};
};

typedef DataContainer<Dv1Dy> DataContainerDv1Dy;

Dv1Dy EvalSlope1D(DataContainer<Qn::StatCalculate>& data, double fit_lo, double fit_hi);
DataContainerDv1Dy EvalSlopeND(DataContainer<Qn::StatCalculate>& data, const std::string& fit_axis, double fit_lo, double fit_hi);

TGraph *ToTGraphSlope(const DataContainerDv1Dy& data);
TMultiGraph *ToTMultiGraphSlope(const DataContainerDv1Dy& data, const std::string& selection_axis_name);

} // namespace Qn



#endif //QNANALYSIS_SRC_QNANALYSISOBSERVABLESEK_QNDV1DY_HPP_
