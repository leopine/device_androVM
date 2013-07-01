#ifndef SENSOR_ACCELEROMETTER_HPP
#define SENSOR_ACCELEROMETER_HPP

#include "sensor.hpp"

class AccelerometerSensor : public Sensor {

public:
    AccelerometerSensor(void);
    virtual ~AccelerometerSensor(void);

private:
    AccelerometerSensor(const AccelerometerSensor &);
    AccelerometerSensor operator=(const AccelerometerSensor &);

public:
    virtual sensors_event_t getDefaultValue(void) const;

};

#endif
