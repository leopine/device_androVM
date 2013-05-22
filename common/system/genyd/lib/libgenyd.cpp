
#include <cutils/properties.h>
#include <sys/wait.h>

#include "libgenyd.hpp"
#define NO_PROTOBUF
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

// Set property with external binary androVM_setprop to avoid RO problem
int LibGenyd::setProperty(const char *property, const char *value)
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

// Store current value to Genymotion cache
void LibGenyd::cacheCurrentValue(const char *key,
                                 const char *buff)
{
    char full_key[PROPERTY_KEY_MAX];
    // Don't cache empty value to use default one instead
    if (!buff || strlen(buff) <= 0)
        return;

    // Generate new key
    snprintf(full_key, sizeof(full_key), "%s%s", key, CACHE_SUFFIX);
    LibGenyd::setProperty(full_key, buff);
}

// Check if the /proc path is a fake one then values must be overloaded every time
int LibGenyd::useFakeValue(const char* path, char* buf, size_t size)
{
    // Check if /proc path start with the fake genymotion power supply one
    if (strncmp(path, GENYMOTION_FAKE_POWER_SUPPLY,
        strlen(GENYMOTION_FAKE_POWER_SUPPLY)) == 0) {

        // one fake value mean that we should switch to manual mode
        if (!isManualMode(BATTERY_MODE)) {
            SLOGD("Switching to manual mode and setting default values");
            LibGenyd::setProperty(BATTERY_MODE, MANUAL_MODE);
            LibGenyd::setProperty(BATTERY_LEVEL, "50000000");
            LibGenyd::setProperty(BATTERY_FULL, "50000000");
            LibGenyd::setProperty(BATTERY_STATUS, "Not charging");
            LibGenyd::setProperty(AC_ONLINE, "1");
        }

        // make sure nothing is present in buf or garbage will be cached and reused
        if (buf && size>0) buf[0] = '\0';
        // Use the standard mechanism to read our custom properties with
        // corresponding callbacks
        return LibGenyd::getValueFromProc(path, buf, size);
    }
    return 0;
}

// Overload /proc values with genymotion configuration
int LibGenyd::getValueFromProc(const char *path, char *buf, size_t size)
{
    LibGenyd &instance = LibGenyd::getInstance();

    LibGenyd::t_dispatcher_member sensorCallback = instance.getSensorCallback(path);

    if (sensorCallback) {
        return (instance.*sensorCallback)(path, buf, size);
    }
    SLOGD("%s No callback found. Returning", __FUNCTION__);
    return -1;
}

// Check if value of 'key' should be read from system or from the stored property
bool LibGenyd::isManualMode(const char *key)
{
    char manual[PROPERTY_VALUE_MAX];
    // if value is MANUAL_MODE (default is AUTO_MODE) we must use real value
    property_get(key, manual, AUTO_MODE);
    return !strcmp(manual, MANUAL_MODE);
}

void LibGenyd::setAcOnlineFromStatus(const char *status)
{
    if (strcmp(status, "Discharging") == 0) {
        property_set(AC_ONLINE, "0" /* offline */);
    } else { // Full, Charging or Not Charging
        property_set(AC_ONLINE, "1" /* online */);
    }
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
