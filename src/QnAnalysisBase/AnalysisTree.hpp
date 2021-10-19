//
// Created by eugene on 19/10/2021.
//

#ifndef QNANALYSIS_SRC_QNANALYSISBASE_ANALYSISTREE_HPP_
#define QNANALYSIS_SRC_QNANALYSISBASE_ANALYSISTREE_HPP_

#include <AnalysisTreeVersion.hpp>

#if ANALYSISTREE_VERSION_MAJOR == 1

#include <AnalysisTree/Variable.hpp>
#include <AnalysisTree/VarManager.hpp>

using ATVariable = ::AnalysisTree::Variable;
using ATVarManager = ::AnalysisTree::VarManager;

#elif ANALYSISTREE_VERSION_MAJOR == 2

#include <AnalysisTree/infra-1.0/Variable.hpp>
#include <AnalysisTree/infra-1.0/VarManager.hpp>

using ATVariable = ::AnalysisTree::Version1::Variable;
using ATVarManager = ::AnalysisTree::Version1::VarManager;

#endif


#endif //QNANALYSIS_SRC_QNANALYSISBASE_ANALYSISTREE_HPP_
