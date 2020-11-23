//
// Created by mikhail on 11/21/20.
//

#ifndef QNANALYSIS_SRC_QNANALYSISOBSERVABLES_METHOD_OF_RS_H_
#define QNANALYSIS_SRC_QNANALYSISOBSERVABLES_METHOD_OF_RS_H_

#include "Method.hpp"

class MethodOfRS : public Method {
 public:
  MethodOfRS(const VectorConfig& u_vector_config, const std::vector<VectorConfig>& q_vector_config, const std::vector<VectorConfig>& resolution_q_vectors_configs);
  ~MethodOfRS() override;
  void CalculateObservables() override;
};

#endif//QNANALYSIS_SRC_QNANALYSISOBSERVABLES_METHOD_OF_RS_H_
