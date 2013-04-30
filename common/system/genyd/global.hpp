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

// Properties keys

#define VALUE_USE_REAL   "--use-real-value--"

// Battery
#define BATTERY_VALUE    "genyd.battery.value"
#define BATTERY_STATUS   "genyd.battery.status"
#define BATTERY_LOADTYPE "genyd.battery.load_type"

#endif
