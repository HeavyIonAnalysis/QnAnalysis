//
// Created by eugene on 13/08/2020.
//

#ifndef FLOW_SRC_CONFIG_CONVERT_H
#define FLOW_SRC_CONFIG_CONVERT_H

#include <AnalysisSetup.h>
#include <AnalysisTree/Variable.hpp>
#include <Histogram.h>
#include <base/QVector.h>
#include <base/Variable.h>
#include <bitset>

#include <QnTools/CorrectionOnQnVector.hpp>

namespace Qn::Analysis::Config::Utils {

AnalysisTree::Variable Convert(const Base::VariableConfig& variable);

Base::Variable Convert1(const Base::VariableConfig& variable);

Qn::CorrectionOnQnVector* Convert(const Base::QVectorCorrectionConfig& config);

Base::QVector* Convert(const Base::QVectorConfig& config);

Base::Axis Convert(const Base::AxisConfig& axis_config);

Base::Cut Convert(const Base::CutConfig& config);

Base::AnalysisSetup Convert(const Base::AnalysisSetupConfig& config);

Base::Histogram Convert(const Base::HistogramConfig& histogram_config);

}// namespace Qn::Analysis::Config::Utils

#endif//FLOW_SRC_CONFIG_CONVERT_H
