//
// Created by eugene on 04/05/2021.
//

#ifndef QNANALYSIS_SRC_QNANALYSISOBSERVABLESEK_QNSYSTEMATICERROR_HPP_
#define QNANALYSIS_SRC_QNANALYSISOBSERVABLESEK_QNSYSTEMATICERROR_HPP_

#include <DataContainer.hpp>

namespace Qn {

class SystematicError {
  double value;
  double statistical_error;
  std::vector<double> systematic_error;
};

using DataContainerSystematicError = DataContainer<SystematicError>;

} // namespace Qn

#endif //QNANALYSIS_SRC_QNANALYSISOBSERVABLESEK_QNSYSTEMATICERROR_HPP_
