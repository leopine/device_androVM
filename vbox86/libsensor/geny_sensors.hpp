#ifndef GENY_SENSORS_HPP
#define GENY_SENSORS_HPP

#include <hardware/hardware.h>
#include <hardware/sensors.h>
#include <map>

typedef struct sensor_t type_sensor_t;

class Sensor;

class GenySensors {

public:
    GenySensors(void);
    ~GenySensors(void);

private:
    GenySensors(const GenySensors &);
    GenySensors operator=(const GenySensors &);

    int delay;
    int clientSock;
    int serverSock;
    int numSensors;
    struct sensor_t *sensorList;
    std::map<int, Sensor *> sensors;

public:
    static int activate(struct sensors_poll_device_t *dev, int handle, int enabled);
    static int closeSensor(struct hw_device_t *dev);
    static int setDelay(struct sensors_poll_device_t *dev, int handle, int64_t ns);
    static int poll(struct sensors_poll_device_t *dev, sensors_event_t *data, int count);
    static int getSensorsList(struct sensors_module_t *module, struct sensor_t const **list);
    static int initialize(const hw_module_t *module, hw_device_t **device);

private:
    // Get sensors list
    struct sensor_t *getList(void);
    // Get number of declared sensors
    int getNum(void) const;
    // Set the status for a given sensor
    void setSensorStatus(int handle, bool enabled);
    // Wait for events
    int poll(sensors_event_t *data, int count);
    // Connect a new client
    void acceptNewClient(void);
    // Read sensor data from socket
    int readData(sensors_event_t *data, int count);
    // Return last sensor data
    int lastData(sensors_event_t *data, int count);
    // Set the delay for a given sensor
    void setDelay(int handle, int64_t ns);

};

#endif
