//
// Created by eugene on 24/11/2020.
//

#include <DataContainer.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/proto/proto.hpp>

#include <filesystem>
#include <utility>

#include <TFile.h>
#include <TKey.h>
#include <TClass.h>

#include <QnAnalysisTools/QnAnalysisTools.hpp>



int main() {

  TFile f("correlation.root", "READ");

  /*
   * Note 1: Two types of objects:
   *  DataContainer<StatsCollect>:  for Rebins,Merges,etc
   *  DataContainer<StatsCalculate>: Arithmetics
   */

  using namespace Qn::Analysis::Tools;

  for (auto c : Combination(std::vector<std::string>{"psd1", "psd2","psd3"}, std::vector<int>{1,2,3})) {
    std::cout << std::get<0>(c) << " " << std::to_string(std::get<1>(c)) << std::endl;
  }




  return 0;
}

