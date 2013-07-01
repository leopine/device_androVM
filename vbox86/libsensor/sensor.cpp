
#include "sensor.hpp"

Sensor::Sensor(void)
{

}

Sensor::~Sensor(void)
{

}

sensor_t Sensor::getSensorCore(void) const
{
    return sensorCore;
}

void Sensor::setEnabled(bool enabled)
{
    this->enabled = enabled;
}

bool Sensor::isEnabled(void) const
{
    return enabled;
}
