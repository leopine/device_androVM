#include "genymotion.hpp"
#include <cutils/properties.h>

// Plug battery callbacks
void Genymotion::initBatteryCallbacks()
{
    sensor_callbacks["/sys/class/power_supply"] = &Genymotion::batteryCallback;

    battery_callbacks["/energy_full"] = &Genymotion::batteryFull;
    battery_callbacks["/energy_now"] = &Genymotion::batteryValue;
}

// Search for battery callbacks in file pattern
int Genymotion::batteryCallback(const char *path, char *buff, size_t size)
{
    std::map<std::string, Genymotion::t_callback_member>::iterator begin = battery_callbacks.begin();
    std::map<std::string, Genymotion::t_callback_member>::iterator end = battery_callbacks.end();

    std::string haystack(path);

    while (begin != end) {
	size_t pos = haystack.rfind(begin->first);
	// if haystack ends with
	if (pos != std::string::npos && pos + begin->first.size() == haystack.size()) {
	    // Store current value to Genymotion cache
	    storeCurrentValue(path, buff, size);
	    // Retrieve value forced by callback
	    int result = (this->*(begin->second))(buff, size);
	    SLOGI("%s Battery callback: Overloading file %s with content = '%s'",
		  __FUNCTION__, path, buff);
	    return result;
	}
	++begin;
    }

    return -1;
}

// Static helper method that reads property key in a sane manner
int readPropertyValueOrDefault(const char *key, char *buff, size_t max_size)
{
    // Read property value
    char property[PROPERTY_VALUE_MAX];
    int length = property_get(key, property, NULL);
    if (length == 0) {
	SLOGE("%s: No property %s. Let use default value '%s'",
	      __FUNCTION__, key, buff);
	return -1;
    } else if (length > (int)max_size) {
	SLOGE("%s: Unable to fill '%s' in %d chars max. Let use default value '%s'",
	      __FUNCTION__, property, max_size, buff);
	return -1;
    }
    // Copy battery Value
    strncpy(buff, property, length);
    return length;
}


// Get battery value when full
int Genymotion::batteryFull(char *buff, size_t size)
{
    return readPropertyValueOrDefault("geny.bat.full", buff, size);
}

// Get current battery value
int Genymotion::batteryValue(char *buff, size_t size)
{
    return readPropertyValueOrDefault("geny.bat.value", buff, size);
}
