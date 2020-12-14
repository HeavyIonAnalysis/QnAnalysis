//
// Created by eugene on 10/12/2020.
//

#ifndef QNANALYSIS_SRC_QNANALYSISOBSERVABLES_METHODS_HPP
#define QNANALYSIS_SRC_QNANALYSISOBSERVABLES_METHODS_HPP

#include <QnTools/DataContainer.hpp>

namespace Methods {

inline
Qn::DataContainerStatCalculate
Resolution3S(ResourceManager::Resource nom1,
             ResourceManager::Resource nom2,
             ResourceManager::Resource denom) {
  auto nom = nom1.Ref<Qn::DataContainerStatCalculate>() * nom2.Ref<Qn::DataContainerStatCalculate>();
  nom = 2 * nom;
  return Qn::Sqrt(nom / denom.Ref<Qn::DataContainerStatCalculate>());
}

} /// namespace Methods


#endif //QNANALYSIS_SRC_QNANALYSISOBSERVABLES_METHODS_HPP
