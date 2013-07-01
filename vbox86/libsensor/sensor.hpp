#ifndef GENYMOTION_SENSOR_HPP
#define GENYMOTION_SENSOR_HPP

#define LSG         (1024.0f)
#define MAX_RANGE_A (2*GRAVITY_EARTH)
#define CONVERT_A   (GRAVITY_EARTH / LSG)
#define CONVERT_A_X (CONVERT_A)
#define CONVERT_A_Y (CONVERT_A)
#define CONVERT_A_Z (CONVERT_A)

#include <hardware/sensors.h>

class Sensor {

 public:
    Sensor(void);
    virtual ~Sensor(void);

 protected:
    sensor_t sensorCore;
    bool enabled;

 public:
    virtual sensors_event_t getDefaultValue(void) const = 0;
    sensor_t getSensorCore(void) const;
    void setEnabled(bool enabled);
    bool isEnabled(void) const;
};

#endif
