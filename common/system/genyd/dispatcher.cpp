
#include "dispatcher.hpp"

Dispatcher::Dispatcher(void)
{

}

Dispatcher::~Dispatcher(void)
{

}

void Dispatcher::treatePing(const Request &request, Reply *reply)
{
  (void)request;
  reply->set_type(Reply::Pong);
  Status *status = reply->mutable_status();
  status->set_code(Status::Ok);
}

void Dispatcher::unknownRequest(const Request &request, Reply *reply)
{
  (void)request;
  reply->set_type(Reply::Error);
}

Reply *Dispatcher::dispatchRequest(const Request &request)
{
  (void)request;

  Reply *reply = new Reply();;
  // Test

  switch (request.type()) {
  case Request::Ping:
    treatePing(request, reply);
  default:
    unknownRequest(request, reply);
  }

  return (reply);
}
