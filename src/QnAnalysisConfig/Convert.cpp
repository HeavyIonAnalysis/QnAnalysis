//
// Created by eugene on 13/08/2020.
//

#include <string>

#include <QnTools/CorrectionHistogramSparse.hpp>

#include <QnTools/CorrectionProfileComponents.hpp>
#include <QnTools/Recentering.hpp>
#include <QnTools/TwistAndRescale.hpp>
#include <QnTools/Alignment.hpp>
#include <TFormula.h>

#include "Convert.hpp"

ATVariable Qn::Analysis::Config::Utils::Convert(const Qn::Analysis::Base::VariableConfig& variable) {
  return {variable.branch, variable.field};
}

Qn::Analysis::Base::Variable Qn::Analysis::Config::Utils::Convert1(const Qn::Analysis::Base::VariableConfig &variable) {
  Base::Variable result;
  result.config = variable;
  return result;
}

Qn::Analysis::Base::QVector *Qn::Analysis::Config::Utils::Convert(const Qn::Analysis::Base::QVectorConfig &config) {
  using namespace Qn::Analysis::Base;
  auto name = config.name;
  auto type = config.type;
  auto phi = Convert(config.phi);
  auto weight = Convert(config.weight);

  auto phi1 = Convert1(config.phi);
  phi1.RequestQnBinding(
      type == EQVectorType::CHANNEL ? name + "/" + phi1.config.branch + "/" + phi1.config.field : phi1.config.branch
          + "/" + phi1.config.field);
  phi1.RequestATBinding();

  auto weight1 = Convert1(config.weight);
  if (config.weight != VariableConfig::Ones()) {
    weight1.RequestQnBinding(
        type == EQVectorType::CHANNEL ? name + "/" + weight1.config.branch + "/" + weight1.config.field :
        weight1.config.branch + "/" + weight1.config.field);
    weight1.RequestATBinding();
  }

  if (type == EQVectorType::CHANNEL) {
    phi1.MapChannels(config.channel_ids);
    weight1.MapChannels(config.channel_ids);
  }

  auto harmonics = std::bitset<8>(config.harmonics);

  std::vector<Axis> axes(config.axes.size());
  std::transform(config.axes.begin(), config.axes.end(), axes.begin(), [](const AxisConfig &ax_conf) {
    return Convert(ax_conf);
  });
  std::vector<Cut> cuts(config.cuts.size());
  std::transform(config.cuts.begin(), config.cuts.end(), cuts.begin(), [](const CutConfig &cut_conf) {
    return Convert(cut_conf);
  });

  std::vector<Qn::CorrectionOnQnVector *> corrections(config.corrections.size());
  std::transform(config.corrections.begin(),
                 config.corrections.end(),
                 corrections.begin(),
                 [](const QVectorCorrectionConfig &config) {
                   return Convert(config);
                 });

  auto channel_ids = config.channel_ids;

  if (type == EQVectorType::TRACK) {
    auto result = new QVectorTrack(name, phi, weight, axes);
    for (auto correction_ptr : corrections) {
      result->AddCorrection(correction_ptr);
    }
    for (auto &cut : cuts) {
      result->AddCut(cut);
    }
    for (auto &qa_histogram : config.qa) {
      result->AddQAHistogram(Convert(qa_histogram));
    }
    result->SetHarmonics(harmonics);
    result->SetNormalization(config.normalization);
    return result;
  } else if (type == EQVectorType::CHANNEL) {
    auto result = new QVectorChannel(name, phi, weight, channel_ids);
    for (auto correction_ptr : corrections) {
      result->AddCorrection(correction_ptr);
    }
    for (auto &qa_histogram : config.qa) {
      result->AddQAHistogram(Convert(qa_histogram));
    }
    result->SetHarmonics(harmonics);
    result->SetNormalization(config.normalization);
    return result;
  } else if (type == EQVectorType::EVENT_PSI) {
    auto result = new QVectorPsi(name, phi, weight);
    for (auto &qa_histogram : config.qa) {
      result->AddQAHistogram(Convert(qa_histogram));
    }
    result->SetNormalization(config.normalization);
    result->SetHarmonics(harmonics);
    return result;
  }

  throw std::runtime_error("Invalid QVector type");
}

Qn::Analysis::Base::Axis Qn::Analysis::Config::Utils::Convert(const Qn::Analysis::Base::AxisConfig &axis_config) {
  Base::Axis result;
  result.var_ = Convert(axis_config.variable);

  if (axis_config.type == Base::AxisConfig::RANGE) {
    result.axis_ = Qn::AxisD(result.var_.GetName(), axis_config.nb, axis_config.lo, axis_config.hi);
  } else if (axis_config.type == Base::AxisConfig::BIN_EDGES) {
    result.axis_ = Qn::AxisD(result.var_.GetName(), axis_config.bin_edges);
  } else {
  }
  return result;
}

Qn::Analysis::Base::Cut Qn::Analysis::Config::Utils::Convert(const Qn::Analysis::Base::CutConfig &config) {
  using Qn::Analysis::Base::Cut;
  auto var = Convert(config.variable);

  Cut::FunctionType function;

  std::stringstream description_stream;
  if (config.type == Base::CutConfig::EQUAL) {
    auto equal_val = config.equal_val;
    auto equal_tol = config.equal_tol;
    function = [equal_val, equal_tol](Cut::FunctionArgType v) -> bool {
      return std::abs(v[0] - equal_val) <= equal_tol;
    };
    description_stream << var.GetName() << " == " << equal_val;
    return {{var}, function, description_stream.str()};
  } else if (config.type == Base::CutConfig::RANGE) {
    auto range_lo = config.range_lo;
    auto range_hi = config.range_hi;
    function = [range_lo, range_hi](Cut::FunctionArgType v) -> bool {
      return range_lo <= v[0] && v[0] <= range_hi;
    };
    description_stream << var.GetName() << " in [" << range_lo << "; " << range_hi << "]";
    return {{var}, function, description_stream.str()};
  } else if (config.type == Base::CutConfig::ANY_OF) {
    auto allowed_values = config.any_of_values; // maybe std::set here is better
    auto tolerance = config.any_of_tolerance;
    sort(begin(allowed_values), end(allowed_values));
    function = [allowed_values, tolerance](Cut::FunctionArgType value) -> bool {
      if (value[0] - tolerance > allowed_values.back()) {
        return false;
      }
      if (value[0] + tolerance < allowed_values.front()) {
        return false;
      }
      for (auto allowed_value : allowed_values) {
        if (tolerance == 0) {
          if (allowed_value == value[0])
            return true;
        } else {
          if (std::abs(allowed_value - value[0]) < tolerance)
            return true;
        }
      }
      return false;
    };

    description_stream << var.GetName() << " any of [" << allowed_values.front();
    if (allowed_values.size() > 1) {
      for (auto it = allowed_values.begin() + 1; it != allowed_values.end(); ++it) {
        description_stream << ", " << *it;
      }
    }
    description_stream << "]";
    return {{var}, function, description_stream.str()};
  } else if (config.type == Base::CutConfig::EXPR) {

    std::list<std::string> variables_list;

    auto expression_string = config.expr_string;
    auto expression_string_parsed = expression_string;
    std::regex re_var(R"((\{\{[\w_]+/[\w_]+\}\}))");
    std::smatch match_results;
    std::string::difference_type replacement_offset{0};
    auto expr_string_it = cbegin(expression_string);
    while (std::regex_search(expr_string_it, cend(expression_string), match_results, re_var)) {
      std::string var_match = match_results.str(1);
      auto var_name = var_match.substr(2, var_match.length()-4);
      auto var_name_it = find(cbegin(variables_list), cend(variables_list), var_name);
      if (var_name_it == cend(variables_list)) {
        variables_list.emplace_back(var_name);
      }
      auto var_id = var_name_it == cend(variables_list) ? variables_list.size() - 1 : distance(cbegin(variables_list), var_name_it);

      std::string var_replacement("x[");
      var_replacement.append(std::to_string(var_id)).append("]");
      auto replacement_start = distance(cbegin(expression_string), expr_string_it) + match_results.position(1) + replacement_offset;
      expression_string_parsed.replace(replacement_start, var_match.length(), var_replacement);
      expr_string_it += var_match.length();
      replacement_offset += var_replacement.length() - var_match.length();

      if (expr_string_it == cend(expression_string)) {
        break;
      }
    }

    TFormula expression_formula("", expression_string_parsed.c_str(), false, true);
    if (!expression_formula.IsValid()) {
      throw std::runtime_error("Expression '" + expression_string_parsed + "' is not valid");
    }
    if (expression_formula.GetNpar() != config.expr_parameters.size()) {
      throw std::runtime_error("Number of expression parameters different from number of provided parameters");
    }
    if (expression_formula.GetNpar() > 0) {
      expression_formula.SetParameters(config.expr_parameters.data());
    }
    auto formula_function = [expression_formula] (Cut::FunctionArgType arg_type) -> bool {
      return bool(expression_formula.EvalPar(arg_type.data()));
    };

    Cut::VariableListType variable_list;
    for (auto && variable_name : variables_list) {
      std::regex re_variable("([\\w_]+)/([\\w_]+)");
      std::smatch match_results;
      std::regex_search(variable_name, match_results, re_variable);
      auto branch_name = match_results.str(1);
      auto field_name = match_results.str(2);
      variable_list.emplace_back(ATVariable(branch_name, field_name));
    }

    return {variable_list, formula_function, expression_string};
  }

  throw std::runtime_error("Unsupported type of Cut");
}

Qn::Analysis::Base::AnalysisSetup Qn::Analysis::Config::Utils::Convert(const Qn::Analysis::Base::AnalysisSetupConfig &config) {
  Base::AnalysisSetup setup;

  for (auto &config_variable : config.event_variables) {
    setup.AddEventVar(Convert(config_variable));
  }

  for (auto &config_axis : config.event_axes) {
    auto axis = Convert(config_axis);
    setup.AddCorrectionAxis(axis.GetQnAxis());
  }

  for (auto &config_q_vector : config.q_vectors) {
    setup.AddQVector(Convert(config_q_vector));
  }

  for (auto &qa_config : config.qa) {
    setup.qa_.push_back(Convert(qa_config));
  }

  return setup;
}

Qn::Analysis::Base::Histogram Qn::Analysis::Config::Utils::Convert(const Qn::Analysis::Base::HistogramConfig &histogram_config) {
  Base::Histogram result;
  result.axes.resize(histogram_config.axes.size());
  std::transform(histogram_config.axes.begin(), histogram_config.axes.end(), result.axes.begin(),
                 [](const Base::AxisConfig &ax_config) { return Convert(ax_config); });
  result.weight = histogram_config.weight;
  return result;
}
Qn::CorrectionOnQnVector *Qn::Analysis::Config::Utils::Convert(const Qn::Analysis::Base::QVectorCorrectionConfig &config) {
  using Qn::Analysis::Base::ETwistRescaleMethod;
  if (config.type == Qn::Analysis::Base::EQVectorCorrectionType::RECENTERING) {
    auto correction = new Qn::Recentering;
    correction->SetApplyWidthEqualization(config.recentering_width_equalization);
    correction->SetNoOfEntriesThreshold(config.no_of_entries);
    return correction;
  } else if (config.type == Qn::Analysis::Base::EQVectorCorrectionType::TWIST_AND_RESCALE) {
    auto correction = new Qn::TwistAndRescale;
    correction->SetApplyTwist(config.twist_rescale_apply_twist);
    correction->SetApplyRescale(config.twist_rescale_apply_rescale);
    auto method =
        (config.twist_rescale_method == ETwistRescaleMethod::CORRELATIONS) ? Qn::TwistAndRescale::Method::CORRELATIONS
                                                                           : Qn::TwistAndRescale::Method::DOUBLE_HARMONIC;
    correction->SetTwistAndRescaleMethod(method);
    correction->SetNoOfEntriesThreshold(config.no_of_entries);
    return correction;
  } else if (config.type == Qn::Analysis::Base::EQVectorCorrectionType::ALIGNMENT) {
    auto correction = new Qn::Alignment;
    correction->SetHarmonicNumberForAlignment(config.alignment_harmonic);
    return correction;
  }

  throw std::runtime_error("Invalid correction type");
}
