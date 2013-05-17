
#include <cutils/properties.h>

#include "dispatcher.hpp"

Dispatcher::Dispatcher(void)
{
    // "GetParam" callbacks list
    getCallbacks[Parameter::AndroidVersion] = &Dispatcher::getAndroidVersion;
    getCallbacks[Parameter::BatteryStatus] = &Dispatcher::getBatteryStatus;
    getCallbacks[Parameter::BatteryLevel] = &Dispatcher::getBatteryLevel;
    getCallbacks[Parameter::BatteryMode] = &Dispatcher::isBatteryManual;
    getCallbacks[Parameter::GpsStatus] = &Dispatcher::getGpsStatus;

    // "SetParam" callback list
    setCallbacks[Parameter::BatteryStatus] = &Dispatcher::setBatteryStatus;
    setCallbacks[Parameter::BatteryLevel] = &Dispatcher::setBatteryLevel;
    setCallbacks[Parameter::BatteryMode] = &Dispatcher::setBatteryMode;
    setCallbacks[Parameter::GpsStatus] = &Dispatcher::setGpsStatus;
}

Dispatcher::~Dispatcher(void)
{

}

void Dispatcher::treatPing(const Request &request, Reply *reply)
{
    (void)request;
    reply->set_type(Reply::Pong);
    Status *status = reply->mutable_status();
    status->set_code(Status::Ok);
}

void Dispatcher::getAndroidVersion(const Request &request, Reply *reply)
{
    reply->set_type(Reply::Value);
    Status *status = reply->mutable_status();
    status->set_code(Status::Ok);
    Value *value = reply->mutable_value();
    value->set_type(Value::String);

    char property[PROPERTY_VALUE_MAX];
    property_get("ro.build.version.release", property, "Unknown");
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

    std::map<int, Dispatcher::t_get_callback>::iterator func = getCallbacks.find(param.type());

    if (func != getCallbacks.end()) {
        (this->*(func->second))(request, reply);
    } else {
        reply->set_type(Reply::Error);
        Status *status = reply->mutable_status();
        status->set_code(Status::GenericError);
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

    std::map<int, Dispatcher::t_set_callback>::iterator func = setCallbacks.find(param.type());

    if (func != setCallbacks.end()) {
        (this->*(func->second))(request, reply);
    } else {
        reply->set_type(Reply::Error);
        Status *status = reply->mutable_status();
        status->set_code(Status::GenericError);
    }
}

void Dispatcher::unknownRequest(const Request &request, Reply *reply)
{
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
