//
// Created by mikhail on 10/24/20.
//

#include "MethodOf3SE.hpp"

Qn::DataContainer<Qn::StatCalculate> MethodOf3SE::CalculateResolution3SE(std::vector<Qn::DataContainerStatCalculate> correlations){
  auto result = Sqrt(correlations.at(0) * correlations.at(1) / correlations.at(2) ) * sqrt(2.0);
  return result;
}

void MethodOf3SE::CalculateObservables(){
  auto combinations = ConstructResolution3SECombinations(ep_vectors_configs_.front(), resolution_q_vectors_configs_);
  std::vector<Qn::DataContainer<Qn::StatCalculate>> results;
  for( auto res_combination_names : combinations ){
    std::vector<Qn::DataContainer<Qn::StatCalculate>> set_for_res_calc;
    set_for_res_calc.push_back( ReadContainerFromFile( qq_directory_, res_combination_names.at(0) ) );
    set_for_res_calc.push_back( ReadContainerFromFile( qq_directory_,  res_combination_names.at(1) ) );
    set_for_res_calc.push_back( ReadContainerFromFile(  qq_directory_, res_combination_names.at(2) ) );
    observables_names_.push_back( u_vector_config_.name+"."+ ep_vectors_configs_.front().name+"("+res_combination_names.at(2).front().name+","+res_combination_names.at(2).back().name+")" );
    resolutions_.push_back( CalculateResolution3SE( set_for_res_calc ) );
  }
  auto uq_correlation_confg = ep_vectors_configs_;
  uq_correlation_confg.insert(uq_correlation_confg.begin(), u_vector_config_);
  auto uq_correlation = ReadContainerFromFile( uq_directory_, uq_correlation_confg );
  for( const auto& res : resolutions_ ){
    observables_.push_back(uq_correlation/res*2.0);
  }
}

std::vector<std::vector<std::vector<VectorConfig>>> MethodOf3SE::ConstructResolution3SECombinations(
        VectorConfig reff_q, std::vector<VectorConfig> q_combination){
  auto q1 = std::move(reff_q);
  std::vector<std::vector<std::vector<VectorConfig>>> combinations;
  for (size_t i=0; i<q_combination.size(); ++i ){
    auto q2 = q_combination.at(i);
    if( q1.name == q2.name )
      continue;
    q2.component_name = q1.component_name;
    for( size_t j=i+1; j<q_combination.size(); ++j ){
      auto q3 = q_combination.at(j);
      if( q1.name == q3.name )
        continue;
      q3.component_name = q1.component_name;
      std::vector<std::vector<VectorConfig>> combination;
      combination.push_back({q1, q2});
      combination.push_back({q1, q3});
      combination.push_back({q2, q3});
      combinations.push_back(combination);
    }
  }
  return combinations;
}
