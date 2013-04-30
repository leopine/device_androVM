#include "genymotion.hpp"

// Singleton object
Genymotion Genymotion::instance = Genymotion();

// Constructor
Genymotion::Genymotion(void)
{
    // Populate callbacks lists
    sensor_callbacks["/sys/class/power_supply"] = &Genymotion::batteryCallback;

    battery_callbacks["/energy_full"] = &Genymotion::batteryFull;
    battery_callbacks["/energy_now"] = &Genymotion::batteryValue;
}

// Destructor
Genymotion::~Genymotion(void)
{
}

// Get singleton object
Genymotion &Genymotion::getInstance(void)
{
    return instance;
}

// Store current value to Genymotion cache
void Genymotion::storeCurrentValue(const char *path, const char *buf, const size_t size)
{
    (void)size;
    SLOGI("Storing system value from path %s: %s", path, buf);
}

// Overload /proc values with genymotion configuration
int Genymotion::getValueFromProc(const char *path, char *buf, size_t size)
{
    SLOGI("Reading forced value from %s. Current = '%s'", path, buf);

    Genymotion &instance = Genymotion::getInstance();

    Genymotion::t_dispatcher_member sensorCallback = instance.getSensorCallback(path);

    if (sensorCallback) {
	return (instance.*sensorCallback)(path, buf, size);
    }
    SLOGI("%s No callback found. Returning", __FUNCTION__);
    return -1;
}

Genymotion::t_dispatcher_member Genymotion::getSensorCallback(const char *path)
{
    std::map<std::string, Genymotion::t_dispatcher_member>::iterator begin = sensor_callbacks.begin();
    std::map<std::string, Genymotion::t_dispatcher_member>::iterator end = sensor_callbacks.end();

    std::string haystack(path);

    while (begin != end) {
	// if haystack starts with
	if (haystack.find(begin->first) == 0)
	    return begin->second;
	++begin;
    }

    return NULL;
}

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
