//
// Created by mikhail on 11/21/20.
//

#include "MethodOfRS.hpp"
MethodOfRS::MethodOfRS(const VectorComponentConfig& u_vector_config, const std::vector<VectorComponentConfig>& q_vector_config, const std::vector<VectorComponentConfig>& resolution_q_vectors_configs) : Method(u_vector_config, q_vector_config, resolution_q_vectors_configs) {}
MethodOfRS::~MethodOfRS() = default;
void MethodOfRS::CalculateObservables() {
  if( std::size(resolution_q_vectors_configs_) != 2 )
    throw std::runtime_error( "MethodOfRS::CalculateObservables(): "
                             "only 2 vectors must be in set for resolution correction calculation" );
  for( auto& res_vec : resolution_q_vectors_configs_ ){
    res_vec.component_name = ep_vectors_configs_.front().component_name;
  }
  auto qq_container = ReadContainerFromFile(qq_directory_, resolution_q_vectors_configs_);
  for( const auto& ep_vector : ep_vectors_configs_ ){
    auto uq_correlation = ReadContainerFromFile( uq_directory_, {u_vector_config_, ep_vector} );
    resolutions_.emplace_back(Sqrt( qq_container ));
    observables_.emplace_back( uq_correlation*sqrt(2.0) / resolutions_.back() );
    observables_names_.emplace_back( u_vector_config_.name+"."+ ep_vector.name+"("+resolution_q_vectors_configs_.at(0).name+","+resolution_q_vectors_configs_.at(1).name+")" );
  }
}
