//
// Created by eugene on 04/05/2021.
//

#include "QnSystematicError.hpp"
void Qn::DataContainerSystematicError::AddSystematicVariation(const std::string &name,
                                                              const Qn::DataContainerStatCalculate &other) {
  auto source_id_it = systematic_sources_ids.find(name);
  if (source_id_it == systematic_sources_ids.end()) {
    AddSystematicSource(name);
    source_id_it = systematic_sources_ids.find(name);
  }
  auto source_id = source_id_it->second;

  /* consistency check between this and other */
  assert(this->size() == other.size());
  // TODO other bin consistency checks

  using size_type = decltype(this->size());
  for (size_type ibin = 0; ibin < this->size(); ++ibin) {
    this->operator[](ibin).AddVariation(source_id, other.operator[](ibin));
  }
}

void Qn::DataContainerSystematicError::AddSystematicSource(const std::string &name) {
  int source_id = systematic_sources_ids.size();
  for (auto &bin : *this) {
    bin.AddSystematicSource(source_id);
  }
  systematic_sources_ids.emplace(name, source_id);
}

std::vector<std::string> Qn::DataContainerSystematicError::GetSystematicSources() const {
  std::vector<std::string> result;
  std::transform(
      ::std::begin(systematic_sources_ids),
      ::std::end(systematic_sources_ids),
      ::std::back_inserter(result),
      [] (const auto& entry) { return entry.first; });
  return result;
}

int Qn::DataContainerSystematicError::GetSystematicSourceId(const std::string &name) const {
  return systematic_sources_ids.at(name);
}

GraphSysErr *Qn::ToGSE(const Qn::DataContainerSystematicError &data) {
  if (data.GetAxes().size() > 1) {
    std::cout << "Data container has more than one dimension. " << std::endl;
    std::cout
        << "Cannot draw as Graph. Use Projection() to make it one dimensional."
        << std::endl;
    return nullptr;
  }

  auto graph = new GraphSysErr;

  std::map<int, int> systematic_id_map;
  for (auto &systematic_source_name : data.GetSystematicSources()) {
    auto data_error_id = data.GetSystematicSourceId(systematic_source_name);
    auto graph_error_id = graph->DeclarePoint2Point(systematic_source_name.c_str(), false);
    systematic_id_map.emplace(data_error_id, graph_error_id);
  }

  unsigned int ibin = 0;
  for (const auto &bin : data) {
    if (bin.SumWeights() <= 0) {
      ibin++;
      continue;
    }
    auto y = bin.Mean();
    auto ey_stat = bin.GetStatisticalErrorOfMean();
    auto xhi = data.GetAxes().front().GetUpperBinEdge(ibin);
    auto xlo = data.GetAxes().front().GetLowerBinEdge(ibin);
    auto xhalfwidth = (xhi - xlo)/2.;
    auto x = xlo + xhalfwidth;

    auto graph_point_id = graph->GetN();
    graph->SetPoint(graph_point_id, x, y);
    graph->SetStatError(graph_point_id, ey_stat);

    for (auto &stat_source_element : systematic_id_map) {
      const auto data_error_id = stat_source_element.first;
      const auto graph_error_id = stat_source_element.second;
      graph->SetSysError(graph_error_id, graph_point_id, 0., bin.GetSystematicalError(data_error_id));
    }

    ibin++;
  }
  return graph;
}
