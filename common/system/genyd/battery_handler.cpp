
#include <cutils/properties.h>

#include "dispatcher.hpp"
#include "libgenyd.hpp"

void Dispatcher::setBatteryStatus(const Request &request, Reply *reply)
{
    std::string value = request.parameter().value().stringvalue();

    if (property_set(BATTERY_STATUS, value.c_str())) {
        SLOGE("Can't set property");
    } else {
        reply->set_type(Reply::None);
    }
}

void Dispatcher::getBatteryValue(const Request &request, Reply *reply)
{
    // Read keys
    char property_full[PROPERTY_VALUE_MAX];
    char property_value[PROPERTY_VALUE_MAX];
    property_get(CACHE_PREFIX BATTERY_FULL, property_full, "0");
    property_get(CACHE_PREFIX BATTERY_VALUE, property_value, "0");

    int efull = atoi(property_full);
    int enow = atoi(property_value);

    // Compute battery level
    int batlevel = efull ? ((long long)enow)*100/efull : 0;

    // Prepare response
    reply->set_type(Reply::Value);
    Status *status = reply->mutable_status();
    status->set_code(Status::Ok);
    Value *value = reply->mutable_value();
    value->set_type(Value::Uint);

    // Set value in response
    value->set_uintvalue(batlevel);
}

void Dispatcher::isBatteryManual(const Request &request, Reply *reply)
{
    reply->set_type(Reply::Value);
    Status *status = reply->mutable_status();
    status->set_code(Status::Ok);
    Value *value = reply->mutable_value();
    value->set_type(Value::Bool);

    char property[PROPERTY_VALUE_MAX];
    property_get("ro.build.version.release", property, "Unknown");
    value->set_boolvalue(!LibGenyd::useRealValue(BATTERY_VALUE));
}
