#include <iostream>
#include <string>

#include <boost/program_options.hpp>

#include "CorrelationTaskRunner.hpp"

using namespace Qn::Analysis::Correlate;
using namespace boost::program_options;

int main(int argc, char **argv) {

  CorrelationTaskRunner runner;

  options_description desc("Common options");
  desc.add_options()
      ("help", "Print help message")
      ;

  desc.add(runner.GetBoostOptions());

  variables_map vm;

  store(parse_command_line(argc, argv, desc), vm);

  if (vm.count("help")) {
    std::cout << desc << std::endl;
    return 0;
  }

  vm.notify();


  runner.Initialize();
  runner.Run();


  return 0;
}
