// to use getprop and setprop
#include <cutils/properties.h>

#include "global.hpp"
#include "genyd.hpp"

int main(int argc, char**argv)
{
    SLOGI("Starting genyd...");

    Genyd deamon;

    // TRY: setprop (used in shell)
    if(property_set("my.super.value", "good")){
        fprintf(stderr, "could not set property\n");
    }

    // TRY: getprop (used in shell)
    char my_prop[PROPERTY_VALUE_MAX];
    property_get("my.super.value", my_prop, "");
    if (strlen(my_prop)>0) {
        SLOGI(my_prop);
    }

    if (deamon.isInit()) {
        deamon.run();
    }

    SLOGI("Exiting");

    return 0;
}
