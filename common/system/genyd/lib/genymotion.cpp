#include "genymotion.hpp"

// Singleton object
Genymotion Genymotion::instance = Genymotion();

// Constructor
Genymotion::Genymotion(void)
{
    // Populate callbacks lists
    callbacks["/sys/class/power_supply"] = &Genymotion::batteryCallback;

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
    SLOGI("Storing system value from path %s: %s", path, buf);
}

// Overload /proc values with genymotion configuration
int Genymotion::getValueFromProc(const char *path, char *buf, size_t size)
{
    SLOGI("Retrieving value from %s", path);

    Genymotion &instance = Genymotion::getInstance();

    Genymotion::t_dispatcher_member callback = instance.getCallback(path);

    if (callback) {
	int result = (instance.*callback)(path, buf, size);
	SLOGI("%s Callback returned %d for key %s with content = %s",
	      __FUNCTION__, result, path, buf);
	return result;
    }
    SLOGI("%s No callback found. Returning", __FUNCTION__);
    return -1;
}

Genymotion::t_dispatcher_member Genymotion::getCallback(const char *path)
{
    std::map<std::string, Genymotion::t_dispatcher_member>::iterator begin = callbacks.begin();
    std::map<std::string, Genymotion::t_dispatcher_member>::iterator end = callbacks.end();

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
	    return (this->*(begin->second))(buff, size);
	}
	++begin;
    }

    return -1;
}

// Get battery value when full
int Genymotion::batteryFull(char *buff, size_t size)
{
    unsigned long int val = 50000000UL;
    int sz = snprintf(buff, size, "%lu\n", val);
    return sz;
}

// Get current battery value
int Genymotion::batteryValue(char *buff, size_t size)
{
    unsigned long int val = 47500000UL;
    int sz = snprintf(buff, size, "%lu\n", val);
    return sz;
}
