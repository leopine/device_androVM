
#include "global.hpp"
#include "genyd.hpp"

int main(int argc, char**argv)
{
  SLOGI("Starting genyd...");

  Genyd deamon;

  if (deamon.isInit()) {
    deamon.run();
  }

  SLOGI("Exiting");

  return 0;
}
