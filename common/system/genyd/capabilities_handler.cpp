#include <cutils/properties.h>

#include "dispatcher.hpp"

static inline std::string getcapability(const char *key)
{
    char value[PROPERTY_VALUE_MAX];
    property_get(key, value, "off");

    return value;
}

static std::string getCapabilitiesJSON(void)
{
    std::string capabilities;

    capabilities += "{";
    capabilities += "\"battery\" : \"" + getcapability(CAPABILITY_BATTERY) + "\", ";
    capabilities += "\"gps\" : \"" + getcapability(CAPABILITY_GPS) + "\", ";
    capabilities += "\"accelerometer\" : \"" + getcapability(CAPABILITY_ACCELEROMETER) + "\", ";
    capabilities += "}";

    return capabilities;
}

void Dispatcher::getCapabilities(const Request &request, Reply *reply)
{
    (void)request;

    reply->set_type(Reply::Value);
    Status *status = reply->mutable_status();
    status->set_code(Status::Ok);
    Value *value = reply->mutable_value();
    value->set_type(Value::String);
    value->set_stringvalue(getCapabilitiesJSON());
}
