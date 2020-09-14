//
// Created by eugene on 13/08/2020.
//

#include "Convert.h"

#include <QnTools/CorrectionHistogramSparse.hpp>
#include <QnTools/CorrectionProfileComponents.hpp>
#include <QnTools/Recentering.hpp>
#include <QnTools/TwistAndRescale.hpp>

AnalysisTree::Variable Flow::Config::Utils::Convert(const Flow::Base::VariableConfig& variable) {
  return AnalysisTree::Variable(variable.branch, variable.field);
}

Flow::Base::Variable Flow::Config::Utils::Convert1(const Flow::Base::VariableConfig& variable) {
  Base::Variable result;
  result.config = variable;
  return result;
}

Flow::Base::QVector* Flow::Config::Utils::Convert(const Flow::Base::QVectorConfig& config) {
  using namespace Flow::Base;
  auto name = config.name;
  auto type = config.type;
  auto phi = Convert(config.phi);
  auto weight = Convert(config.weight);

  auto phi1 = Convert1(config.phi);
  phi1.RequestQnBinding(type == EQVectorType::CHANNEL ? name + "/" + phi1.config.branch + "/" + phi1.config.field : phi1.config.branch + "/" + phi1.config.field);
  phi1.RequestATBinding();

  auto weight1 = Convert1(config.weight);
  if (config.weight != VariableConfig::Ones()) {
    weight1.RequestQnBinding(type == EQVectorType::CHANNEL ? name + "/" + weight1.config.branch + "/" + weight1.config.field : weight1.config.branch + "/" + weight1.config.field);
    weight1.RequestATBinding();
  }

  if (type == EQVectorType::CHANNEL) {
    phi1.MapChannels(config.channel_ids);
    weight1.MapChannels(config.channel_ids);
  }

  auto harmonics = std::bitset<8>(config.harmonics);

  std::vector<Axis> axes(config.axes.size());
  std::transform(config.axes.begin(), config.axes.end(), axes.begin(), [](const AxisConfig& ax_conf) {
    return Convert(ax_conf);
  });
  std::vector<Cut> cuts(config.cuts.size());
  std::transform(config.cuts.begin(), config.cuts.end(), cuts.begin(), [](const CutConfig& cut_conf) {
    return Convert(cut_conf);
  });

  std::vector<Qn::CorrectionOnQnVector*> corrections(config.corrections.size());
  std::transform(config.corrections.begin(),
                 config.corrections.end(),
                 corrections.begin(),
                 [](const QVectorCorrectionConfig& config) {
                   return Convert(config);
                 });

  auto channel_ids = config.channel_ids;

  if (type == EQVectorType::TRACK) {
    auto result = new QVectorTrack(name, phi, weight, axes);
    for (auto correction_ptr : corrections) {
      result->AddCorrection(correction_ptr);
    }
    for (auto& cut : cuts) {
      result->AddCut(cut);
    }
    for (auto& qa_histogram : config.qa) {
      result->AddQAHistogram(Convert(qa_histogram));
    }
    result->SetHarmonics(harmonics);

    return result;
  } else if (type == EQVectorType::CHANNEL) {
    auto result = new QVectorChannel(name, phi, weight, channel_ids);
    for (auto correction_ptr : corrections) {
      result->AddCorrection(correction_ptr);
    }
    for (auto& qa_histogram : config.qa) {
      result->AddQAHistogram(Convert(qa_histogram));
    }
    result->SetHarmonics(harmonics);
    return result;
  } else if (type == EQVectorType::EVENT_PSI) {
    auto result = new QVectorPsi(name, phi, weight);
    for (auto& qa_histogram : config.qa) {
      result->AddQAHistogram(Convert(qa_histogram));
    }
    return result;
  }

  throw std::runtime_error("Invalid QVector type");
}

Flow::Base::Axis Flow::Config::Utils::Convert(const Flow::Base::AxisConfig& axis_config) {
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

Flow::Base::Cut Flow::Config::Utils::Convert(const Flow::Base::CutConfig& config) {
  auto var = Convert(config.variable);
  std::function<bool(const double&)> function;
  std::string description;

  if (config.type == Base::CutConfig::EQUAL) {
    auto equal_val = config.equal_val;
    auto equal_tol = config.equal_tol;
    function = [equal_val, equal_tol](const double& v) -> bool {
      return std::abs(v - equal_val) <= equal_tol;
    };
    description = var.GetName() + " == " + std::__cxx11::to_string(equal_val);
  } else if (config.type == Base::CutConfig::RANGE) {
    auto range_lo = config.range_lo;
    auto range_hi = config.range_hi;
    function = [range_lo, range_hi](const double& v) -> bool {
      return range_lo <= v && v <= range_hi;
    };
    description =
        var.GetName() + " in [" + std::__cxx11::to_string(range_lo) + ";" + std::__cxx11::to_string(range_hi) + "]";
    throw std::runtime_error("Not yet implemented");
  }
  return Base::Cut(var, function, description);
}

Flow::Base::AnalysisSetup Flow::Config::Utils::Convert(const Flow::Base::AnalysisSetupConfig& config) {
  Base::AnalysisSetup setup;

  for (auto& config_variable : config.event_variables) {
    setup.AddEventVar(Convert(config_variable));
  }

  for (auto& config_axis : config.event_axes) {
    auto axis = Convert(config_axis);
    setup.AddCorrectionAxis(axis.GetQnAxis());
  }

  for (auto& config_q_vector : config.q_vectors) {
    setup.AddQVector(Convert(config_q_vector));
  }

  return setup;
}

Flow::Base::Histogram Flow::Config::Utils::Convert(const Flow::Base::HistogramConfig& histogram_config) {
  Base::Histogram result;
  result.axes.resize(histogram_config.axes.size());
  std::transform(histogram_config.axes.begin(), histogram_config.axes.end(), result.axes.begin(),
                 [](const Base::AxisConfig& ax_config) { return Convert(ax_config); });
  return result;
}
Qn::CorrectionOnQnVector* Flow::Config::Utils::Convert(const Flow::Base::QVectorCorrectionConfig& config) {
  using Flow::Base::ETwistRescaleMethod;
  if (config.type == Flow::Base::EQVectorCorrectionType::RECENTERING) {
    auto correction = new Qn::Recentering;
    correction->SetApplyWidthEqualization(config.recentering_width_equalization);
    correction->SetNoOfEntriesThreshold(config.no_of_entries);
    return correction;
  } else if (config.type == Flow::Base::EQVectorCorrectionType::TWIST_AND_RESCALE) {
    auto correction = new Qn::TwistAndRescale;
    correction->SetApplyTwist(config.twist_rescale_apply_twist);
    correction->SetApplyRescale(config.twist_rescale_apply_rescale);
    auto method = (config.twist_rescale_method == ETwistRescaleMethod::CORRELATIONS) ? Qn::TwistAndRescale::Method::CORRELATIONS : Qn::TwistAndRescale::Method::DOUBLE_HARMONIC;
    correction->SetTwistAndRescaleMethod(method);
    correction->SetNoOfEntriesThreshold(config.no_of_entries);
    return correction;
  }

  throw std::runtime_error("Invalid correction type");
}
