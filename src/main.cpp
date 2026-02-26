#include "cli/cli.h"
#include <iostream>
#include <cstdlib>

int main(int argc, char *argv[])
{
  try
  {
    logengine::CLI cli;
    return cli.run(argc, argv);
  }
  catch (const std::exception &e)
  {
    std::cerr << "Fatal error: " << e.what() << std::endl;
    return EXIT_FAILURE;
  }
}