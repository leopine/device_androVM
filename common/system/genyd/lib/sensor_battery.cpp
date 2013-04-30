#include "genymotion.hpp"

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

// Get battery value when full
int Genymotion::batteryFull(char *buff, size_t size)
{
    unsigned long int val = 50000000UL;
    int sz = snprintf(buff, size, "%lu", val);
    return sz;
}

// Get current battery value
int Genymotion::batteryValue(char *buff, size_t size)
{
    unsigned long int val = 47500000UL;
    int sz = snprintf(buff, size, "%lu", val);
    return sz;
}
