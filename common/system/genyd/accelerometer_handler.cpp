
#include <cutils/properties.h>

#include "dispatcher.hpp"
#include "libgenyd.hpp"
#include "sensor.hpp"

void Dispatcher::setAccelerometerValues(const Request &request, Reply *reply)
{
    std::string acceleration = request.parameter().value().stringvalue();
    t_sensor_data event;

    reply->set_type(Reply::None);
    Status *status = reply->mutable_status();
    status->set_code(Status::Ok);

    sscanf(acceleration.c_str(), "%lf|%lf|%lf", &event.x, &event.y, &event.z);
}

void Dispatcher::getAccelerometerValues(const Request &request, Reply *reply)
{
    // Prepare response
    reply->set_type(Reply::Value);
    Status *status = reply->mutable_status();
    status->set_code(Status::Ok);
    Value *value = reply->mutable_value();
    value->set_type(Value::String);

    char x[PROPERTY_VALUE_MAX];
    char y[PROPERTY_VALUE_MAX];
    char z[PROPERTY_VALUE_MAX];

    property_get(ACCELEROMETER_X, x, "0.0");
    property_get(ACCELEROMETER_Y, y, "0.0");
    property_get(ACCELEROMETER_Z, z, "0.0");

    std::string acceleration;
    acceleration += x;
    acceleration += "|";
    acceleration += y;
    acceleration += "|";
    acceleration += z;

    // Set value in response
    value->set_stringvalue(acceleration);
}
