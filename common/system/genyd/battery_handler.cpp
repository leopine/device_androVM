
#include <cutils/properties.h>

#include "dispatcher.hpp"

void Dispatcher::setBatteryStatus(const Request &request, Reply *reply)
{
    std::string value = request.parameter().value().stringvalue();

    if (property_set(BATTERY_STATUS, value.c_str())) {
	SLOGE("Can't set property");
    } else {
	reply->set_type(Reply::None);
    }
}
