#ifndef LIB_GENYMOTION_HPP_
#define LIB_GENYMOTION_HPP_

class Genymotion {

public:
    Genymotion(void);
    ~Genymotion(void);

private:
    Genymotion(const Genymotion &);
    Genymotion operator=(const Genymotion &);

public:
    // Overload /proc values with genymotion configuration
    static int getValueFromProc(const char* path, char *buf, size_t size);
};

#endif // #define LIB_GENYMOTION_HPP_GENYD_READ_PROC_HPP_
