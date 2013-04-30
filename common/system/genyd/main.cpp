
#include "global.hpp"
#include "genyd.hpp"

int main(int argc, char**argv)
{
    SLOGI("Starting genyd");

    Genyd daemon;

    if (daemon.isInit()) {
        daemon.run();
    }

    SLOGI("Exiting");

    return 0;
}
