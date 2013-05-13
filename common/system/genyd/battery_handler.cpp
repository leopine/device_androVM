
#include <cutils/properties.h>

#include "dispatcher.hpp"
#include "libgenyd.hpp"

void Dispatcher::setDefaultBatteryLevel(void) const {
    char full[PROPERTY_VALUE_MAX];
    char real[PROPERTY_VALUE_MAX];
    char level[PROPERTY_VALUE_MAX];

    // Get cached full battery level, or 50000000
    property_get(BATTERY_FULL CACHE_SUFFIX, full, "50000000");

    // Get cached real battery level, or full
    property_get(BATTERY_LEVEL CACHE_SUFFIX, real, full);

    // Get old force battery level, or real level
    property_get(BATTERY_LEVEL, level, real);
    property_set(BATTERY_FULL, full);
    property_set(BATTERY_LEVEL, level);
}

void Dispatcher::setDefaultBatteryStatus(void) const {
    char real[PROPERTY_VALUE_MAX];
    char status[PROPERTY_VALUE_MAX];

    // Get cached real battery status, or "Not charging"
    property_get(BATTERY_STATUS CACHE_SUFFIX, real, "Not charging");
    // Get old force battery status, or real level
    property_get(BATTERY_STATUS, status, real);

    property_set(BATTERY_STATUS, status);
    // set AC status too
    LibGenyd::setAcOnlineFromStatus(status);
}

void Dispatcher::setBatteryStatus(const Request &request, Reply *reply)
{
    std::string battery_status = request.parameter().value().stringvalue();

    reply->set_type(Reply::None);
    Status *status = reply->mutable_status();
    status->set_code(Status::Ok);

    // Authorized status: "Charging", "Discharging", "Not charging" and "Full"
    if (battery_status != "Charging" && battery_status != "Discharging" &&
        battery_status != "Not charging" && battery_status != "Full") {

        SLOGD("Unknown \"%s\" battery status", battery_status.c_str());

        reply->set_type(Reply::Error);
        status->set_code(Status::InvalidRequest);
        return;
    }

    // If battery mode is AUTO
    if (!LibGenyd::isManualMode(BATTERY_MODE)) {
        // Force battery mode to MANUAL
        property_set(BATTERY_MODE, MANUAL_MODE);

        // Inform the user of mode switching
        reply->set_type(Reply::None);
        status->set_code(Status::OkWithInformation);
        status->set_description("Battery mode forced to 'manual'");
        SLOGI("Genyd forces manual mode by setting battery status manually");

        // Set default battery level
        setDefaultBatteryLevel();
    }

    property_set(BATTERY_STATUS, battery_status.c_str());
    // set AC status too
    LibGenyd::setAcOnlineFromStatus(battery_status.c_str());

    if (battery_status == "Full") {
        char full[PROPERTY_VALUE_MAX];

        // Get cached full battery level, or 50000000
        property_get(BATTERY_FULL CACHE_SUFFIX, full, "50000000");
        property_set(BATTERY_FULL, full);
        property_set(BATTERY_LEVEL, full);
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

    char battery_status[PROPERTY_VALUE_MAX];

    if (LibGenyd::isManualMode(BATTERY_MODE)) {
        property_get(BATTERY_STATUS, battery_status, "Unknown");
    } else {
        property_get(BATTERY_STATUS CACHE_SUFFIX, battery_status, "Unknown");
    }

    // Set value in response
    value->set_stringvalue(battery_status);
}

void Dispatcher::setBatteryLevel(const Request &request, Reply *reply)
{
    int batlevel = request.parameter().value().intvalue();

    Status *status = reply->mutable_status();
    reply->set_type(Reply::None);
    status->set_code(Status::Ok);

    // Battery level must be between 0 and 100
    if (batlevel < 0 || batlevel > 100) {
        SLOGE("Invalid battery level %d", batlevel);
        reply->set_type(Reply::Error);
        status->set_code(Status::InvalidRequest);
        return;
    }

    // If battery mode is AUTO
    if (!LibGenyd::isManualMode(BATTERY_MODE)) {
        // Force battery mode to MANUAL
        property_set(BATTERY_MODE, MANUAL_MODE);

        // Inform the user of mode switching
        reply->set_type(Reply::None);
        status->set_code(Status::OkWithInformation);
        status->set_description("Battery mode forced to 'manual'");

        // Set default battery status
        setDefaultBatteryStatus();

        SLOGI("Genyd forces manual mode by setting battery value manually");
    }

    // Compute battery voltage
    uint64_t efull = 50000000UL;
    uint64_t ratio = efull / 100UL;
    uint64_t enow = batlevel * ratio;

    // Prepare string values
    char prop_full[PROPERTY_VALUE_MAX];
    char prop_now[PROPERTY_VALUE_MAX];

    snprintf(prop_full, sizeof(prop_full), "%lld", efull);
    snprintf(prop_now, sizeof(prop_now), "%lld", enow);

    property_set(BATTERY_FULL, prop_full);
    property_set(BATTERY_LEVEL, prop_now);

    // Make sure status is not "Full" if battery level is not 100%
    char bat_status[PROPERTY_VALUE_MAX];
    property_get(BATTERY_STATUS, bat_status, "Unknown");
    if (batlevel < 100 && (strcmp(bat_status, "Full") == 0)) {
        property_set(BATTERY_STATUS, "Charging");
    }
}

void Dispatcher::getBatteryLevel(const Request &request, Reply *reply)
{
    char full[PROPERTY_VALUE_MAX];
    char level[PROPERTY_VALUE_MAX];

    if (LibGenyd::isManualMode(BATTERY_MODE)) {
        // Read battery full level, or 50000000
        property_get(BATTERY_FULL, full, "50000000");
        // Read battery full level, or full
        property_get(BATTERY_LEVEL, level, full);
    } else {
        // Read battery full level, or 50000000
        property_get(BATTERY_FULL CACHE_SUFFIX, full, "50000000");
        // Read battery full level, or full
        property_get(BATTERY_LEVEL CACHE_SUFFIX, level, full);
    }

    uint64_t efull = atoll(full);
    uint64_t enow = atoll(level);
    uint64_t batlevel = (efull) ? ((enow * 100UL) / efull) : 100UL;

    // Prepare response
    reply->set_type(Reply::Value);
    Status *status = reply->mutable_status();
    status->set_code(Status::Ok);
    Value *value = reply->mutable_value();
    value->set_type(Value::Int);

    // Set value in response
    value->set_intvalue(batlevel);
}

void Dispatcher::setBatteryMode(const Request &request, Reply *reply) {

    bool automode = !request.parameter().value().boolvalue();

    reply->set_type(Reply::None);
    Status *status = reply->mutable_status();
    status->set_code(Status::Ok);

    // If automode, then set the battery mode to AUTO
    // Else, set the battery mode to MANUAL, and set default level/status
    if (automode) {
        property_set(BATTERY_MODE, AUTO_MODE);
    } else {
        property_set(BATTERY_MODE, MANUAL_MODE);

        // Set default battery level
        setDefaultBatteryLevel();

        // Set default battery status
        setDefaultBatteryStatus();
    }
}

void Dispatcher::isBatteryManual(const Request &request, Reply *reply)
{
    // Prepare response
    reply->set_type(Reply::Value);
    Status *status = reply->mutable_status();
    status->set_code(Status::Ok);
    Value *value = reply->mutable_value();
    value->set_type(Value::Bool);

    value->set_boolvalue(LibGenyd::isManualMode(BATTERY_MODE));
}
