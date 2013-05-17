#include <cutils/properties.h>

#include "dispatcher.hpp"
#include "libgenyd.hpp"


// Answer "SetParam GPS Status" request
void Dispatcher::setGpsStatus(const Request &request, Reply *reply)
{
    bool gps_status = request.parameter().value().boolvalue();

    reply->set_type(Reply::None);
    Status *status = reply->mutable_status();
    status->set_code(Status::Ok);

    property_set(GPS_STATUS, gps_status ? "true" : "false");
}

// Answer "GetParam GPS Status" request
void Dispatcher::getGpsStatus(const Request &request, Reply *reply)
{
    // Prepare response
    reply->set_type(Reply::Value);
    Status *status = reply->mutable_status();
    status->set_code(Status::Ok);
    Value *value = reply->mutable_value();
    value->set_type(Value::Bool);

    char gps_status[PROPERTY_VALUE_MAX];

    property_get(GPS_STATUS, gps_status, "false");

    int cmp = strncmp(gps_status, "true", 4);
    // Set value in response
    value->set_boolvalue(cmp == 0);
}
