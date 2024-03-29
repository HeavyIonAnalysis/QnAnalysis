//
// Created by eugene on 12/08/2020.
//

#ifndef FLOW_SRC_BASE_VARIABLE_H
#define FLOW_SRC_BASE_VARIABLE_H

#include <TObject.h>

#include <memory>
#include <string>
#include <utility>

#include "AnalysisTree.hpp"

namespace Qn::Analysis::Base {

struct VariableConfig : public TObject {
  std::string branch;
  std::string field;

  bool operator==(const VariableConfig& Rhs) const {
    return branch == Rhs.branch && field == Rhs.field;
  }
  bool operator!=(const VariableConfig& Rhs) const {
    return !(Rhs == *this);
  }

  static VariableConfig Ones() {
    VariableConfig result;
    result.branch = "";
    result.field = "Ones";
    return result;
  }

  ClassDef(Qn::Analysis::Base::VariableConfig, 2)
};

struct VariableQnBinding {
  bool is_bound{false};
  std::string name;
  size_t size{0};
  size_t id{0};
};

struct Variable {
  enum EMappingType {
    PLAIN,             /* mapping as-is, channel by channel (if many) */
    SELECTED_CHANNELS, /* map only selected channels */
  };
  VariableConfig config;

  EMappingType mapping_type{PLAIN};
  std::vector<int> channel_ids;

  std::shared_ptr<VariableQnBinding> qn_binding;
  std::shared_ptr<ATVariable> at_binding;

  void RequestQnBinding(const std::string& qn_name) {
    if (config == VariableConfig::Ones()) {
      throw std::logic_error("Service variable 'Ones' cannot be bound to the Qn variable");
    }
    if (qn_binding) {
      throw std::logic_error("Qn binding already exists");
    }

    auto result = new VariableQnBinding;
    result->name = qn_name;
    qn_binding.reset(result);
  }

  void RequestATBinding() {
    if (config == VariableConfig::Ones()) {
      throw std::logic_error("Service variable 'Ones' cannot be bound to the AT variable");
    }
    if (at_binding) {
      throw std::logic_error("AT binding already exists");
    }
    at_binding = std::make_shared<ATVariable>(config.branch, config.field);
  }

  void MapPlain() { mapping_type = PLAIN; }
  void MapChannels(std::vector<int> channels) {
    mapping_type = SELECTED_CHANNELS;
    channel_ids = std::move(channels);
  }
};

}// namespace Qn::Analysis::Base

#endif//FLOW_SRC_BASE_VARIABLE_H
