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
#include <cutils/log.h>

#include <linux/input.h>
#include <linux/uinput.h>

#define DEBUG_MT 0

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

    /* We need to save the current state (mouse state) because we must have the
     * right behavior on "MOUSE" movements events
     */
    typedef enum {
	STATE_NO_CLICK = 0,
	STATE_PINCH_TO_ZOOM,
	STATE_CLICKED
    } state;
    state curr_state = STATE_NO_CLICK;

    /* These are specifics to pinch to zoom feature */
    int last_fixed_xpos; /* the coordinate that should be fixed when pinching on this axis */
    int last_fixed_ypos; /* the coordinate that should be fixed when pinching on this axis */
    int centeroffset; /* the offset between the cursor position and each fingers */
    int orientation; /* the current orientation of the device */

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

            pcmd = p1 = p2 = p3 = p4 = NULL;
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
                    int xpos, ypos, r_finger_x, r_finger_y, l_finger_x, l_finger_y, delta = 0;
#if DEBUG_MT
                    SLOGD("MOUSE: State pinch to zoom: orientation:%d x:%s y:%s fixed x:%d y:%d",
                          orientation, p1, p2, last_fixed_xpos, last_fixed_ypos);
#endif /* DEBUG_MT */
                    xpos = atoi(p1);
                    ypos = atoi(p2);

                    // Compute fingers new position
                    switch(orientation) {
                    case 90:
                        /* Mouse up / x decrease should increase delta / fingers spacing */
                        delta = last_fixed_xpos - xpos;
                        l_finger_x = last_fixed_xpos;
                        l_finger_y = last_fixed_ypos + centeroffset - delta;
                        r_finger_x = last_fixed_xpos;
                        r_finger_y = last_fixed_ypos - centeroffset + delta;
                        if (l_finger_y < r_finger_y) {
                            l_finger_y = r_finger_y = last_fixed_ypos;
                        }
                        break;
                    case 180:
                        /* Mouse up / y increase should increase delta / fingers spacing */
                        delta = ypos - last_fixed_ypos;
                        l_finger_x = last_fixed_xpos + centeroffset + delta;
                        l_finger_y = last_fixed_ypos;
                        r_finger_x = last_fixed_xpos - centeroffset - delta;
                        r_finger_y = last_fixed_ypos;
                        if (r_finger_x > l_finger_x) {
                            l_finger_x = r_finger_x = last_fixed_xpos;
                        }
                        break;
                    case 270:
                        /* Mouse up / x increase should increase delta / fingers spacing */
                        delta = xpos - last_fixed_xpos;
                        l_finger_x = last_fixed_xpos;
                        l_finger_y = last_fixed_ypos - centeroffset + delta;
                        r_finger_x = last_fixed_xpos;
                        r_finger_y = last_fixed_ypos + centeroffset - delta;
                        if (l_finger_y > r_finger_y) {
                            l_finger_y = r_finger_y = last_fixed_ypos;
                        }
                        break;
                    case 0:
                    default:
                        /* Mouse up / y decrease should increase delta / fingers spacing */
                        delta = last_fixed_ypos - ypos;
                        l_finger_x = last_fixed_xpos - centeroffset - delta;
                        l_finger_y = last_fixed_ypos;
                        r_finger_x = last_fixed_xpos + centeroffset + delta;
                        r_finger_y = last_fixed_ypos;
                        if (l_finger_x > r_finger_x) {
                            l_finger_x = r_finger_x = last_fixed_xpos;
                        }
                        break;
                    }
#if DEBUG_MT
                    SLOGD("MOUSE: right finger x:%d, y:%d left finger x:%d, y:%d",
                          r_finger_x, r_finger_y, l_finger_x, l_finger_y);
#endif /* DEBUG_MT */

                    memset(&event, 0, sizeof(event));
                    gettimeofday(&event.time, NULL);

                    // First finger
                    event.type = EV_ABS;
                    event.code = ABS_MT_POSITION_X;
                    event.value = l_finger_x;
                    write(uinp_fd, &event, sizeof(event));
                    event.type = EV_ABS;
                    event.code = ABS_MT_POSITION_Y;
                    event.value = l_finger_y;
                    write(uinp_fd, &event, sizeof(event));
                    event.type = EV_SYN;
                    event.code = SYN_MT_REPORT;
                    event.value = 0;
                    write(uinp_fd, &event, sizeof(event));

                    // Second finger
                    event.type = EV_ABS;
                    event.code = ABS_MT_POSITION_X;
                    event.value = r_finger_x;
                    write(uinp_fd, &event, sizeof(event));
                    event.type = EV_ABS;
                    event.code = ABS_MT_POSITION_Y;
                    event.value = r_finger_y;
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
            /**** MOUSE PINCH TO ZOOM PRESSED ****/
            else if (!strcmp(pcmd,"MSPPR")) {
		int xpos, ypos, r_finger_x, r_finger_y, l_finger_x, l_finger_y;
                if (!p1 || !p2 || !p3)
                    continue;
#if DEBUG_MT
                SLOGD("MSPPR:%s:%s:%s", p1, p2, p3);
#endif /* DEBUG_MT */

                xpos = atoi(p1);
                ypos = atoi(p2);
                orientation = atoi(p3);

                /* switch state to pinch to zoom mode */
                curr_state = STATE_PINCH_TO_ZOOM;

                /* Save current X (or Y) fixed value to compute finger spacing in MOUSE event
                   handler, without altering pointer position */
                last_fixed_xpos = xpos;
                last_fixed_ypos = ypos;

                // Compute fingers starting spacing
                centeroffset = DEFAULT_FINGERS_OFFSET;
                // Make sure the centeroffset won't make the right finger position negative
                // because the framework doesn't like that
                if (orientation == 0 || orientation == 180) {
                    if (last_fixed_xpos - centeroffset < 0) {
                        centeroffset = last_fixed_xpos;
                    }
                } else {
                    if (last_fixed_ypos - centeroffset < 0) {
                        centeroffset = last_fixed_ypos;
                    }
                }

                // Compute fingers position
                switch(orientation) {
                case 90:
                    l_finger_x = xpos;
                    l_finger_y = ypos + centeroffset;
                    r_finger_x = xpos;
                    r_finger_y = ypos - centeroffset;
                    break;
                case 180:
                    l_finger_x = xpos + centeroffset;
                    l_finger_y = ypos;
                    r_finger_x = xpos - centeroffset;
                    r_finger_y = ypos;
                    break;
                case 270:
                    l_finger_x = xpos;
                    l_finger_y = ypos - centeroffset;
                    r_finger_x = xpos;
                    r_finger_y = ypos + centeroffset;
                    break;
                case 0:
                default:
                    l_finger_x = xpos - centeroffset;
                    l_finger_y = ypos;
                    r_finger_x = xpos + centeroffset;
                    r_finger_y = ypos;
                    break;
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
                event.value = r_finger_x;
                write(uinp_fd, &event, sizeof(event));
                event.type = EV_ABS;
                event.code = ABS_MT_POSITION_Y;
                event.value = r_finger_y;
                write(uinp_fd, &event, sizeof(event));
                event.type = EV_SYN;
                event.code = SYN_MT_REPORT;
                event.value = 0;
                write(uinp_fd, &event, sizeof(event));

                // Second finger
                event.type = EV_ABS;
                event.code = ABS_MT_POSITION_X;
                event.value = l_finger_x;
                write(uinp_fd, &event, sizeof(event));
                event.type = EV_ABS;
                event.code = ABS_MT_POSITION_Y;
                event.value = l_finger_y;
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
            /**** MOUSE PINCH TO ZOOM RELEASED ****/
            else if (!strcmp(pcmd,"MSPRL")) {
                if (!p1 || !p2)
                    continue;
#if DEBUG_MT
                SLOGD("MSPRL:%s:%s", p1, p2);
#endif /* DEBUG_MT */

                // reset state on release
                curr_state = STATE_NO_CLICK;

                memset(&event, 0, sizeof(event));
                gettimeofday(&event.time, NULL);

                // We just need to send an empty report
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
