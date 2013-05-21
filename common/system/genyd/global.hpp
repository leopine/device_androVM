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

// Property keys naming convention
#define KEY_PREFIX       "genyd."

// Cache values: For example, genyd.battery.value.cache refers
// to real battery value has AOSP would see in manual mode
#define CACHE_SUFFIX     ".cached"

// Properties keys

#define MANUAL_MODE         "manual"
#define AUTO_MODE           "auto"
#define GPS_STATUS_ENABLED  "true"

// Battery
#define BATTERY_MODE     KEY_PREFIX "battery.mode"
#define BATTERY_FULL     KEY_PREFIX "battery.full"
#define BATTERY_LEVEL    KEY_PREFIX "battery.level"
#define BATTERY_STATUS   KEY_PREFIX "battery.status"
#define BATTERY_LOADTYPE KEY_PREFIX "battery.load_type"
#define AC_ONLINE        KEY_PREFIX "ac.online"

// GPS
#define GPS_STATUS       KEY_PREFIX "gps.status"
#define GPS_LATITUDE     KEY_PREFIX "gps.latitude"
#define GPS_LONGITUDE    KEY_PREFIX "gps.longitude"
#define GPS_ALTITUDE     KEY_PREFIX "gps.altitude"
#define GPS_ACCURACY     KEY_PREFIX "gps.accuracy"
#define GPS_BEAR         KEY_PREFIX "gps.bear"

#endif
