
#include <string.h>
#include <cutils/log.h>

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

    baseEvent.version = sizeof(baseEvent);
    baseEvent.sensor = SENSORS_HANDLE_BASE + SENSOR_TYPE_ACCELEROMETER;
    baseEvent.type = SENSOR_TYPE_ACCELEROMETER;
    baseEvent.timestamp = getTimestamp();

    baseEvent.acceleration.x = 0.;
    baseEvent.acceleration.y = 0.;
    baseEvent.acceleration.z = 0.;
}

AccelerometerSensor::~AccelerometerSensor(void)
{

}

void AccelerometerSensor::generateEvent(sensors_event_t *data, t_sensor_data rawData)
{
    memset(data, 0, sizeof(*data));
    data->version = sizeof(sensors_event_t);
    data->sensor = SENSORS_HANDLE_BASE + SENSOR_TYPE_ACCELEROMETER;
    data->type = SENSOR_TYPE_ACCELEROMETER;
    data->timestamp = getTimestamp();
    data->acceleration.x = rawData.x;
    data->acceleration.y = rawData.y;
    data->acceleration.z = rawData.z;

    memcpy(&lastEvent, data, sizeof(lastEvent));
}
