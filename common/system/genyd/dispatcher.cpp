
#include <cutils/properties.h>

#include "dispatcher.hpp"

Dispatcher::Dispatcher(void)
{

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
  Parameter param = request.parameter();

  if (!request.has_parameter()) {
    reply->set_type(Reply::Error);
    Status *status = reply->mutable_status();
    status->set_code(Status::GenericError);
    return;
  }

  switch (param.type()) {
  case Parameter::AndroidVersion:
    getAndroidVersion(request, reply);
    break;
  default:
    reply->set_type(Reply::Error);
    Status *status = reply->mutable_status();
    status->set_code(Status::GenericError);
    break;
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
  case Request::GetParam:
    treatGetParam(request, reply);
    break;
  default:
    unknownRequest(request, reply);
    break;
  }

  return (reply);
}
