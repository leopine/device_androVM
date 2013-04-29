#ifndef LIB_GENYMOTION_HPP_
#define LIB_GENYMOTION_HPP_

#include <map>
#include <string>

class Genymotion {

public:
    Genymotion(void);
    ~Genymotion(void);

private:
    Genymotion(const Genymotion &);
    Genymotion operator=(const Genymotion &);

    static Genymotion &getInstance(void);

    // Callbacks lists
    typedef int (Genymotion::*t_dispatcher_member)(const char *, char *, size_t);
    std::map<std::string, t_dispatcher_member> callbacks;

    typedef int (Genymotion::*t_callback_member)(char *, size_t);
    std::map<std::string, t_callback_member> battery_callbacks;

public:
    // Overload /proc values with genymotion configuration
    static int getValueFromProc(const char* path, char *buf, size_t size);

private:
    // Global dispatcher
    t_dispatcher_member getCallback(const char *path);

    // Battery callbacks
    // Battery dispatcher
    int batteryCallback(const char *path, char *buff, size_t size);
    // Battery status
    int batteryStatus(char *buff, size_t size);
};

#endif // #define LIB_GENYMOTION_HPP_GENYD_READ_PROC_HPP_
