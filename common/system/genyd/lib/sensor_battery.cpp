#include "libgenyd.hpp"
#define NO_PROTOBUF
#include "global.hpp"

#include <cutils/properties.h>

// Plug battery callbacks
void LibGenyd::initBatteryCallbacks()
{
    sensor_callbacks["/sys/class/power_supply"] = &LibGenyd::batteryCallback;

    battery_callbacks["/energy_full"] = &LibGenyd::batteryFull;
    battery_callbacks["/energy_now"] = &LibGenyd::batteryLevel;
    battery_callbacks["/status"] = &LibGenyd::batteryStatus;
    battery_callbacks["/online"] = &LibGenyd::acOnlineStatus;
}

// Search for battery callbacks in file pattern
int LibGenyd::batteryCallback(const char *path, char *buff, size_t size)
{
    std::map<std::string, LibGenyd::t_callback_member>::iterator begin = battery_callbacks.begin();
    std::map<std::string, LibGenyd::t_callback_member>::iterator end = battery_callbacks.end();

    std::string haystack(path);

    while (begin != end) {
        size_t pos = haystack.rfind(begin->first);
        // if haystack ends with
        if (pos != std::string::npos && pos + begin->first.size() == haystack.size()) {
            // Retrieve value forced by callback
            int result = (this->*(begin->second))(buff, size);
            //SLOGD("%s Battery callback: Overloading file %s with content = '%s'",
            //      __FUNCTION__, path, buff);
            return result;
        }
        ++begin;
    }

    return -1;
}

// Static helper method that reads property key in a sane manner
int readPropertyValue(const char *key, char *buff, size_t max_size)
{
    // Read property value
    char property[PROPERTY_VALUE_MAX];
    int len = property_get(key, property, buff);

    if (len >= 0 && len < (int)max_size) {
        // Copy property
        snprintf(buff, len + 1, "%s", property);
        return len;
    }

    return -1;
}


// Get battery value when full
int LibGenyd::batteryFull(char *buff, size_t size)
{
    // Store current value to Genymotion cache
    cacheCurrentValue(BATTERY_FULL, buff);

    if (LibGenyd::isManualMode(BATTERY_MODE)){
        return readPropertyValue(BATTERY_FULL, buff, size);
    } else {
        return -1;
    }
}

// Get current battery level
int LibGenyd::batteryLevel(char *buff, size_t size)
{
    // Store current value to Genymotion cache
    cacheCurrentValue(BATTERY_LEVEL, buff);

    if (LibGenyd::isManualMode(BATTERY_MODE)){
        return readPropertyValue(BATTERY_LEVEL, buff, size);
    } else {
        return -1;
    }
}


// Get current battery status
int LibGenyd::batteryStatus(char *buff, size_t size)
{
    // Store current value to Genymotion cache
    cacheCurrentValue(BATTERY_STATUS, buff);

    if (LibGenyd::isManualMode(BATTERY_MODE)){
        return readPropertyValue(BATTERY_STATUS, buff, size);
    } else {
        return -1;
    }
}

// Get ac online value
int LibGenyd::acOnlineStatus(char *buff, size_t size)
{
    // Store current value to Genymotion cache
    cacheCurrentValue(AC_ONLINE, buff);

    if (LibGenyd::isManualMode(BATTERY_MODE)){
        return readPropertyValue(AC_ONLINE, buff, size);
    } else {
        return -1;
    }
}
