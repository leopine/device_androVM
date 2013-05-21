#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/select.h>
#include <sys/errno.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <pthread.h>

#define LOG_TAG "local_gps"
#include <cutils/log.h>
#include <cutils/properties.h>

#define GPS_PORT 22470

#define GPS_STATUS_PROPERTY "genymotion.gps.status"
#define GPS_DISABLED "disabled"
#define GPS_ENABLED "enabled"
#define GPS_DEFAULT_STATUS GPS_DISABLED

#define GPS_LATITUDE "genymotion.gps.latitude"
#define GPS_LONGITUDE "genymotion.gps.longitude"
#define GPS_ALTITUDE "genymotion.gps.altitude"

#define GPS_PRECISION_PROPERTY "genymotion.gps.precision"
#define GPS_DEFAULT_PRECISION "1"

#define GPS_UPDATE_PERIOD 5 /* period in sec between 2 gps fix emission */

static int last_time = 0;

#define STRING_GPS_FIX "$GPGGA,%06d,%02d%02d.%04d,%c,%02d%02d.%04d,%c,1,08,%d,%.1g,M,0.,M,,,*47\n"
#define STRING_GPRMC "$GPRMC,%02d%02d%02d.%02d,A,%02d%02d.%04d,%c,%02d%02d.%04d,%c,%f,%f,%02d%02d%02d,%f,*47\n"

static int wait_for_client(void) {
    int ssocket, main_socket;
    struct sockaddr_in srv_addr;
    long haddr;

    bzero(&srv_addr, sizeof(srv_addr));
    srv_addr.sin_family = AF_INET;
    srv_addr.sin_addr.s_addr = INADDR_ANY;
    srv_addr.sin_port = htons(GPS_PORT);

    if ((ssocket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        SLOGE("Unable to create socket\n");
        exit(-1);
    }

    int yes = 1;
    setsockopt(ssocket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

    if (bind(ssocket, (struct sockaddr *)&srv_addr, sizeof(srv_addr))<0) {
        SLOGE("Unable to bind socket, errno=%d\n", errno);
        exit(-1);
    }

    if (listen(ssocket, 1) < 0) {
        SLOGE("Unable to listen to socket, errno=%d\n", errno);
        exit(-1);
    }

    main_socket = accept(ssocket, NULL, 0);

    if (main_socket < 0) {
        SLOGE("Unable to accept socket for main conection, errno=%d\n", errno);
        exit(-1);
    }

    return main_socket;
}

int main(int argc, char *argv[]) {
    int main_socket = 0;

    char gps_latitude[PROPERTY_VALUE_MAX];
    char gps_longitude[PROPERTY_VALUE_MAX];
    char gps_altitude[PROPERTY_VALUE_MAX];
    char gps_status[PROPERTY_VALUE_MAX];
    char gps_precision[PROPERTY_VALUE_MAX];
    char gps_fix[128];
    char gprmc[128];

    double i_lat=0, i_lng=0, i_alt=0;
    double o_lat, o_lng, o_alt;
    int o_latdeg, o_latmin, o_lngdeg, o_lngmin;
    char o_clat, o_clng;

    // Listen for main connection
    while ((main_socket = wait_for_client())) {

        while (1) {
            sleep(GPS_UPDATE_PERIOD);
            property_get(GPS_STATUS_PROPERTY, gps_status, GPS_DEFAULT_STATUS);
            if (strcmp(gps_status, GPS_ENABLED) == 0) {
                SLOGD("GPS enabled, parsing properties");

                property_get(GPS_LATITUDE, gps_latitude, "0");
                property_get(GPS_LONGITUDE, gps_longitude, "0");
                property_get(GPS_ALTITUDE, gps_altitude, "0");
                i_lat = atof(gps_latitude);
                i_lng = atof(gps_longitude);
                i_alt = atof(gps_altitude);

                if (i_lat<0) {
                    o_lat = -i_lat;
                    o_clat = 'S';
                }
                else {
                    o_lat = i_lat;
                    o_clat = 'N';
                }
                o_latdeg = (int)o_lat;
                o_lat = 60*(o_lat - o_latdeg);
                o_latmin = (int) o_lat;
                o_lat = 10000*(o_lat - o_latmin);

                if (i_lng<0) {
                    o_lng = -i_lng;
                    o_clng = 'W';
                }
                else {
                    o_lng = i_lng;
                    o_clng = 'E';
                }
                o_lngdeg = (int)o_lng;
                o_lng = 60*(o_lng - o_lngdeg);
                o_lngmin = (int) o_lng;
                o_lng = 10000*(o_lng - o_lngmin);

                /* HDOP (horizontal dilution of precision) */
                property_get(GPS_PRECISION_PROPERTY, gps_precision, GPS_DEFAULT_PRECISION);
                int precision = atoi(gps_precision);
                if (precision < 0 || precision > 200) {
                    SLOGE("Invalid precision %s, should be [0..200]");
                    continue;
                }

                struct timeval tv;
                struct tm tm;

                if (gettimeofday(&tv, NULL) == -1) {
                    SLOGE("gettimeofday");
                    continue;
                }

                if (!gmtime_r(&tv.tv_sec, &tm)) {
                    SLOGE("gmtime_r");
                    continue;
                }

                snprintf(gps_fix, sizeof(gps_fix), STRING_GPS_FIX, last_time++,
                         o_latdeg, o_latmin, (int)o_lat, o_clat,
                         o_lngdeg, o_lngmin, (int)o_lng, o_clng,
                         precision,
                         i_alt);

                snprintf(gprmc, sizeof(gprmc), STRING_GPRMC,
                         tm.tm_hour, tm.tm_min, tm.tm_sec, 0,
                         o_latdeg, o_latmin, (int)o_lat, o_clat,
                         o_lngdeg, o_lngmin, (int)o_lng, o_clng,
                         0.0,
                         90.0,
                         tm.tm_year % 100, tm.tm_mon + 1, tm.tm_mday,
                         90.0);

                SLOGD("GGA command : %s", gps_fix);
                SLOGD("RMC command : %s", gprmc);

                if (send(main_socket, gps_fix, strlen(gps_fix), MSG_NOSIGNAL) == -1) {
                    SLOGE("Can't send GGA command");
                    break;
                }

                if (send(main_socket, gprmc, strlen(gprmc), MSG_NOSIGNAL) == -1) {
                    SLOGE("Can't send RMC command");
                    break;
                }
            }
        }
    }

    return 0;
}
