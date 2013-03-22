
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
  value->set_stringvalue("Android 4.1.1");
}

void Dispatcher::treatGetParam(const Request &request, Reply *reply)
{
  Parameter param = request.parameter();

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
