
#include <cutils/properties.h>
#include <cutils/sockets.h>
#include <cutils/log.h>

#include "geny_sensors.hpp"

// Sensors availables
#include "sensor_accelerometer.hpp"

// Sensors manager instance
static GenySensors genySensors;

//
// sensors_poll_device_t callbacks
//

int GenySensors::activate(struct sensors_poll_device_t *dev, int handle, int enabled)
{
    genySensors.setSensorStatus(handle, enabled);

    return 0;
}

int GenySensors::close(struct hw_device_t *dev)
{
    return 0;
}

int GenySensors::setDelay(struct sensors_poll_device_t *dev, int handle, int64_t ns)
{
    // genySensors.setDelay(handle, ns);
    return 0;
}

int GenySensors::poll(struct sensors_poll_device_t *dev, sensors_event_t *data, int count)
{
    return 0;
}

//
// sensors_module_t
//

int GenySensors::getSensorsList(struct sensors_module_t *module, struct sensor_t const **list)
{
    *list = genySensors.getList();
    return genySensors.getNum();
}

static int open_sensors(const struct hw_module_t *module, const char *name, struct hw_device_t **device)
{
    return GenySensors::initialize(module, device);
}

static struct hw_module_methods_t sensors_module_methods = {
 open : open_sensors
};

struct sensors_module_t HAL_MODULE_INFO_SYM = {
 common: {
    tag: HARDWARE_MODULE_TAG,
    version_major: 0,
    version_minor: 9,
    id: SENSORS_HARDWARE_MODULE_ID,
    name: "Genymotion Sensor Module",
    author: "Genymobile",
    methods: &sensors_module_methods,
    dso: 0,
    reserved: {},
 },
 get_sensors_list: &GenySensors::getSensorsList
};

//
// Initialization
//

int GenySensors::initialize(const hw_module_t *module, hw_device_t **device)
{
    struct sensors_poll_device_t *dev;

    dev = new sensors_poll_device_t;

    if (!dev) {
        SLOGE("Unable to allocate sensors_poll_device_t");
        return -1;
    }

    memset(dev, 0, sizeof(*dev));

    dev->common.tag      = HARDWARE_DEVICE_TAG;
    dev->common.version  = 0;
    dev->common.module   = const_cast<hw_module_t *>(module);
    dev->common.close    = &GenySensors::close;
    dev->activate        = &GenySensors::activate;
    dev->setDelay        = &GenySensors::setDelay;
    dev->poll            = &GenySensors::poll;

    *device = &dev->common;

    return 0;
}

//
// Sensor manager implementation
//

GenySensors::GenySensors(void) :
    serverSock(-1),
    numSensors(1)
{
    serverSock = socket_inaddr_any_server(22471, SOCK_STREAM);

    if (serverSock < 0) {
        SLOGE("Unable to start listening server: %d", errno);
    }

    sensors[SENSOR_TYPE_ACCELEROMETER] = new AccelerometerSensor();
    sensorList = new type_sensor_t[numSensors];

    sensorList[0] = sensors[SENSOR_TYPE_ACCELEROMETER]->getSensorCore();
}

GenySensors::~GenySensors(void)
{
    delete [] sensorList;

    delete sensors[SENSOR_TYPE_ACCELEROMETER];
}

struct sensor_t *GenySensors::getList(void) {
    return sensorList;
}

int GenySensors::getNum(void) const {
    return numSensors;
}

void GenySensors::setSensorStatus(int handle, bool enabled)
{
    sensors[SENSOR_TYPE_ACCELEROMETER]->setEnabled(enabled);
}

int64_t GenySensors::getTimestamp(void) {
    struct timespec t;

    t.tv_sec = 0;
    t.tv_nsec = 0;

    clock_gettime(CLOCK_MONOTONIC, &t);

    return int64_t(t.tv_sec) * 1000000000LL + t.tv_nsec;
}
