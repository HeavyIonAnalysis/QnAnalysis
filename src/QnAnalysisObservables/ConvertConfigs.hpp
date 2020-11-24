//
// Created by mikhail on 11/23/20.
//

#ifndef QNANALYSIS_SRC_QNANALYSISOBSERVABLES_CONVERTCONFIGS_H_
#define QNANALYSIS_SRC_QNANALYSISOBSERVABLES_CONVERTCONFIGS_H_

#include "V1Observables.hpp"
#include <yaml-cpp/node/node.h>
namespace YAML {
template<>
struct convert<VectorConfig> {
  static bool decode(const Node& node, VectorConfig& vector_config) {
    if( !node.IsMap() ) {
      return false;
    }
    vector_config.name = node["name"].as<std::string>();
    vector_config.components = node["components"].as<std::vector<std::string>>();
    vector_config.correction_step = node["correction-step"].as<std::string>();
    vector_config.tag = node["tag"].as<std::string>("");
    return true;
  }
};

template<>
struct convert<VnObservablesConfig> {
  static bool decode(const Node& node, VnObservablesConfig& vn_observables_config) {
    auto method_type = node["method"].as<std::string>();
    if( method_type == "three-sub-event" )
      vn_observables_config.method = V1Observables::METHODS::MethodOf3SE;
    else if( method_type == "rnd-sub-event" )
      vn_observables_config.method = V1Observables::METHODS::MethodOfRS;
    else return false;
    vn_observables_config.qq_correlations_directory = node["QQ-directory"].as<std::string>();
    vn_observables_config.uq_correlations_directory = node["uQ-directory"].as<std::string>();
    vn_observables_config.u_vector = node["u-vector"].as<VectorConfig>();
    vn_observables_config.ep_vectors_tag = node["EP-vectors"].as<std::string>();
    vn_observables_config.resolution_vectors_tag = node["resolution-vectors"].as<std::string>();
    return true;
  }
};
}// namespace YAML
#endif//QNANALYSIS_SRC_QNANALYSISOBSERVABLES_CONVERTCONFIGS_H_
