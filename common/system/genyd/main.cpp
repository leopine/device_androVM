#include <cutils/log.h>

int main(int argc, char**argv)
{
    SLOGI("Coucou");

    while(1) {
        sleep(1000);
    }

    return 0;
}
