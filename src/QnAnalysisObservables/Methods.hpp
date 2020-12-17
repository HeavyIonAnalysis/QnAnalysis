//
// Created by eugene on 10/12/2020.
//

#ifndef QNANALYSIS_SRC_QNANALYSISOBSERVABLES_METHODS_HPP
#define QNANALYSIS_SRC_QNANALYSISOBSERVABLES_METHODS_HPP

#include <QnTools/DataContainer.hpp>

namespace Methods {

using Resource = ResourceManager::Resource;
using ResourceMeta = ResourceManager::MetaType;


/*************** RESOLUTION *****************/

inline
Resource
Resolution3S(Resource nom1, Resource nom2, Resource denom) {
  auto nom = nom1.As<Qn::DataContainerStatCalculate>() * nom2.As<Qn::DataContainerStatCalculate>();
  nom = 2 * nom;

  /* populating meta information */
  auto meta = ResourceMeta();
  meta.put("type", "resolution");
  meta.put("method", "3sub");
  meta.put("source", __func__);

  auto result = Qn::Sqrt(nom / denom.As<Qn::DataContainerStatCalculate>());
  result.SetErrors(Qn::StatCalculate::ErrorType::BOOTSTRAP);

  return ResourceManager::Resource(
      result,
      meta);
}

/*************** v1 *****************/

inline
Resource
v1(Qn::DataContainerStatCalculate &uQ, Resource & resolution) {
  auto result = 2 * uQ / resolution.As<Qn::DataContainerStatCalculate>();
  result.SetErrors(Qn::StatCalculate::ErrorType::BOOTSTRAP);

  ResourceMeta meta;
  meta.put("type", "v1");
  meta.put("source", __func__);
  meta.put_child("resolution", resolution.meta);
  return Resource(result, meta);
}


} /// namespace Methods


#endif //QNANALYSIS_SRC_QNANALYSISOBSERVABLES_METHODS_HPP
