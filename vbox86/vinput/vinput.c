#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <strings.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>

#include <cutils/properties.h>
#include <cutils/sockets.h>

#include <linux/input.h>
#include <linux/uinput.h>

#define DEFAULT_FINGERS_OFFSET 200

int main(int argc, char *argv[]) {
    int uinp_fd;
    struct uinput_user_dev uinp;
    struct input_event event;
    int csocket;
    int i;

    int keyboard_disabled = 0;
    char keyboard_prop[PROPERTY_VALUE_MAX];
    property_get("androVM.keyboard_disable", keyboard_prop, "0");

    typedef enum {
	STATE_NO_CLICK = 0,
	STATE_PINCH_TO_ZOOM,
	STATE_CLICKED
    } state;
    state curr_state = STATE_NO_CLICK;
    int lastxpos, centeroffset;

    if (!strcmp(keyboard_prop, "1")) {
        keyboard_disabled = 1;
    }

    while (1) {
        FILE *f_sock;
#define BUFSIZE 256
        char mbuf[BUFSIZE];

        int ssocket;
        ssocket = socket_inaddr_any_server(22469, SOCK_STREAM);
        if (ssocket < 0) {
            fprintf(stderr, "Unable to start listening server...\n");
            break;
        }
        csocket = accept(ssocket, NULL, NULL);
        if (csocket < 0) {
            fprintf(stderr, "Unable to accept connection...\n");
            close(ssocket);
            break;
        }
        close(ssocket);


        f_sock = fdopen(csocket, "r");
        if (!f_sock) {
            fprintf(stderr, "Unable to have fdsock on socket...\n");
            close(csocket);
            break;
        }

        while (fgets(mbuf, BUFSIZE, f_sock)) {
            char *pcmd, *p1, *p2, *p3, *p4, *pb, *pe;

            pcmd = p1 = p2 = NULL;
            if (pe=strchr(mbuf,'\n'))
                *pe='\0';
            pcmd=pb=mbuf;
            if (pb && (pe=strchr(pb,':'))) {
                *pe='\0';
                p1=pb=++pe;
            }
            if (pb && (pe=strchr(pb,':'))) {
                *pe='\0';
                p2=pb=++pe;
            }
            if (pb && (pe=strchr(pb,':'))) {
                *pe='\0';
                p3=pb=++pe;
            }
            if (pb && (pe=strchr(pb,':'))) {
                *pe='\0';
                p4=pb=++pe;
            }
            if (!pcmd)
                continue;
            if (!strcmp(pcmd,"CONFIG")) {
                if (!p1 || !p2)
                    continue;

                // Create ABS input device
                if (!(uinp_fd = open("/dev/uinput", O_WRONLY|O_NDELAY))) {
                    fprintf(stderr, "Unable to open /dev/uinput !\n");
                    exit(-1);
                }

                memset(&uinp, 0, sizeof(uinp));
                strncpy(uinp.name, "androVM Virtual Input", UINPUT_MAX_NAME_SIZE);
                uinp.id.vendor=0x1234;
                uinp.id.product=1;
                uinp.id.version=1;
                uinp.absmin[ABS_MT_POSITION_X]=0;
                uinp.absmax[ABS_MT_POSITION_X]=atoi(p1);
                uinp.absmin[ABS_MT_POSITION_Y]=0;
                uinp.absmax[ABS_MT_POSITION_Y]=atoi(p2);
                ioctl(uinp_fd, UI_SET_EVBIT, EV_KEY);
                ioctl(uinp_fd, UI_SET_EVBIT, EV_ABS);
                ioctl(uinp_fd, UI_SET_ABSBIT, ABS_MT_POSITION_X);
                ioctl(uinp_fd, UI_SET_ABSBIT, ABS_MT_POSITION_Y);
                ioctl(uinp_fd, UI_SET_ABSBIT, ABS_MT_PRESSURE);
                if (!keyboard_disabled) {
                    for (i=0;i<256;i++)
                        ioctl(uinp_fd, UI_SET_KEYBIT, i);
                }
                ioctl(uinp_fd, UI_SET_KEYBIT, BTN_TOUCH);
                ioctl(uinp_fd, UI_SET_KEYBIT, BTN_LEFT);
                ioctl(uinp_fd, UI_SET_KEYBIT, BTN_TOOL_FINGER);
                ioctl(uinp_fd, UI_SET_EVBIT, EV_REL);
                ioctl(uinp_fd, UI_SET_RELBIT, REL_HWHEEL);
                ioctl(uinp_fd, UI_SET_RELBIT, REL_WHEEL);
                write(uinp_fd, &uinp, sizeof(uinp));
                if (ioctl(uinp_fd, UI_DEV_CREATE)) {
                    fprintf(stderr, "Unable to create ABS uinput device...\n");
                    exit(-1);
                }
            }
            else if (!strcmp(pcmd,"MOUSE")) {
                if (!p1 || !p2)
                    continue;

                if (curr_state == STATE_CLICKED) {
                    memset(&event, 0, sizeof(event));
                    gettimeofday(&event.time, NULL);

                    event.type = EV_ABS;
                    event.code = ABS_MT_POSITION_X;
                    event.value = atoi(p1);
                    write(uinp_fd, &event, sizeof(event));
                    event.type = EV_ABS;
                    event.code = ABS_MT_POSITION_Y;
                    event.value = atoi(p2);
                    write(uinp_fd, &event, sizeof(event));

                    event.type = EV_SYN;
                    event.code = SYN_MT_REPORT;
                    event.value = 0;
                    write(uinp_fd, &event, sizeof(event));

                    event.type = EV_SYN;
                    event.code = SYN_REPORT;
                    event.value = 0;
                    write(uinp_fd, &event, sizeof(event));
                } else if (curr_state == STATE_PINCH_TO_ZOOM) {
                    int xpos, delta = 0;
                    // we are pinching, compute the delta with new X position
                    delta = lastxpos - atoi(p1);

                    memset(&event, 0, sizeof(event));
                    gettimeofday(&event.time, NULL);

                    // First finger
                    event.type = EV_ABS;
                    event.code = ABS_MT_POSITION_X;
                    xpos = lastxpos - centeroffset - delta;
                    // We want to avoid zoom inversion during zoom in
                    if (xpos > lastxpos) {
                        xpos = lastxpos;
                    }
                    event.value = xpos;
                    write(uinp_fd, &event, sizeof(event));
                    event.type = EV_ABS;
                    event.code = ABS_MT_POSITION_Y;
                    event.value = atoi(p2);
                    write(uinp_fd, &event, sizeof(event));
                    event.type = EV_SYN;
                    event.code = SYN_MT_REPORT;
                    event.value = 0;
                    write(uinp_fd, &event, sizeof(event));

                    // Second finger
                    event.type = EV_ABS;
                    event.code = ABS_MT_POSITION_X;
                    xpos = lastxpos + centeroffset + delta;
                    // We want to avoid zoom inversion during zoom in
                    if (xpos < lastxpos) {
                        xpos = lastxpos;
                    }
                    event.value = xpos;
                    write(uinp_fd, &event, sizeof(event));
                    event.type = EV_ABS;
                    event.code = ABS_MT_POSITION_Y;
                    event.value = atoi(p2);
                    write(uinp_fd, &event, sizeof(event));
                    event.type = EV_SYN;
                    event.code = SYN_MT_REPORT;
                    event.value = 0;
                    write(uinp_fd, &event, sizeof(event));

                    event.type = EV_SYN;
                    event.code = SYN_REPORT;
                    event.value = 0;
                    write(uinp_fd, &event, sizeof(event));
                }
            }
            else if (!strcmp(pcmd,"WHEEL")) {
                if (!p1 || !p2 || !p3 || !p4)
                    continue;
                memset(&event, 0, sizeof(event));
                gettimeofday(&event.time, NULL);
                event.type = EV_ABS;
                event.code = ABS_MT_POSITION_X;
                event.value = atoi(p1);
                write(uinp_fd, &event, sizeof(event));
                event.type = EV_ABS;
                event.code = ABS_MT_POSITION_Y;
                event.value = atoi(p2);
                write(uinp_fd, &event, sizeof(event));
                event.type = EV_REL;
                event.code = REL_WHEEL;
                event.value = atoi(p3);
                write(uinp_fd, &event, sizeof(event));
                event.type = EV_REL;
                event.code = REL_HWHEEL;
                event.value = atoi(p4);
                write(uinp_fd, &event, sizeof(event));
                event.type = EV_SYN;
                event.code = SYN_REPORT;
                event.value = 0;
                write(uinp_fd, &event, sizeof(event));
            }
            else if (!strcmp(pcmd,"MSBPR")) {
                if (!p1 || !p2)
                    continue;
                curr_state = STATE_CLICKED;

                memset(&event, 0, sizeof(event));
                gettimeofday(&event.time, NULL);

                event.type = EV_KEY;
                event.code = BTN_TOUCH;
                event.value = 1;
                write(uinp_fd, &event, sizeof(event));
                event.type = EV_ABS;
                event.code = ABS_MT_PRESSURE;
                event.value = 1;
                write(uinp_fd, &event, sizeof(event));

                event.type = EV_ABS;
                event.code = ABS_MT_POSITION_X;
                event.value = atoi(p1);
                write(uinp_fd, &event, sizeof(event));

                event.type = EV_ABS;
                event.code = ABS_MT_POSITION_Y;
                event.value = atoi(p2);
                write(uinp_fd, &event, sizeof(event));

                event.type = EV_SYN;
                event.code = SYN_MT_REPORT;
                event.value = 0;
                write(uinp_fd, &event, sizeof(event));

                event.type = EV_SYN;
                event.code = SYN_REPORT;
                event.value = 0;
                write(uinp_fd, &event, sizeof(event));
            }
            else if (!strcmp(pcmd,"MSBRL")) {
                if (!p1 || !p2)
                    continue;
                // reset state on release
                curr_state = STATE_NO_CLICK;
                memset(&event, 0, sizeof(event));
                gettimeofday(&event.time, NULL);

                event.type = EV_KEY;
                event.code = BTN_TOUCH;
                event.value = 0;
                write(uinp_fd, &event, sizeof(event));
                event.type = EV_ABS;
                event.code = ABS_MT_PRESSURE;
                event.value = 0;
                write(uinp_fd, &event, sizeof(event));

                event.type = EV_SYN;
                event.code = SYN_MT_REPORT;
                event.value = 0;
                write(uinp_fd, &event, sizeof(event));
                event.type = EV_SYN;
                event.code = SYN_REPORT;
                event.value = 0;
                write(uinp_fd, &event, sizeof(event));
            }
            /* MOUSE PINCH TO ZOOM PRESSED */
            else if (!strcmp(pcmd,"MSPPR")) {
                if (!p1 || !p2)
                    continue;
                /* switch state to pinch to zoom mode */
                curr_state = STATE_PINCH_TO_ZOOM;
                /* Save current X value to compute variation in MOUSE event,
                 We don't want mouse movement to modify it or the center of the pinch
                 will be lost */
                lastxpos = atoi(p1);
                // This is default finger spacing/2
                centeroffset = DEFAULT_FINGERS_OFFSET;
                // Make sure the centeroffset won't make the first finger X negative
                if (lastxpos - centeroffset < 0) {
                    centeroffset = lastxpos;
                }

                memset(&event, 0, sizeof(event));
                gettimeofday(&event.time, NULL);

                event.type = EV_KEY;
                event.code = BTN_TOUCH;
                event.value = 1;
                write(uinp_fd, &event, sizeof(event));
                event.type = EV_ABS;
                event.code = ABS_MT_PRESSURE;
                event.value = 1;
                write(uinp_fd, &event, sizeof(event));

                // First finger
                event.type = EV_ABS;
                event.code = ABS_MT_POSITION_X;
                event.value = atoi(p1) - centeroffset;
                write(uinp_fd, &event, sizeof(event));
                event.type = EV_ABS;
                event.code = ABS_MT_POSITION_Y;
                event.value = atoi(p2);
                write(uinp_fd, &event, sizeof(event));
                event.type = EV_SYN;
                event.code = SYN_MT_REPORT;
                event.value = 0;
                write(uinp_fd, &event, sizeof(event));

                // Second finger
                event.type = EV_ABS;
                event.code = ABS_MT_POSITION_X;
                event.value = atoi(p1) + centeroffset;
                write(uinp_fd, &event, sizeof(event));
                event.type = EV_ABS;
                event.code = ABS_MT_POSITION_Y;
                event.value = atoi(p2);
                write(uinp_fd, &event, sizeof(event));
                event.type = EV_SYN;
                event.code = SYN_MT_REPORT;
                event.value = 0;
                write(uinp_fd, &event, sizeof(event));

                event.type = EV_SYN;
                event.code = SYN_REPORT;
                event.value = 0;
                write(uinp_fd, &event, sizeof(event));
            }
            /* MOUSE PINCH TO ZOOM RELEASED */
            else if (!strcmp(pcmd,"MSPRL")) {
                if (!p1 || !p2)
                    continue;
                // reset state on release
                curr_state = STATE_NO_CLICK;

                memset(&event, 0, sizeof(event));
                gettimeofday(&event.time, NULL);

                event.type = EV_KEY;
                event.code = BTN_TOUCH;
                event.value = 0;
                write(uinp_fd, &event, sizeof(event));
                event.type = EV_ABS;
                event.code = ABS_MT_PRESSURE;
                event.value = 0;
                write(uinp_fd, &event, sizeof(event));

                event.type = EV_SYN;
                event.code = SYN_MT_REPORT;
                event.value = 0;
                write(uinp_fd, &event, sizeof(event));
                event.type = EV_SYN;
                event.code = SYN_REPORT;
                event.value = 0;
                write(uinp_fd, &event, sizeof(event));
            }
            else if (!strcmp(pcmd,"KBDPR")) {
                if (!p1 || !p2)
                    continue;
                memset(&event, 0, sizeof(event));
                gettimeofday(&event.time, NULL);
                event.type = EV_KEY;
                event.code = atoi(p1);
                event.value = 1;
                write(uinp_fd, &event, sizeof(event));
                event.type = EV_SYN;
                event.code = SYN_REPORT;
                event.value = 0;
                write(uinp_fd, &event, sizeof(event));
            }
            else if (!strcmp(pcmd,"KBDRL")) {
                if (!p1 || !p2)
                    continue;
                memset(&event, 0, sizeof(event));
                gettimeofday(&event.time, NULL);
                event.type = EV_KEY;
                event.code = atoi(p1);
                event.value = 0;
                write(uinp_fd, &event, sizeof(event));
                event.type = EV_SYN;
                event.code = SYN_REPORT;
                event.value = 0;
                write(uinp_fd, &event, sizeof(event));
            }
        }

        close(csocket);
    }

    return 0;
}
