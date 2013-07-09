#ifndef GLOBAL_HPP_
#define GLOBAL_HPP_

#ifndef LOG_TAG
#define LOG_TAG "Genyd"
#endif

#include <cutils/log.h>

#ifndef NO_PROTOBUF
#include "device/androVM/common/system/genyd/requests.pb.h"

using Genymotion::Parameter;
using Genymotion::Request;
using Genymotion::Status;
using Genymotion::Reply;
using Genymotion::Value;

#endif

#define LIBSENSOR_PORT (22471)
#define LIBSENSOR_IP   "127.0.0.1"

// Property keys naming convention
#define KEY_PREFIX            "genyd."

// Cache values: For example, genyd.battery.value.cache refers
// to real battery value has AOSP would see in manual mode
#define CACHE_SUFFIX          ".cached"

// Properties keys

#define MANUAL_MODE           "manual"
#define AUTO_MODE             "auto"

// BATTERY
#define BATTERY_MODE          KEY_PREFIX "battery.mode"
#define BATTERY_FULL          KEY_PREFIX "battery.full"
#define BATTERY_LEVEL         KEY_PREFIX "battery.level"
#define BATTERY_STATUS        KEY_PREFIX "battery.status"
#define BATTERY_LOADTYPE      KEY_PREFIX "battery.load_type"
#define AC_ONLINE             KEY_PREFIX "ac.online"

// GPS
#define GPS_STATUS            KEY_PREFIX "gps.status"
#define GPS_LATITUDE          KEY_PREFIX "gps.latitude"
#define GPS_LONGITUDE         KEY_PREFIX "gps.longitude"
#define GPS_ALTITUDE          KEY_PREFIX "gps.altitude"
#define GPS_ACCURACY          KEY_PREFIX "gps.accuracy"
#define GPS_BEARING           KEY_PREFIX "gps.bearing"

// ACCELEROMETER
#define ACCELEROMETER_X       KEY_PREFIX "accelerometer.x"
#define ACCELEROMETER_Y       KEY_PREFIX "accelerometer.y"
#define ACCELEROMETER_Z       KEY_PREFIX "accelerometer.z"

#define GPS_DISABLED          "disabled"
#define GPS_ENABLED           "enabled"
#define GPS_DEFAULT_STATUS    GPS_DISABLED
#define GPS_DEFAULT_ACCURACY  "1"

// CAPABILITIES
#define CAPABILITY_PREFIX        "ro.genymotion.capability."
#define CAPABILITY_BATTERY       CAPABILITY_PREFIX "battery"
#define CAPABILITY_GPS           CAPABILITY_PREFIX "gps"
#define CAPABILITY_ACCELEROMETER CAPABILITY_PREFIX "accelerometer"

#endif
