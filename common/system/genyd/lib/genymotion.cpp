
#include "genymotion.hpp"

static Genymotion __instance;

Genymotion &Genymotion::getInstance(void)
{
    return __instance;
}

// cTor
Genymotion::Genymotion(void)
{
    callbacks["/sys/class/power_supply"] = &Genymotion::batteryCallback;

    battery_callbacks["/health"] = &Genymotion::batteryStatus;
}

// dTor
Genymotion::~Genymotion(void)
{

}

// Overload /proc values with genymotion configuration
int Genymotion::getValueFromProc(const char *path, char *buf, size_t size)
{
    Genymotion &instance = Genymotion::getInstance();

    Genymotion::t_dispatcher_member callback = instance.getCallback(path);

    if (callback)
	return (instance.*callback)(path, buf, size);
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
	if (pos != std::string::npos && pos + begin->first.size() == haystack.size())
	    return (this->*(begin->second))(buff, size);
	++begin;
    }

    return -1;
}

int Genymotion::batteryStatus(char *buff, size_t size)
{
    return -1;
}
