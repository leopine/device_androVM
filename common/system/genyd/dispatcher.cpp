
#include "dispatcher.hpp"

Dispatcher::Dispatcher(void)
{

}

Dispatcher::~Dispatcher(void)
{

}

Reply *Dispatcher::dispatchRequest(const Request &request)
{
  (void)request;

  Reply *reply = NULL;
  // Test
  reply = new Reply();
  reply->set_type(Reply::Value);
  Status *status = reply->mutable_status();
  status->set_code(Status::Ok);
  Value *value = reply->mutable_value();
  value->set_type(Value::String);
  value->set_stringvalue("Android 4.1.1");
  // End test

  return (reply);
}
