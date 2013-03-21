
#include <cutils/log.h>
#define LOG_TAG "Genyd"

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
