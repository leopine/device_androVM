#include <cutils/properties.h>

#include "dispatcher.hpp"

#define TOSTRING(str) #str

static inline std::string getProp(const char *key)
{
    char value[PROPERTY_VALUE_MAX];
    property_get(GPS_STATUS, value, "off");

    return value;
}

static std::string getCapabilities(void)
{
    std::string capabilities = "{";
    capabilities += "\"battery\" : \"" + getProp(CAPABILITY_BATTERY) + "\", ";
    capabilities += "\"gps\" : \"" + getProp(CAPABILITY_GPS) + "\", ";
    capabilities += "\"accelerometer\" : \"" + getProp(CAPABILITY_ACCELEROMETER) + "\", ";
    capabilities += "}";

    return capabilities;
}

void Dispatcher::treatCapabilities(const Request &request, Reply *reply)
{
    (void)request;
    SLOGD("Received Capabilities");

    reply->set_type(Reply::Value);
    Status *status = reply->mutable_status();
    status->set_code(Status::Ok);
    Value *value = reply->mutable_value();
    value->set_type(Value::String);
    value->set_stringvalue(getCapabilities());
}
