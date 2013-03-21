
#include <cutils/log.h>
#define LOG_TAG "Genyd"

#include "genyd.hpp"

#include "device/androVM/common/system/genyd/requests.pb.h"

using Genymotion::Request;

int main(int argc, char**argv)
{
  Request r;
  SLOGI("Starting genyd...");

  Genyd deamon;

  if (deamon.isInit()) {
    deamon.run();
  }

  SLOGI("Exiting");

  return 0;
}
