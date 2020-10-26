//
// Created by mikhail on 10/25/20.
//

#ifndef OBSERVABLESCALCULATOR_SRC_METHOD_H_
#define OBSERVABLESCALCULATOR_SRC_METHOD_H_

#include <DataContainer.hpp>
#include <utility>

#include "FileManager.hpp"

struct VectorConfig {
  std::string name;
  std::string title;
  std::string component_name;
  std::vector<std::string> projection_axes;
  std::vector<Qn::AxisD> rebin_axes;
};

class Method {
public:
  virtual ~Method() = default;
  virtual void CalculateObservables() {}
  void SetUqDirectory(const std::string &uq_deirectory) {
    uq_directory_ = uq_deirectory;
  }
  void SetQqDirectory(const std::string &qq_directory) {
    qq_directory_ = qq_directory;
  }
  void Write(){
    int i=0;
    std::string component = u_vector_config_.component_name;
    for( auto config : q_vectors_configs_){
      component+=config.component_name;
    }
    for( auto  resolution: resolutions_){
      resolution.Write( std::data( "res."+ observables_names_.at(i)+"."+component ) );
      ++i;
    }
    i=0;
    for(auto flow : observables_){
      flow.Write( std::data( "obs."+ observables_names_.at(i)+"."+component ) );
      ++i;
    }
  }
protected:
  Method() = default;
  Method(VectorConfig u_vector_config,
         std::vector<VectorConfig> q_vector_config,
         std::vector<VectorConfig> resolution_q_vectors_configs)
      : u_vector_config_(std::move(u_vector_config)),
        q_vectors_configs_(std::move(q_vector_config)),
        resolution_q_vectors_configs_(std::move(resolution_q_vectors_configs)) {}

  virtual Qn::DataContainer<Qn::StatCalculate> CalculateResolution(std::vector<Qn::DataContainerStatCalculate> correlations){ };
  virtual std::vector<std::vector<std::pair<VectorConfig,VectorConfig>>> ConstructResolutionCombinations(std::vector<VectorConfig> reff_q, std::vector<VectorConfig> q_combination){}
  Qn::DataContainer<Qn::StatCalculate> ReadContainerFromFile( const std::string&, const std::pair<VectorConfig, VectorConfig>& vectors );

  std::string uq_directory_;
  std::string qq_directory_;
  VectorConfig u_vector_config_;
  std::vector<VectorConfig> q_vectors_configs_;
  std::vector<VectorConfig> resolution_q_vectors_configs_;

  std::vector<std::string> observables_names_;

  std::vector<Qn::DataContainer<Qn::StatCalculate>> resolutions_;
  std::vector<Qn::DataContainer<Qn::StatCalculate>> observables_;
};

#endif // OBSERVABLESCALCULATOR_SRC_METHOD_H_
