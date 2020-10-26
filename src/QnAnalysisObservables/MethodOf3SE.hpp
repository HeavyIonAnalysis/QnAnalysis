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
  MethodOf3SE(const VectorConfig &u_vector_config,
              const std::vector<VectorConfig> &q_vector_config,
              const std::vector<VectorConfig> &resolution_q_vectors_configs);
  ~MethodOf3SE() override = default;
  void CalculateObservables() override;

protected:
  Qn::DataContainer<Qn::StatCalculate> CalculateResolution(std::vector<Qn::DataContainerStatCalculate> correlations) override;
  std::vector<std::vector<std::pair<VectorConfig,VectorConfig>>> ConstructResolutionCombinations(std::vector<VectorConfig> reff_q, std::vector<VectorConfig> q_combination) override;
};

#endif // OBSERVABLESCALCULATOR_SRC_METHOD_OF_3SE_H_
