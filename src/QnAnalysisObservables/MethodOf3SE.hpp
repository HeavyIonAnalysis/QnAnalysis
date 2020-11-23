//
// Created by mikhail on 10/24/20.
//

#ifndef OBSERVABLESCALCULATOR_SRC_METHOD_OF_3SE_H_
#define OBSERVABLESCALCULATOR_SRC_METHOD_OF_3SE_H_

#include <utility>

#include <DataContainer.hpp>
#include <StatCalculate.hpp>

#include "FileManager.hpp"
#include "Method.hpp"

class MethodOf3SE : public Method {
public:
  MethodOf3SE(const VectorComponentConfig&u_vector_config,
              const std::vector<VectorComponentConfig> &q_vector_config,
              const std::vector<VectorComponentConfig> &resolution_q_vectors_configs)
      : Method(u_vector_config, q_vector_config, resolution_q_vectors_configs) {}
  ~MethodOf3SE() override = default;
  void CalculateObservables() override;

protected:
  static Qn::DataContainer<Qn::StatCalculate> CalculateResolution3SE(std::vector<Qn::DataContainerStatCalculate> correlations);
  static std::vector<std::vector<std::vector<VectorComponentConfig>>> ConstructResolution3SECombinations(VectorComponentConfig reff_q, std::vector<VectorComponentConfig> q_combination);
};

#endif // OBSERVABLESCALCULATOR_SRC_METHOD_OF_3SE_H_
