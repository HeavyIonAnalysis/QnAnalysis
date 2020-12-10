//
// Created by eugene on 10/12/2020.
//

#ifndef QNANALYSIS_SRC_QNANALYSISOBSERVABLES_METHODS_HPP
#define QNANALYSIS_SRC_QNANALYSISOBSERVABLES_METHODS_HPP

#include <QnTools/DataContainer.hpp>

namespace Methods {

inline
Qn::DataContainerStatCalculate
Resolution3S(const Qn::DataContainerStatCalculate &nom1,
             const Qn::DataContainerStatCalculate &nom2,
             const Qn::DataContainerStatCalculate &denom) {
  auto nom = nom1 * nom2;
  nom = 2 * nom;
  return Qn::Sqrt(nom / denom);
}

} /// namespace Methods


#endif //QNANALYSIS_SRC_QNANALYSISOBSERVABLES_METHODS_HPP
