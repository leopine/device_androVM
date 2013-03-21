#ifndef DISPATCHER_HPP_
#define DISPATCHER_HPP_

#include "global.hpp"

class Dispatcher {

public:
  Dispatcher(void);
  ~Dispatcher(void);

private:
  Dispatcher(const Dispatcher &);
  Dispatcher operator=(const Dispatcher &);

public:
  Reply *dispatchRequest(const Request &request);

};

#endif
