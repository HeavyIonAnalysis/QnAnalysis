//
// Created by mikhail on 10/24/20.
//

#include "MethodOf3SE.hpp"

Qn::DataContainer<Qn::StatCalculate> MethodOf3SE::CalculateResolution(std::vector<Qn::DataContainerStatCalculate> correlations){
  auto result = Sqrt(correlations.at(0) * correlations.at(1) / correlations.at(2) ) * sqrt(2.0);
  return result;
}

void MethodOf3SE::CalculateObservables(){
  auto combinations = ConstructResolutionCombinations(q_vectors_configs_, resolution_q_vectors_configs_);
  std::vector<Qn::DataContainer<Qn::StatCalculate>> results;
  for( auto res_combination_names : combinations ){
    std::vector<Qn::DataContainer<Qn::StatCalculate>> set_for_res_calc;
    set_for_res_calc.push_back( ReadContainerFromFile( qq_directory_, res_combination_names.at(0) ) );
    set_for_res_calc.push_back( ReadContainerFromFile( qq_directory_,  res_combination_names.at(1) ) );
    set_for_res_calc.push_back( ReadContainerFromFile(  qq_directory_, res_combination_names.at(2) ) );
    observables_names_.push_back( u_vector_config_.title+"."+q_vectors_configs_.front().title+"("+res_combination_names.at(2).first.title+","+res_combination_names.at(2).second.title+")" );
    resolutions_.push_back( CalculateResolution( set_for_res_calc ) );
  }
  auto uq_correlation = ReadContainerFromFile( uq_directory_, std::pair(u_vector_config_, q_vectors_configs_.front()) );
  for( const auto& res : resolutions_ ){
    observables_.push_back(uq_correlation/res*2.0);
  }
}

std::vector<std::vector<std::pair<VectorConfig,VectorConfig>>> MethodOf3SE::ConstructResolutionCombinations(
        std::vector<VectorConfig> reff_q, std::vector<VectorConfig> q_combination){
  auto q1 = std::move(reff_q.front());
  std::vector<std::vector<std::pair<VectorConfig, VectorConfig>>> combinations;
  for (size_t i=0; i<q_combination.size(); ++i ){
    auto q2 = q_combination.at(i);
    q2.component_name = q1.component_name;
    for( size_t j=i+1; j<q_combination.size(); ++j ){
      auto q3 = q_combination.at(j);
      q3.component_name = q1.component_name;
      std::vector<std::pair<VectorConfig,VectorConfig>> combination;
      combination.emplace_back(q1, q2);
      combination.emplace_back(q1, q3);
      combination.emplace_back(q2, q3);
      combinations.push_back(combination);
    }
  }
  return combinations;
}
MethodOf3SE::MethodOf3SE(
    const VectorConfig &u_vector_config,
    const std::vector<VectorConfig> &q_vector_config,
    const std::vector<VectorConfig> &resolution_q_vectors_configs)
    : Method(u_vector_config, q_vector_config, resolution_q_vectors_configs) {}
