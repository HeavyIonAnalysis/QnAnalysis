#ifndef FLOW_SRC_BASE_CUTCONFIG_H_
#define FLOW_SRC_BASE_CUTCONFIG_H_

#include <TObject.h>

#include "AnalysisTree/Variable.hpp"

#include "Variable.h"

namespace Qn::Analysis::Base {

struct CutConfig : public TObject {
  enum ECutType {
    EQUAL,
    RANGE,
    NAMED_FUNCTION
  };

  VariableConfig variable;
  ECutType type{EQUAL};

  /* equal */
  double equal_val{0.};
  double equal_tol{0.};

  /* range */
  double range_lo{0.};
  double range_hi{0.};

  /* named function */
  std::string named_function_name;

  ClassDef(Qn::Analysis::Base::CutConfig, 2)
};

struct Cut {
  Cut() = default;
  Cut(AnalysisTree::Variable var, std::function<bool(double)> function, std::string description) : var_(std::move(var)),
                                                                                                   function_(std::move(function)),
                                                                                                   description_(std::move(description)) {}

  const AnalysisTree::Variable& GetVariable() const { return var_; }
  const std::function<bool(double)>& GetFunction() const { return function_; }
  const std::string& GetDescription() const { return description_; }

 private:
  AnalysisTree::Variable var_{};
  std::function<bool(double)> function_{};
  std::string description_{};
};

}// namespace Qn::Analysis::Base

#endif//FLOW_SRC_BASE_CUTCONFIG_H_
