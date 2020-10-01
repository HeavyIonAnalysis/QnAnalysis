#include <iostream>
#include <string>

#include <boost/program_options.hpp>

#include "CorrelationTask.h"
/*
 *
 *
 */


int main(int argc, char **argv) {
  using namespace std;
  using namespace boost::program_options;

  CorrelationTask task;




  try {
    options_description desc("Common options");
    desc.add_options()("help,h", "usage");

    desc.add(task.GetBoostOptions());

    variables_map vm;
    store(parse_command_line(argc, argv, desc), vm);

    if (vm.count("help")) {
      std::cout << desc << std::endl;
      return 0;
    }

    vm.notify();
  }
  catch (exception &e) {
    cerr << e.what() << endl;
    return 1;
  }

  task.Initialize();
  task.Run();
  task.Finalize();

  return 0;
}
