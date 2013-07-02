
#include <time.h>
#include <cutils/log.h>

#include "sensor.hpp"

Sensor::Sensor(void)
{
    memset(&baseEvent, 0, sizeof(baseEvent));
}

Sensor::~Sensor(void)
{

}

sensors_event_t *Sensor::getLastEvent(void)
{
    lastEvent.timestamp = getTimestamp();

    return &lastEvent;
}

sensors_event_t *Sensor::getBaseEvent(void)
{
    baseEvent.timestamp = getTimestamp();

    return &baseEvent;
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

int64_t Sensor::getTimestamp(void) const {
    struct timespec t;

    t.tv_sec = 0;
    t.tv_nsec = 0;

    clock_gettime(CLOCK_MONOTONIC, &t);

    return int64_t(t.tv_sec) * 1000000000LL + t.tv_nsec;
}
