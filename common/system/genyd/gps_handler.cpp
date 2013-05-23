#include <cutils/properties.h>

#include "dispatcher.hpp"
#include "libgenyd.hpp"

// Check if GPS Status is enabled (and then, we should read values from stored properties)
bool isGpsStatusEnabled()
{
    char str_value[PROPERTY_VALUE_MAX];
    property_get(GPS_STATUS, str_value, GPS_DEFAULT_STATUS);

    int comp = strcmp(str_value, GPS_ENABLED);
    return comp == 0;
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

    // Set value in response
    bool enabled = isGpsStatusEnabled();
    value->set_boolvalue(enabled);
}

// Answer "SetParam GPS Status" request
void Dispatcher::setGpsStatus(const Request &request, Reply *reply)
{
    bool gps_status = request.parameter().value().boolvalue();

    reply->set_type(Reply::None);
    Status *status = reply->mutable_status();
    status->set_code(Status::Ok);

    property_set(GPS_STATUS, gps_status ? GPS_ENABLED : GPS_DISABLED);
}

// Helper method that reads a double value, given its key and protobuf request object
void getDoubleParam(const char* key, const Request &request, Reply *reply)
{
    // Prepare response
    reply->set_type(Reply::Value);
    Status *status = reply->mutable_status();
    status->set_code(Status::Ok);
    Value *value = reply->mutable_value();
    value->set_type(Value::Float);

    // Read property
    char str_value[PROPERTY_VALUE_MAX];
    if (isGpsStatusEnabled()) {
        property_get(key, str_value, "0");
    } else {
        str_value[0] = '0';
        str_value[1] = '\0';
    }

    // Converts to double
    double val = atof(str_value);

    // Set value in response
    value->set_floatvalue(val);
}

void setDoubleParam(const char* key, const Request &request, Reply *reply,
                    double lower, double upper,
                    bool lowerIncluded = true, bool upperIncluded = true)
{
    double value = request.parameter().value().floatvalue();

    // Prepare default OK response
    Status *status = reply->mutable_status();
    reply->set_type(Reply::None);
    status->set_code(Status::Ok);

    // Check value limits
    bool lowerValid = lowerIncluded ? (value >= lower) : (value > lower);
    bool upperValid = upperIncluded ? (value <= upper) : (value < upper);

    if (!(lowerValid && upperValid)) {
        SLOGE("Invalid value %d", value);
        reply->set_type(Reply::Error);
        status->set_code(Status::InvalidRequest);
        return;
    }

    if (!isGpsStatusEnabled()) {
        // Enable GPS by forcing Status to true
        property_set(GPS_STATUS, GPS_ENABLED);

        // Inform the user of mode switching
        reply->set_type(Reply::None);
        status->set_code(Status::OkWithInformation);
        status->set_description("GPS have been enabled");

        SLOGI("Genyd enabled GPS automatically");
    }

    // Read property
    char str_value[PROPERTY_VALUE_MAX];
    snprintf(str_value, PROPERTY_VALUE_MAX, "%f", value);

    // Set property
    property_set(key, str_value);
}

// Answer "GetParam GPS Latitude" request
void Dispatcher::getGpsLatitude(const Request &request, Reply *reply)
{
    getDoubleParam(GPS_LATITUDE, request, reply);
}

// Answer "SetParam GPS Latitude" request
void Dispatcher::setGpsLatitude(const Request &request, Reply *reply)
{
    setDoubleParam(GPS_LATITUDE, request, reply, -90, 90, true, false);
}

// Answer "GetParam GPS Longitude" request
void Dispatcher::getGpsLongitude(const Request &request, Reply *reply)
{
    getDoubleParam(GPS_LONGITUDE, request, reply);
}

// Answer "SetParam GPS Longitude" request
void Dispatcher::setGpsLongitude(const Request &request, Reply *reply)
{
    setDoubleParam(GPS_LONGITUDE, request, reply, -180, 180, true, false);
}

// Answer "GetParam GPS Altitude" request
void Dispatcher::getGpsAltitude(const Request &request, Reply *reply)
{
    getDoubleParam(GPS_ALTITUDE, request, reply);
}

// Answer "SetParam GPS Altitude" request
void Dispatcher::setGpsAltitude(const Request &request, Reply *reply)
{
    setDoubleParam(GPS_ALTITUDE, request, reply, -20, 10000);
}

// Answer "GetParam GPS Accuracy" request
void Dispatcher::getGpsAccuracy(const Request &request, Reply *reply)
{
    getDoubleParam(GPS_ACCURACY, request, reply);
}

// Answer "SetParam GPS Accuracy" request
void Dispatcher::setGpsAccuracy(const Request &request, Reply *reply)
{
    setDoubleParam(GPS_ACCURACY, request, reply, 0, 200);
}

// Answer "GetParam GPS Bearing" request
void Dispatcher::getGpsBearing(const Request &request, Reply *reply)
{
    getDoubleParam(GPS_BEARING, request, reply);
}

// Answer "SetParam GPS Bearing" request
void Dispatcher::setGpsBearing(const Request &request, Reply *reply)
{
    setDoubleParam(GPS_BEARING, request, reply, 0, 360, true, false);
}
