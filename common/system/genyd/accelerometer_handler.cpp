#include <hardware/sensors.h>
#include <cutils/properties.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <string.h>
#include <errno.h>
#include <arpa/inet.h>

#include "dispatcher.hpp"
#include "libgenyd.hpp"
#include "sensor.hpp"

void Dispatcher::setAccelerometerValues(const Request &request, Reply *reply)
{
    std::string acceleration = request.parameter().value().stringvalue();
    t_sensor_data event;
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));

    reply->set_type(Reply::None);
    Status *status = reply->mutable_status();
    status->set_code(Status::Ok);

    sscanf(acceleration.c_str(), "%lf|%lf|%lf", &event.x, &event.y, &event.z);
    event.sensor = SENSOR_TYPE_ACCELEROMETER;

    // Connect to libsensor to send event
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
	SLOGE("Failed opening socket: %s", strerror(errno));
	return;
    }

    addr.sin_family = AF_INET;
    addr.sin_port = htons(LIBSENSOR_PORT);
    addr.sin_addr.s_addr = inet_addr(LIBSENSOR_IP);

    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr))) {
	SLOGE("Failed to connect to local libsensor daemon: %s",
	      strerror(errno));
	return;
    }

    // Send request to libsensor
    if (send(sock, &event, sizeof(event), MSG_NOSIGNAL|MSG_DONTWAIT) != sizeof(event)) {
	ALOGE("Failed to send event: %s", strerror(errno));
    }

    close(sock);
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
