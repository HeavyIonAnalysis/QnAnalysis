//
// Created by mikhail on 10/25/20.
//

#ifndef OBSERVABLESCALCULATOR_SRC_CALCULATOR_H_
#define OBSERVABLESCALCULATOR_SRC_CALCULATOR_H_

#include "Method.hpp"
#include "MethodOf3SE.hpp"
#include <utility>
#include <vector>
class V1Observables {
 public:
  enum class METHODS{
    MethodOf3SE
  };
  explicit V1Observables(METHODS method_type) : method_type_(method_type) {}
  virtual ~V1Observables() = default;
  void SetUvectors( std::string name, std::vector<std::string> components ){
    u_vector_name_ = std::move(name);
    u_vector_components_ = std::move(components);
  }
  void SetEPvectors( std::vector<std::string> names, std::vector<std::string> components ){
    ep_vectors_names_ = std::move(names);
    ep_vectors_components_ = std::move(components);
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
    std::vector<VectorConfig> resolution_qs;
    for( const auto& name : resolution_q_vectors_ ){
      resolution_qs.push_back({name});
    }
    for( const auto& component : u_vector_components_ )
      for( const auto& ep_vector : ep_vectors_names_ )
        for (const auto& ep_component : ep_vectors_components_) {
          methods_.emplace_back(new MethodOf3SE(
              {u_vector_name_, component}, {{ep_vector, ep_component}}, resolution_qs));
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
 std::vector<MethodOf3SE*> methods_;
 METHODS method_type_;
 std::string uq_correlations_directory_;
 std::string qq_correlations_directory_;
 std::string u_vector_name_;
 std::vector<std::string> u_vector_components_;
 std::vector<std::string> ep_vectors_names_;
 std::vector<std::string> ep_vectors_components_;
 std::vector<std::string> resolution_q_vectors_;
};

#endif // OBSERVABLESCALCULATOR_SRC_CALCULATOR_H_
