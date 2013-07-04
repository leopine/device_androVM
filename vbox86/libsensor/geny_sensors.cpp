
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
    (void)dev;

    genySensors.setSensorStatus(handle, enabled);

    return 0;
}

int GenySensors::closeSensor(struct hw_device_t *dev)
{
    if (dev) {
        free(dev);
    }

    return 0;
}

int GenySensors::setDelay(struct sensors_poll_device_t *dev, int handle, int64_t ns)
{
    (void)dev;

    genySensors.setDelay(handle, ns);
    return 0;
}

int GenySensors::poll(struct sensors_poll_device_t *dev, sensors_event_t *data, int count)
{
    (void)dev;

    return genySensors.poll(data, count);
}

//
// sensors_module_t
//

int GenySensors::getSensorsList(struct sensors_module_t *module, struct sensor_t const **list)
{
    (void)module;

    *list = genySensors.getList();
    return genySensors.getNum();
}

static int open_sensors(const struct hw_module_t *module, const char *name, struct hw_device_t **device)
{
    (void)name;

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
    delay(DEFAULT_DELAY),
    serverSock(-1),
    numSensors(1)
{
    memset(&clientsSocks, 0,sizeof(clientsSocks));

    serverSock = socket_inaddr_any_server(LIBSENSOR_PORT, SOCK_STREAM);

    if (serverSock < 0) {
        SLOGE("Unable to start listening server.");
        return;
    }

    sensors[SENSOR_TYPE_ACCELEROMETER] = new AccelerometerSensor();
    sensorList = new sensor_t[numSensors];

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
    int maxfd;
    fd_set readfs;
    struct timeval max_delay;

    max_delay.tv_sec = 0;
    max_delay.tv_usec = delay;

    while (1) {
        maxfd = serverSock;

        FD_ZERO(&readfs);

        FD_SET(serverSock, &readfs);

        // Add connected sockets to the list of sockets to watch
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clientsSocks[i] > 0) {
                FD_SET(clientsSocks[i], &readfs);
                // make sure we stored the biggest fd
                if (clientsSocks[i] > maxfd) {
                    maxfd = clientsSocks[i];
                }
            }
        }

        ret = select(maxfd + 1, &readfs, NULL, NULL, &max_delay);
        // SLOGD("select() returns %d", ret);

        if (ret < 0) {
            if (maxfd == serverSock) {
                close(serverSock);
                SLOGE("Server closed connection, exiting. (errno=%d)", errno);
                exit(EXIT_FAILURE);
            } else {
                // Something's wrong, disconnect every clients but keep server cnx
                for (int c = 0; c < MAX_CLIENTS; c++) {
                    if (clientsSocks[c] > 0) {
                        close(clientsSocks[c]);
                        clientsSocks[c] = 0;
                    }
                }
                SLOGE("Select fail, disconnect all clients (errno=%d)", errno);
                continue;
            }
        }

        if (FD_ISSET(serverSock, &readfs)) {
            acceptNewClient();
	}

        int events_read = 0;
        for (int c = 0; c < MAX_CLIENTS; c++) {
            if (FD_ISSET(clientsSocks[c], &readfs)) {
                events_read += readData(&clientsSocks[c],
                                      &data[events_read],/* first empty slot */
                                      count - events_read /* space left */);
            }
            if (count == events_read) {
                // Stop events read, we have read enough
                break;
            }
        }
        // if no client write data, we should lastRead
        if (events_read == 0) {
            return useLastData(data, count);
        }

        // return read events number
        return events_read;
    }

    return 0;
}

int GenySensors::acceptNewClient(void)
{
    int j = 0;
    int clientSock = accept(serverSock, NULL, NULL);
    if (clientSock == -1) {
        SLOGD("Client connection refused (errno=%d)", errno);
        return -1;
    }

    // Pick a slot in the client sockets list
    for (j = 0; j < MAX_CLIENTS; j++) {
        if (clientsSocks[j] == 0) {
            clientsSocks[j] = clientSock;
            SLOGD("Client connection accepted (%d)", clientSock);
            break;
        }
    }
    // No space left in the client sockets table
    if (j >= MAX_CLIENTS) {
        SLOGE("Too many clients, connection refused");
        return -1;
    }
    return 0;
}

int GenySensors::readData(int *socket_p, sensors_event_t *data, int count)
{
    t_sensor_data rawData[count];
    int eventsToRead, eventsRead = 0;
    int readSize;
    int i = 0;

    readSize = recv(*socket_p, rawData, sizeof(*rawData) * count, 0);

    if (readSize <= 0) {
        //SLOGD("Closing client connection (%d)", errno);
        close(*socket_p);
        *socket_p = 0;
        return 0;
    }

    if ((readSize % sizeof(*rawData)) != 0) {
        SLOGD("read() returned %d bytes, not a multiple of %d !", readSize, sizeof(*rawData));
        return 0;
    }

    eventsToRead = readSize / sizeof(*rawData);

    for (i = 0, eventsRead = 0 ; i < eventsToRead ; ++i) {
        if (sensors.find(rawData[i].sensor) == sensors.end()) {
            SLOGI("Unknown sensor type : %lld !", rawData[i].sensor);
            continue;
        }
        // Write event in the sensor
        sensors[rawData[i].sensor]->generateEvent(&data[eventsRead], rawData[i]);
        eventsRead++;
    }

    return eventsRead;
}

int GenySensors::useLastData(sensors_event_t *data, int count)
{
    int i = 0;
    std::map<int, Sensor *>::iterator begin = sensors.begin();
    std::map<int, Sensor *>::iterator end = sensors.end();

    for ( ; begin != end && i < count ; ++begin, ++i) {
        Sensor *sensor = begin->second;
        if (sensor->isEnabled()) {
            memcpy(&data[i], sensor->getLastEvent(), sizeof(data[i]));
        } else {
            memcpy(&data[i], sensor->getBaseEvent(), sizeof(data[i]));
        }
    }

    return i;
}

void GenySensors::setDelay(int handle, int64_t ns)
{
    delay = ns / 1000ULL;
}
