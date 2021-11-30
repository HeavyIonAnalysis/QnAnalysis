#ifndef FLOW_SRC_BASE_CUTCONFIG_H_
#define FLOW_SRC_BASE_CUTCONFIG_H_

#include <TObject.h>
#include <list>

#include "AnalysisTree/Variable.hpp"

#include <QnAnalysisBase/Variable.hpp>

namespace Qn::Analysis::Base {

struct CutConfig : public TObject {
  enum ECutType {
    EQUAL,
    ANY_OF,
    RANGE,
    EXPR,
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

  /* any-of */
  std::vector<double> any_of_values;
  double any_of_tolerance{0.};

  /* expr */
  std::string expr_string;
  std::vector<double> expr_parameters;


  /* named function */
  std::string named_function_name;

  ClassDef(Qn::Analysis::Base::CutConfig, 2)
};

struct CutListConfig {
  std::list<CutConfig> cuts;
};

struct Cut {
  typedef std::list<AnalysisTree::Variable> VariableListType;
  typedef const std::vector<double>& FunctionArgType;
  typedef std::function<bool (FunctionArgType)> FunctionType;

  Cut() = default;
  Cut(VariableListType var, FunctionType function, std::string description) : variables_list_(std::move(var)),
                                                                                    function_(std::move(function)),
                                                                                    description_(std::move(description)) {}

  VariableListType GetListOfVariables() const { return variables_list_; }
  FunctionType GetFunction() const { return function_; }
  std::string GetDescription() const { return description_; }

 private:
  VariableListType variables_list_{};
  FunctionType function_{};
  std::string description_{};
};

}// namespace Qn::Analysis::Base

#endif//FLOW_SRC_BASE_CUTCONFIG_H_
