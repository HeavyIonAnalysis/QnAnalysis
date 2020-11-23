//
// Created by mikhail on 10/25/20.
//

#ifndef OBSERVABLESCALCULATOR_SRC_CALCULATOR_H_
#define OBSERVABLESCALCULATOR_SRC_CALCULATOR_H_

#include <utility>
#include <vector>

#include "Method.hpp"
#include "MethodOf3SE.hpp"
#include "MethodOfRS.hpp"

struct VectorConfig{
  std::string name;
  std::string correction_step;
  std::vector<std::string> components;
  std::string tag;
};

class V1Observables {
 public:
  enum class METHODS{
    MethodOf3SE,
    MethodOfRS
  };
  explicit V1Observables(METHODS method_type) : method_type_(method_type) {}
  virtual ~V1Observables() = default;
  void SetUvector( std::string name, std::vector<std::string> components ){
    u_vector_name_ = std::move(name);
    u_vector_components_ = std::move(components);
  }
  void SetUvector( const VectorConfig& u_vector ){
      u_vector_name_ = u_vector.name+"_"+u_vector.correction_step;
      u_vector_components_ = u_vector.components;
  };
  void SetEPvectors( std::vector<std::string> names, std::vector<std::string> components ){
    ep_vectors_names_ = std::move(names);
    ep_vectors_components_ = std::move(components);
  }
  void SetEPvectors( const std::vector<VectorConfig>& vectors){
    for( const auto& vector : vectors ) {
      ep_vectors_names_.emplace_back(vector.name + "_" + vector.correction_step);
    }
    ep_vectors_components_ = vectors.front().components;
  }
  void SetResolutionVectors( std::vector<std::string> names){
    resolution_q_vectors_ = std::move(names);
  }
  void SetUqCorrelationsDirectory(const std::string& uq_correlations_directory) {
    uq_correlations_directory_ = uq_correlations_directory;
  }
  void SetQqCorrelationsDirectory(const std::string& qq_correlations_directory) {
    qq_correlations_directory_ = qq_correlations_directory;
  }
  void Calculate(){
    std::vector<VectorComponentConfig> resolution_qs;
    for( const auto& name : resolution_q_vectors_ ){
      resolution_qs.push_back({name});
    }
    for( const auto& component : u_vector_components_ )
      for( const auto& ep_vector : ep_vectors_names_ )
        for (const auto& ep_component : ep_vectors_components_) {
          switch (method_type_) {
            case METHODS::MethodOf3SE:
              methods_.emplace_back(new MethodOf3SE(
                  {u_vector_name_, component}, {{ep_vector, ep_component}}, resolution_qs));
              break;
            case METHODS::MethodOfRS:
              methods_.emplace_back(new MethodOfRS(
                  {u_vector_name_, component}, {{ep_vector, ep_component}}, resolution_qs));
              break;
          }
          methods_.back()->SetQqDirectory(qq_correlations_directory_);
          methods_.back()->SetUqDirectory(uq_correlations_directory_);
        }
    for( auto method : methods_ )
      method->CalculateObservables();
  }
  void Write(){
    for( auto method : methods_ )
      method->Write();
  }

 private:
 std::vector<Method*> methods_;
 METHODS method_type_;
 std::string uq_correlations_directory_;
 std::string qq_correlations_directory_;
 std::string u_vector_name_;
 std::vector<std::string> u_vector_components_;
 std::vector<std::string> ep_vectors_names_;
 std::vector<std::string> ep_vectors_components_;
 std::vector<std::string> resolution_q_vectors_;
};

struct VnObservablesConfig {
  V1Observables::METHODS method;
  std::string uq_correlations_directory;
  std::string qq_correlations_directory;
  VectorConfig u_vector;
  std::string ep_vectors_tag;
  std::string resolution_vectors_tag;
};

#endif // OBSERVABLESCALCULATOR_SRC_CALCULATOR_H_
