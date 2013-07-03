#ifndef GENYMOTION_SENSOR_HPP
#define GENYMOTION_SENSOR_HPP

#define LSG         (1024.0f)
#define MAX_RANGE_A (2*GRAVITY_EARTH)
#define CONVERT_A   (GRAVITY_EARTH / LSG)
#define CONVERT_A_X (CONVERT_A)
#define CONVERT_A_Y (CONVERT_A)
#define CONVERT_A_Z (CONVERT_A)

#include <hardware/sensors.h>

#define NO_PROTOBUF
#include "global.hpp"

typedef struct s_sensor_data {
    u_int64_t sensor;
    double x;
    double y;
    double z;
} t_sensor_data;

class Sensor {

 public:
    Sensor(void);
    virtual ~Sensor(void);

 protected:
    sensors_event_t lastEvent;
    sensors_event_t baseEvent;
    sensor_t sensorCore;
    bool enabled;

 public:
    virtual void generateEvent(sensors_event_t *data, t_sensor_data rawData) = 0;
    sensors_event_t *getBaseEvent(void);
    sensors_event_t *getLastEvent(void);
    sensor_t getSensorCore(void) const;
    void setEnabled(bool enabled);
    bool isEnabled(void) const;

protected:
    // Get the current timestamp, in nanoseconds
    int64_t getTimestamp(void) const;

    // Function to write property
    static int setProperty(const char *property, const char *value);

};

#endif
