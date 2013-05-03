
#include <cutils/properties.h>

#include "dispatcher.hpp"
#include "libgenyd.hpp"

void Dispatcher::setBatteryStatus(const Request &request, Reply *reply)
{
    std::string value = request.parameter().value().stringvalue();

    reply->set_type(Reply::None);
    Status *status = reply->mutable_status();
    status->set_code(Status::Ok);

    if (value != "Charging" && value != "Discharging" &&
        value != "Not charging" && value != "Full") {

        SLOGD("Unknown \"%s\" battery status", value.c_str());

        reply->set_type(Reply::Error);
        status->set_code(Status::InvalidRequest);
    } else {
        if (LibGenyd::useRealValue(BATTERY_STATUS)) {
            reply->set_type(Reply::Error);
            status->set_code(Status::GenericError);
        }
        property_set(BATTERY_STATUS, value.c_str());
    }
}

void Dispatcher::getBatteryStatus(const Request &request, Reply *reply)
{
    // Prepare response
    reply->set_type(Reply::Value);
    Status *status = reply->mutable_status();
    status->set_code(Status::Ok);
    Value *value = reply->mutable_value();
    value->set_type(Value::String);

    char property_value[PROPERTY_VALUE_MAX];
    property_get(BATTERY_STATUS CACHE_SUFFIX, property_value, "Unknown");

    if (!LibGenyd::useRealValue(BATTERY_VALUE)) {
        property_get(BATTERY_STATUS, property_value, "Not charging");
    }

    // Set value in response
    value->set_stringvalue(property_value);
}

void Dispatcher::setBatteryValue(const Request &request, Reply *reply)
{
    int batlevel = request.parameter().value().uintvalue();

    Status *status = reply->mutable_status();
    reply->set_type(Reply::None);
    status->set_code(Status::Ok);

    if (batlevel == -1) {
        property_set(BATTERY_VALUE, VALUE_USE_REAL);
        return;
    } else if (batlevel < -1 || batlevel > 100) {
        SLOGE("Invalid battery level %d", batlevel);
        reply->set_type(Reply::Error);
        status->set_code(Status::InvalidRequest);
        return;
    }

    if (LibGenyd::useRealValue(BATTERY_VALUE)) {
        reply->set_type(Reply::None);
        status->set_code(Status::OkWithInformation);
        status->set_description("Battery mode forced to 'manual'");
        SLOGI("Genyd forces manual mode by setting battery value manually");

        // Force battery status if it has not been set yet
        if (batlevel == 100) {
            property_set(BATTERY_STATUS, "Full");
        } else {
            char prop_status[PROPERTY_VALUE_MAX];
            property_get(BATTERY_STATUS CACHE_SUFFIX, prop_status, "Unknown");
            if (strncasecmp(prop_status, "full", 4) == 0) {
                property_set(BATTERY_STATUS, "Not charging");
            }
        }
    }

    // Compute battery voltage
    uint64_t efull = 50000000;
    uint64_t enow = (efull * batlevel) / 100UL;

    // Prepare string values
    char prop_full[PROPERTY_VALUE_MAX];
    char prop_now[PROPERTY_VALUE_MAX];

    snprintf(prop_full, sizeof(prop_full), "%lld", efull);
    snprintf(prop_now, sizeof(prop_now), "%lld", enow);

    property_set(BATTERY_FULL, prop_full);
    property_set(BATTERY_VALUE, prop_now);
}

void Dispatcher::getBatteryValue(const Request &request, Reply *reply)
{
    // Read keys
    char property_full[PROPERTY_VALUE_MAX];
    char property_value[PROPERTY_VALUE_MAX];

    if (LibGenyd::useRealValue(BATTERY_VALUE)) {
        property_get(BATTERY_FULL CACHE_SUFFIX, property_full, "0");
        property_get(BATTERY_VALUE CACHE_SUFFIX, property_value, "0");
    } else {
        property_get(BATTERY_FULL, property_full, "0");
        property_get(BATTERY_VALUE, property_value, "0");
    }

    uint64_t efull = atoll(property_full);
    uint64_t enow = atoll(property_value);

    // Compute battery level
    uint64_t batlevel = efull ? ((enow * ((uint64_t)100)) / efull) : 0;

    // Prepare response
    reply->set_type(Reply::Value);
    Status *status = reply->mutable_status();
    status->set_code(Status::Ok);
    Value *value = reply->mutable_value();
    value->set_type(Value::Uint);

    // Set value in response
    value->set_uintvalue(batlevel);
}

void Dispatcher::isBatteryManual(const Request &request, Reply *reply)
{
    // Prepare response
    reply->set_type(Reply::Value);
    Status *status = reply->mutable_status();
    status->set_code(Status::Ok);
    Value *value = reply->mutable_value();
    value->set_type(Value::Bool);

    // Set value in response
    value->set_boolvalue(!LibGenyd::useRealValue(BATTERY_VALUE));
}
