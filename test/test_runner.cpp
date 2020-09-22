#include <gtest/gtest.h>

#include <AnalysisTree/ToyMC.hpp>
#include <random>

int main(int argc, char **argv) {

  const int n_events = 1000;

  AnalysisTree::ToyMC<std::default_random_engine> toy_mc;
  toy_mc.GenerateEvents(n_events);
  toy_mc.WriteToFile("toy_mc.root", "fl_toy_mc.txt");

  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
