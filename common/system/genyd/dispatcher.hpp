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

    typedef void (Dispatcher::*t_get_callback)(const Request &, Reply *);
    std::map<int, t_get_callback> getCallbacks;

    typedef void (Dispatcher::*t_set_callback)(const Request &, Reply *);
    std::map<int, t_set_callback> setCallbacks;

public:
    // Switch among requests
    Reply *dispatchRequest(const Request &request);


private:

    /////////////////////////
    // Dispatchers         //
    /////////////////////////

    // Answer "Ping" request
    void treatPing(const Request &request, Reply *reply);

    // Answer "GetParam" requests
    void treatGetParam(const Request &request, Reply *reply);

    // Answer "SetParam" requests
    void treatSetParam(const Request &request, Reply *reply);

    // Fallback for unknown requests
    void unknownRequest(const Request &request, Reply *reply);




    /////////////////////////
    // Generic requests    //
    /////////////////////////

    // Answer "GetParam AndroidVersion" request
    void getAndroidVersion(const Request &request, Reply *reply);




    /////////////////////////
    // Battery requests    //
    // battery_handler.cpp //
    /////////////////////////

    // Action behind "SetParam Battery Value" request
    void setBatteryValue(const Request &request, Reply *reply);

    // Answer "GetParam Battery Value" request
    void getBatteryValue(const Request &request, Reply *reply);

    // Answer "SetParam Battery Status" request
    void setBatteryStatus(const Request &request, Reply *reply);

    // Answer "GetParam Battery Status" request
    void getBatteryStatus(const Request &request, Reply *reply);

    // Answer "GetParam Battery Is manual" request
    void isBatteryManual(const Request &request, Reply *reply);
};

#endif
