#ifndef GPS_VBOX86_H_
#define GPS_VBOX86_H_

typedef struct {
    char buff[128];
    char *p;
    char *end;
    int init;
} Token;

#define MAX_NMEA_TOKENS  16

typedef struct {
    int count;
    Token tokens[MAX_NMEA_TOKENS];
} NmeaTokenizer;

#define  NMEA_MAX_SIZE  83

typedef struct {
    int pos;
    int overflow;
    int utc_year;
    int utc_mon;
    int utc_day;
    int utc_diff;
    GpsLocation fix;
    gps_location_callback callback;
    char in[NMEA_MAX_SIZE + 1];
} NmeaReader;

typedef struct s_all_tokens {
    Token time;
    Token latitude;
    Token latitudeHemi;
    Token longitude;
    Token fixStatus;
    Token longitudeHemi;
    Token accuracy;
    Token altitude;
    Token altitudeUnits;
    Token speed;
    Token bearing;
    Token date;
} t_all_tokens;

/* commands sent to the gps thread */
enum {
    CMD_QUIT = 0,
    CMD_START = 1,
    CMD_STOP = 2
};


typedef struct {
    int init;
    int fd;
    GpsCallbacks callbacks;
    pthread_t thread;
    int control[2];
} GpsState;

#endif
