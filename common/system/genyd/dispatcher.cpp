
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

void Dispatcher::unknownRequest(const Request &request, Reply *reply)
{
  (void)request;
  reply->set_type(Reply::Error);
}

Reply *Dispatcher::dispatchRequest(const Request &request)
{
  (void)request;

  Reply *reply = new Reply();

  switch (request.type()) {
  case Request::Ping:
    treatPing(request, reply);
    break;
  default:
    unknownRequest(request, reply);
    break;
  }

  return (reply);
}
