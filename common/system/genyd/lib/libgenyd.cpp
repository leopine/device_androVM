
#include <cutils/properties.h>
#include <sys/wait.h>

#include "libgenyd.hpp"
#define __NO_PROTO
#include "global.hpp"

// Singleton object
LibGenyd LibGenyd::instance = LibGenyd();

// Constructor
LibGenyd::LibGenyd(void)
{
    // Populate callbacks links
    initBatteryCallbacks();
}

// Destructor
LibGenyd::~LibGenyd(void)
{
}

// Get singleton object
LibGenyd& LibGenyd::getInstance(void)
{
    return instance;
}

// Store current value to Genymotion cache
void LibGenyd::cacheCurrentValue(const char *key,
                                 const char *buff)
{
    char full_key[PROPERTY_KEY_MAX];

    // Generate new key
    snprintf(full_key, sizeof(full_key), "%s%s", key, CACHE_SUFFIX);

    // Store value
    pid_t p_id = fork();
    if (p_id < 0) {
        SLOGE("Unable to fork.");
        return;
    } else if (p_id == 0) {
        execl("/system/bin/androVM_setprop",
              "androVM_setprop", full_key, buff, NULL);
        return;
    } else {
        // Wait for child process
        int status = 0;
        wait(&status);
        SLOGD("Setprop process exited  with status %d", WEXITSTATUS(status));
    }
}

// Check if the /proc path is a fake one an should be overloaded completely
int LibGenyd::useFakeValue(const char* path, char* buf, size_t size)
{
    // Check if /proc path start with the fake genymotion power supply one
    if (strncmp(path, GENYMOTION_FAKE_POWER_SUPPLY,
        strlen(GENYMOTION_FAKE_POWER_SUPPLY)) == 0) {
        SLOGD("Path value must be overloaded: '%s'", path);

        // one fake value mean that we should switch to manual mode
        SLOGD("Switching to manual mode");
        property_set(BATTERY_MODE, MANUAL_MODE);

        // Use the standard mechanism then to read our custom properties with
        // corresponding callbacks
        return LibGenyd::getValueFromProc(path, buf, size);
    }
    return 0;
}

// Overload /proc values with genymotion configuration
int LibGenyd::getValueFromProc(const char *path, char *buf, size_t size)
{
    SLOGD("Searching system value from '%s': '%s'", path, buf);

    LibGenyd &instance = LibGenyd::getInstance();

    LibGenyd::t_dispatcher_member sensorCallback = instance.getSensorCallback(path);

    if (sensorCallback) {
        return (instance.*sensorCallback)(path, buf, size);
    }
    SLOGD("%s No callback found. Returning", __FUNCTION__);
    return -1;
}

// Check if value of 'key' should be read from from system or from the stored property
bool LibGenyd::isManualMode(const char *key)
{
    char manual[PROPERTY_VALUE_MAX];
    // if value is MANUAL_MODE (default is AUTO_MODE) we must use real value
    property_get(key, manual, AUTO_MODE);
    SLOGD("Forced value for [%s]: \"%s\"", key, manual);
    return !strcmp(manual, MANUAL_MODE);
}

LibGenyd::t_dispatcher_member LibGenyd::getSensorCallback(const char *path)
{
    std::map<std::string, LibGenyd::t_dispatcher_member>::iterator begin = sensor_callbacks.begin();
    std::map<std::string, LibGenyd::t_dispatcher_member>::iterator end = sensor_callbacks.end();

    std::string haystack(path);

    while (begin != end) {
        // if haystack starts with
        if (haystack.find(begin->first) == 0)
            return begin->second;
        ++begin;
    }

    return NULL;
}
