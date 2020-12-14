//
// Created by eugene on 10/12/2020.
//

#ifndef QNANALYSIS_SRC_QNANALYSISOBSERVABLES_METHODS_HPP
#define QNANALYSIS_SRC_QNANALYSISOBSERVABLES_METHODS_HPP

#include <QnTools/DataContainer.hpp>

namespace Methods {

using Resource = ResourceManager::Resource;
using ResourceMeta = ResourceManager::MetaType;

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
  meta.put("reference", ""); // TODO
  meta.put("component", ""); // TODO

  return ResourceManager::Resource(
      Qn::Sqrt(nom / denom.As<Qn::DataContainerStatCalculate>()),
      meta);
}

} /// namespace Methods


#endif //QNANALYSIS_SRC_QNANALYSISOBSERVABLES_METHODS_HPP
