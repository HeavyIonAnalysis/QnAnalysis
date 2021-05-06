//
// Created by eugene on 06/05/2021.
//

#ifndef QNANALYSIS_SRC_QNANALYSISOBSERVABLESEK_NA61_USING_HPP_
#define QNANALYSIS_SRC_QNANALYSISOBSERVABLESEK_NA61_USING_HPP_

#include <DataContainer.hpp>
#include <StatCollect.hpp>
#include <StatCalculate.hpp>
#include <ResourceManager.hpp>

#include "Format.hpp"
#include "Predicates.hpp"
#include "Tools.hpp"

using Meta = ResourceManager::MetaType;

using DTCalc = Qn::DataContainerStatCalculate;
using DTColl = Qn::DataContainerStatCollect;

using ::Tools::Format;
using Tmpltor = ::Predicates::MetaTemplateGenerator;
using ::Predicates::MetaFeatureSet;

using Predicates::Resource::META;
using Predicates::Resource::KEY;
using Predicates::Resource::BASE_OF;

using ::Tools::Define;

#endif //QNANALYSIS_SRC_QNANALYSISOBSERVABLESEK_NA61_USING_HPP_
