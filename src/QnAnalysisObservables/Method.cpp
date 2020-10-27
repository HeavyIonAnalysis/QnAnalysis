//
// Created by mikhail on 10/25/20.
//

#include "Method.hpp"

Qn::DataContainer<Qn::StatCalculate> Method::ReadContainerFromFile( const std::string& directory, const std::vector<VectorConfig>& vectors ){
  Qn::DataContainerStatCollect*  obj;
  std::string name;
  std::string component;
  try{
    name = directory+"/";
    component.clear();
    for (const auto& vector : vectors) {
      name += vector.name+".";
      component += vector.component_name;
    }
    name+=component;
    obj = FileManager::GetObject<Qn::DataContainerStatCollect>(name);
    auto result = Qn::DataContainerStatCalculate(*obj);
    result.SetErrors(Qn::StatCalculate::ErrorType::BOOTSTRAP);
    return result;
  } catch (std::exception&) {}
  auto combinations = GetAllCombinations(vectors);
  int n=0;
  while ( n<combinations.size() ) {
    try {
      name = directory+"/";
      component.clear();
      for (const auto& vector : combinations.at(n)) {
        name += vector.name+".";
        component += vector.component_name;
      }
      name+=component;
      obj = FileManager::GetObject<Qn::DataContainerStatCollect>(name);
      auto result = Qn::DataContainerStatCalculate(*obj);
      result.SetErrors(Qn::StatCalculate::ErrorType::BOOTSTRAP);
      return result;
    } catch (std::runtime_error&) {
      n++;
    }
  }
  throw std::runtime_error( "Method::ReadContainerFromFile: No such a correlation in file "+name );
}

std::vector<std::vector<VectorConfig>> Method::GetAllCombinations( std::vector<VectorConfig> elements ){
  std::vector<std::vector<VectorConfig>> result;
  if( elements.size() == 1 ){
    result = {elements};
    return result;
  }
  auto el1 = elements.front();
  std::vector<std::vector<VectorConfig>> prev_combinations;
  elements.erase(elements.begin());
  prev_combinations=GetAllCombinations(elements);
  for( size_t i=0; i<std::size(prev_combinations); ++i ){
    for( size_t j=0; j<std::size(prev_combinations.front())+1; ++j ){
      result.emplace_back();
      for( size_t k=0; k<std::size(prev_combinations.front())+1;++k ){
        if( k==j )
          result.back().emplace_back(el1);
        if( k<j )
          result.back().emplace_back(prev_combinations.at(i).at(k));
        if ( k>j )
          result.back().emplace_back(prev_combinations.at(i).at(k-1));
      }
    }
  }
  return result;
}
