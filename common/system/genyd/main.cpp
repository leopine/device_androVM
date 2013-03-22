
#include "global.hpp"
#include "genyd.hpp"

int main(int argc, char**argv)
{
    SLOGI("Starting genyd...");

    Genyd deamon;

    // TRY: setprop (used in shell)
    // if(property_set("my.super.value", "good")){
    //     fprintf(stderr, "could not set property\n");
    // }

    if (deamon.isInit()) {
        deamon.run();
    }

    SLOGI("Exiting");

    return 0;
}
