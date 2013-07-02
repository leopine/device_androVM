
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

int GenySensors::closeSensor(struct hw_device_t *dev)
{
    return 0;
}

int GenySensors::setDelay(struct sensors_poll_device_t *dev, int handle, int64_t ns)
{
    genySensors.setDelay(handle, ns);
    return 0;
}

int GenySensors::poll(struct sensors_poll_device_t *dev, sensors_event_t *data, int count)
{
    return genySensors.poll(data, count);;
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
    dev->common.close    = &GenySensors::closeSensor;
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
    delay(200000),
    clientSock(-1),
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

int GenySensors::poll(sensors_event_t *data, int count)
{
    int ret;
    int maxfs;
    fd_set readfs;
    struct timeval max_delay;

    max_delay.tv_sec = 0;
    max_delay.tv_usec = delay;

    while (1) {

        FD_ZERO(&readfs);

        if (clientSock != -1) {
            maxfs = clientSock;
            FD_SET(clientSock, &readfs);
        } else {
            maxfs = serverSock;
            FD_SET(serverSock, &readfs);
        }

        ret = select(maxfs + 1, &readfs, NULL, NULL, (clientSock == -1) ? NULL : &max_delay);

        if (ret == -1) {
            return 0;
        }

        if (clientSock == -1 && FD_ISSET(serverSock, &readfs)) {
            acceptNewClient();
        } else if (clientSock != -1 && FD_ISSET(clientSock, &readfs)) {
            return readData(data, count);
        } else {
            return lastData(data, count);
        }
    }

    return 0;
}

void GenySensors::acceptNewClient(void)
{
    clientSock = accept(serverSock, NULL, NULL);

    if (clientSock == -1) {
        SLOGD("Can't accept new client");
    }
}

int GenySensors::readData(sensors_event_t *data, int count)
{
    t_sensor_data rawData[count];
    int eventCount;
    int readSize;
    int i;

    readSize = recv(clientSock, rawData, sizeof(*rawData) * count, 0);

    if (readSize <= 0) {
        ALOGE("Error reading datas, errno=%d", errno);
        close(clientSock);
        clientSock = -1;
        return 0;
    }

    if ((readSize % sizeof(*rawData)) != 0) {
        ALOGD("read() returned %d bytes, not a multiple of %d !", readSize, sizeof(*rawData));
        return 0;
    }

    eventCount = readSize / sizeof(*rawData);
    i = 0;

    while (i < eventCount) {
        if (sensors.find(rawData[i].sensor) == sensors.end()) {
            SLOGI("Unknown sensor type : %lld !", rawData[i].sensor);
            ++i;
            continue;
        }

        sensors[rawData[i].sensor]->generateEvent(&data[i], rawData[i]);

        ++i;
    }

    if (i) {
        return i;
    } else {
        return lastData(data, count);
    }
}

int GenySensors::lastData(sensors_event_t *data, int count)
{
    int i = 0;
    std::map<int, Sensor *>::iterator begin = sensors.begin();
    std::map<int, Sensor *>::iterator end = sensors.end();

    while (begin != end && i < count) {
        Sensor *sensor = begin->second;
        if (sensor->isEnabled()) {
        memcpy(&data[i], sensor->getLastEvent(), sizeof(data[i]));
        } else {
        memcpy(&data[i], sensor->getBaseEvent(), sizeof(data[i]));
        }
        ++begin;
        ++i;
    }

    return i;
}

void GenySensors::setDelay(int handle, int64_t ns)
{
    delay = ns / 1000ULL;
}
