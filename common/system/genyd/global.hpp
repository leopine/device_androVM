#ifndef GLOBAL_HPP_
#define GLOBAL_HPP_

#ifndef LOG_TAG
#define LOG_TAG "Genyd"
#endif

#include <cutils/log.h>

#include "device/androVM/common/system/genyd/requests.pb.h"

using Genymotion::Parameter;
using Genymotion::Request;
using Genymotion::Status;
using Genymotion::Reply;
using Genymotion::Value;

// Properties keys

// Battery
#define BATTERY_VALUE    "genyd.battery.value"
#define BATTERY_STATUS   "genyd.battery.status"
#define BATTERY_LOADTYPE "genyd.battery.load_type"

#endif
