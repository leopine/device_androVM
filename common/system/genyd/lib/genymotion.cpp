#include "genymotion.hpp"

// Singleton object
Genymotion Genymotion::instance = Genymotion();

// Constructor
Genymotion::Genymotion(void)
{
    // Populate callbacks links
    initBatteryCallbacks();
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
    SLOGI("Storing   system value from '%s': '%s'", path, buf);
}

// Overload /proc values with genymotion configuration
int Genymotion::getValueFromProc(const char *path, char *buf, size_t size)
{
    SLOGI("Searching system value from '%s': '%s'", path, buf);

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
