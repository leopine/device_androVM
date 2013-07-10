
#include <cutils/properties.h>

#include "dispatcher.hpp"

Dispatcher::Dispatcher(void)
{
    // "GetParam" callbacks list
    getCallbacks[Parameter::AndroidVersion] = &Dispatcher::getAndroidVersion;
    getCallbacks[Parameter::GenymotionVersion] = &Dispatcher::getGenymotionVersion;
    getCallbacks[Parameter::BatteryStatus] = &Dispatcher::getBatteryStatus;
    getCallbacks[Parameter::BatteryLevel] = &Dispatcher::getBatteryLevel;
    getCallbacks[Parameter::BatteryMode] = &Dispatcher::isBatteryManual;
    getCallbacks[Parameter::GpsStatus] = &Dispatcher::getGpsStatus;
    getCallbacks[Parameter::GpsLatitude] = &Dispatcher::getGpsLatitude;
    getCallbacks[Parameter::GpsLongitude] = &Dispatcher::getGpsLongitude;
    getCallbacks[Parameter::GpsAltitude] = &Dispatcher::getGpsAltitude;
    getCallbacks[Parameter::GpsAccuracy] = &Dispatcher::getGpsAccuracy;
    getCallbacks[Parameter::GpsBearing] = &Dispatcher::getGpsBearing;
    getCallbacks[Parameter::Accelerometer] = &Dispatcher::getAccelerometerValues;

    // "SetParam" callback list
    setCallbacks[Parameter::BatteryStatus] = &Dispatcher::setBatteryStatus;
    setCallbacks[Parameter::BatteryLevel] = &Dispatcher::setBatteryLevel;
    setCallbacks[Parameter::BatteryMode] = &Dispatcher::setBatteryMode;
    setCallbacks[Parameter::GpsStatus] = &Dispatcher::setGpsStatus;
    setCallbacks[Parameter::GpsLatitude] = &Dispatcher::setGpsLatitude;
    setCallbacks[Parameter::GpsLongitude] = &Dispatcher::setGpsLongitude;
    setCallbacks[Parameter::GpsAltitude] = &Dispatcher::setGpsAltitude;
    setCallbacks[Parameter::GpsAccuracy] = &Dispatcher::setGpsAccuracy;
    setCallbacks[Parameter::GpsBearing] = &Dispatcher::setGpsBearing;
    setCallbacks[Parameter::Accelerometer] = &Dispatcher::setAccelerometerValues;
}

Dispatcher::~Dispatcher(void)
{
}

void Dispatcher::treatPing(const Request &request, Reply *reply)
{
    (void)request;
    SLOGD("Received Ping");

    reply->set_type(Reply::Pong);
    Status *status = reply->mutable_status();
    status->set_code(Status::Ok);
}

void Dispatcher::getAndroidVersion(const Request &request, Reply *reply)
{
    SLOGD("Received Get AndroidVersion");

    reply->set_type(Reply::Value);
    Status *status = reply->mutable_status();
    status->set_code(Status::Ok);
    Value *value = reply->mutable_value();
    value->set_type(Value::String);

    char property[PROPERTY_VALUE_MAX];
    property_get(ANDROID_VERSION, property, "Unknown");
    value->set_stringvalue(property);
}

void Dispatcher::getGenymotionVersion(const Request &request, Reply *reply)
{
    SLOGD("Received Get GenymotionVersion");

    reply->set_type(Reply::Value);
    Status *status = reply->mutable_status();
    status->set_code(Status::Ok);
    Value *value = reply->mutable_value();
    value->set_type(Value::String);

    char property[PROPERTY_VALUE_MAX];
    property_get(GENYMOTION_VERSION, property, "Unknown");
    value->set_stringvalue(property);
}

void Dispatcher::treatGetParam(const Request &request, Reply *reply)
{
    if (!request.has_parameter()) {
        reply->set_type(Reply::Error);
        Status *status = reply->mutable_status();
        status->set_code(Status::GenericError);
        return;
    }

    Parameter param = request.parameter();
    Genymotion::Parameter_Type type = param.type();
    SLOGD("Received Get %s", Parameter::Type_Name(type).c_str());

    std::map<int, Dispatcher::t_get_callback>::iterator func = getCallbacks.find(type);

    if (func != getCallbacks.end()) {
        (this->*(func->second))(request, reply);
    } else {
        reply->set_type(Reply::Error);
        Status *status = reply->mutable_status();
        status->set_code(Status::NotImplemented);
    }
}

void Dispatcher::treatSetParam(const Request &request, Reply *reply)
{
    if (!request.has_parameter()) {
        reply->set_type(Reply::Error);
        Status *status = reply->mutable_status();
        status->set_code(Status::GenericError);
        return;
    }

    Parameter param = request.parameter();

    if (!param.has_value()) {
        reply->set_type(Reply::Error);
        Status *status = reply->mutable_status();
        status->set_code(Status::GenericError);
        return;
    }

    Genymotion::Parameter_Type type = param.type();
    SLOGD("Received Set %s", Parameter::Type_Name(type).c_str());

    std::map<int, Dispatcher::t_set_callback>::iterator func = setCallbacks.find(type);

    if (func != setCallbacks.end()) {
        (this->*(func->second))(request, reply);
    } else {
        reply->set_type(Reply::Error);
        Status *status = reply->mutable_status();
        status->set_code(Status::NotImplemented);
    }
}

void Dispatcher::unknownRequest(const Request &request, Reply *reply)
{
    SLOGD("Received unknown request");
    (void)request;
    reply->set_type(Reply::Error);
    Status *status = reply->mutable_status();
    status->set_code(Status::InvalidRequest);
}

Reply *Dispatcher::dispatchRequest(const Request &request)
{
    (void)request;

    Reply *reply = new Reply();

    switch (request.type()) {
    case Request::Ping:
        treatPing(request, reply);
        break;
    case Request::SetParam:
        treatSetParam(request, reply);
        break;
    case Request::GetParam:
        treatGetParam(request, reply);
        break;
    default:
        unknownRequest(request, reply);
        break;
    }

    return (reply);
}
