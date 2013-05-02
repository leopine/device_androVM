#ifndef GLOBAL_HPP_
#define GLOBAL_HPP_

#ifndef LOG_TAG
#define LOG_TAG "Genyd"
#endif

#include <cutils/log.h>

#ifndef __NO_PROTO
  #include "device/androVM/common/system/genyd/requests.pb.h"

  using Genymotion::Parameter;
  using Genymotion::Request;
  using Genymotion::Status;
  using Genymotion::Reply;
  using Genymotion::Value;
#endif


// Property keys cache. For example, cache.genyd.battery.value refers
// to real battery value has AOSP would see in manual mode
#define CACHE_PREFIX     "genyd."

// Properties keys

#define VALUE_USE_REAL   "--use-real-value--"

// Battery
#define BATTERY_FULL     CACHE_PREFIX "battery.full"
#define BATTERY_VALUE    CACHE_PREFIX "battery.value"
#define BATTERY_STATUS   CACHE_PREFIX "battery.status"
#define BATTERY_LOADTYPE CACHE_PREFIX "battery.load_type"

#endif
