//
// Created by eugene on 13/08/2020.
//

#ifndef FLOW_SRC_CONFIG_CONVERT_H
#define FLOW_SRC_CONFIG_CONVERT_H

#include <bitset>
#include <base/Variable.h>
#include <base/QVector.h>
#include <AnalysisTree/Variable.hpp>
#include <AnalysisSetup.h>
#include <Histogram.h>

#include <QnTools/CorrectionOnQnVector.hpp>

namespace Flow::Config::Utils {

AnalysisTree::Variable Convert(const Base::VariableConfig &variable);

Base::Variable Convert1(const Base::VariableConfig &variable);

Qn::CorrectionOnQnVector *Convert(const Base::QVectorCorrectionConfig &config);

Base::QVector *Convert(const Base::QVectorConfig &config);

Base::Axis Convert(const Base::AxisConfig &axis_config);

Base::Cut Convert(const Base::CutConfig &config);

Base::AnalysisSetup Convert(const Base::AnalysisSetupConfig &config);

Base::Histogram Convert(const Base::HistogramConfig &histogram_config);

}

#endif //FLOW_SRC_CONFIG_CONVERT_H
