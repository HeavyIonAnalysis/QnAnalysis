//
// Created by eugene on 19/10/2021.
//

#ifndef QNANALYSIS_SRC_QNANALYSISBASE_ANALYSISTREE_HPP_
#define QNANALYSIS_SRC_QNANALYSISBASE_ANALYSISTREE_HPP_

#include <AnalysisTree/AnalysisTreeVersion.hpp>

#if ANALYSISTREE_VERSION_MAJOR == 1

#include <AnalysisTree/BranchReader.hpp>
#include <AnalysisTree/Variable.hpp>
#include <AnalysisTree/VarManager.hpp>
#include <AnalysisTree/VarManagerEntry.hpp>

using ATBranchReader = ::AnalysisTree::BranchReader;
using ATVariable = ::AnalysisTree::Variable;
using ATVarManager = ::AnalysisTree::VarManager;
using ATVarManagerEntry = ::AnalysisTree::VarManagerEntry;

#elif ANALYSISTREE_VERSION_MAJOR == 2

#include <AnalysisTree/infra-1.0/BranchReader.hpp>
#include <AnalysisTree/infra-1.0/Variable.hpp>
#include <AnalysisTree/infra-1.0/VarManager.hpp>
#include <AnalysisTree/infra-1.0/VarManagerEntry.hpp>

using ATBranchReader = ::AnalysisTree::Version1::BranchReader;
using ATVariable = ::AnalysisTree::Version1::Variable;
using ATVarManager = ::AnalysisTree::Version1::VarManager;
using ATVarManagerEntry = ::AnalysisTree::Version1::VarManagerEntry;

#endif


#endif //QNANALYSIS_SRC_QNANALYSISBASE_ANALYSISTREE_HPP_
