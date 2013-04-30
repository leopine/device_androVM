#ifndef LIB_GENYMOTION_HPP_
#define LIB_GENYMOTION_HPP_

#include <map>
#include <string>

class Genymotion {

public:
    // Constructor
    Genymotion(void);

private:
    // Destructor
    ~Genymotion(void);

    // Copy constructor
    Genymotion(const Genymotion &);

    // Copy operator
    Genymotion operator=(const Genymotion &);

    // Singleton instance
    static Genymotion instance;

    // Get singleton object
    static Genymotion &getInstance(void);

    // Callbacks lists
    typedef int (Genymotion::*t_dispatcher_member)(const char *, char *, size_t);
    std::map<std::string, t_dispatcher_member> callbacks;

    typedef int (Genymotion::*t_callback_member)(char *, size_t);
    std::map<std::string, t_callback_member> battery_callbacks;

public:
    // Store current value to Genymotion cache
    static void storeCurrentValue(const char *path, const char *buf, const size_t size);

    // Overload /proc values with genymotion configuration
    static int getValueFromProc(const char* path, char *buf, size_t size);

private:
    // Global dispatcher
    t_dispatcher_member getCallback(const char *path);

    // =================
    // Battery callbacks
    // =================

    // Battery dispatcher
    int batteryCallback(const char *path, char *buff, size_t size);

    // Get battery value when full
    int batteryFull(char *buff, size_t size);

    // Get current battery value
    int batteryValue(char *buff, size_t size);
};

#endif // #define LIB_GENYMOTION_HPP_GENYD_READ_PROC_HPP_
