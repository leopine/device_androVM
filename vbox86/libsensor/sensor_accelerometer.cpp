
#include <string.h>

#include "sensor_accelerometer.hpp"

AccelerometerSensor::AccelerometerSensor(void)
{
    memset(&sensorCore, 0, sizeof(sensorCore));
    sensorCore.name = "Genymotion Accelerometer";
    sensorCore.vendor = "Genymobile";
    sensorCore.version = 1;
    sensorCore.handle = SENSORS_HANDLE_BASE + SENSOR_TYPE_ACCELEROMETER;
    sensorCore.type = SENSOR_TYPE_ACCELEROMETER;
    sensorCore.maxRange = MAX_RANGE_A;
    sensorCore.resolution = CONVERT_A;
    sensorCore.power = 0.57f;
    sensorCore.minDelay = 2000;
}

AccelerometerSensor::~AccelerometerSensor(void)
{

}


sensors_event_t AccelerometerSensor::getDefaultValue(void) const
{
    sensors_event_t event;

    memset(&event, 0, sizeof(event));

    event.version = sizeof(event);
    event.sensor = SENSORS_HANDLE_BASE + SENSOR_TYPE_ACCELEROMETER;
    event.type = SENSOR_TYPE_ACCELEROMETER;

    event.acceleration.x = 0.;
    event.acceleration.y = 0.;
    event.acceleration.z = 0.;

    return event;
}
