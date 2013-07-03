
#include <time.h>
#include <sys/wait.h>
#include <cutils/log.h>

#include "sensor.hpp"

Sensor::Sensor(void)
{
    memset(&baseEvent, 0, sizeof(baseEvent));
    memset(&lastEvent, 0, sizeof(lastEvent));
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

int Sensor::setProperty(const char *property, const char *value)
{
    int status = 1;

    // Store value
    pid_t p_id = fork();
    if (p_id < 0) {
        SLOGE("Unable to fork.");
        return 1;
    } else if (p_id == 0) {
        execl("/system/bin/androVM_setprop",
              "androVM_setprop", property, value, NULL);
        return 0;
    } else {
        // Wait for child process
        wait(&status);
        if (WEXITSTATUS(status) != 0) {
            SLOGD("Setprop process exited  with status %d", WEXITSTATUS(status));
        }
    }
    return status;
}
