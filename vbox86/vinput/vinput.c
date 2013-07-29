#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <strings.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#include <cutils/properties.h>
#include <cutils/sockets.h>
#include <cutils/log.h>

#include <linux/input.h>
#include <linux/uinput.h>

#define DEBUG_MT 0
/* horizontal offset between each fingers and the cursor */
#define PINCH_TO_ZOOM_FINGERS_OFFSET 200
#define ROTATION_FINGERS_OFFSET      50
#define PINCH_FACTOR                 0.5

/* If you modify this, make sure to report modifications in player event_manager.hpp */
#define MULTITOUCH_MODE_ZOOM         1
#define MULTITOUCH_MODE_ROTATION     2

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
        MOUSE_STATE_NO_CLICK = 0,
        MOUSE_STATE_CLICKED,
        MOUSE_STATE_PINCH_TO_ZOOM,
        MOUSE_STATE_ROTATION
    } mouse_state_t;
    mouse_state_t mouse_state = MOUSE_STATE_NO_CLICK;

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
            memset(&event, 0, sizeof(event));
            gettimeofday(&event.time, NULL);

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

                if (mouse_state == MOUSE_STATE_CLICKED) {
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
                } else if (mouse_state == MOUSE_STATE_ROTATION || mouse_state == MOUSE_STATE_PINCH_TO_ZOOM) {
                    int xpos, ypos, r_finger_x, r_finger_y, l_finger_x, l_finger_y, delta = 0;
#if DEBUG_MT
                    SLOGD("MOUSE: multitouch mode %d orientation:%d x:%s y:%s saved x:%d saved y:%d",
                          mouse_state, orientation, p1, p2, last_fixed_xpos, last_fixed_ypos);
#endif /* DEBUG_MT */
                    xpos = atoi(p1);
                    ypos = atoi(p2);

                    // Compute fingers new position
                    if (mouse_state == MOUSE_STATE_PINCH_TO_ZOOM) {
                        switch(orientation) {
                        case 90:
                            delta = (ypos - last_fixed_ypos) * PINCH_FACTOR;
                            l_finger_x = xpos;
                            l_finger_y = last_fixed_ypos + centeroffset - delta;
                            r_finger_x = xpos;
                            r_finger_y = last_fixed_ypos - centeroffset + delta;
                            if (l_finger_y < r_finger_y) {
                                l_finger_y = last_fixed_ypos;
                                /* Google earth doesn't like when fingers are merged */
                                r_finger_y = l_finger_y + 1;
                            }
                            break;
                        case 180:
                            delta = (xpos - last_fixed_xpos) * PINCH_FACTOR;
                            l_finger_x = last_fixed_xpos + centeroffset + delta;
                            l_finger_y = ypos;
                            r_finger_x = last_fixed_xpos - centeroffset - delta;
                            r_finger_y = ypos;
                            if (r_finger_x > l_finger_x) {
                                r_finger_x = last_fixed_xpos;
                                /* Google earth doesn't like when fingers are merged */
                                l_finger_x = r_finger_x + 1;
                            }
                            break;
                        case 270:
                            delta = (last_fixed_ypos - ypos) * PINCH_FACTOR;
                            l_finger_x = xpos;
                            l_finger_y = last_fixed_ypos - centeroffset + delta;
                            r_finger_x = xpos;
                            r_finger_y = last_fixed_ypos + centeroffset - delta;
                            if (l_finger_y >= r_finger_y) {
                                l_finger_y = last_fixed_ypos;
                                /* Google earth doesn't like when fingers are merged */
                                r_finger_x = l_finger_y + 1;
                            }
                            break;
                        case 0:
                        default:
                            delta = (last_fixed_xpos - xpos) * PINCH_FACTOR;
                            l_finger_x = last_fixed_xpos - centeroffset - delta;
                            l_finger_y = ypos;
                            r_finger_x = last_fixed_xpos + centeroffset + delta;
                            r_finger_y = ypos;
                            if (l_finger_x >= r_finger_x) {
                                l_finger_x = last_fixed_xpos;
                                /* Google earth doesn't like when fingers are merged */
                                r_finger_x = l_finger_x + 1;
                            }
                            break;
                        }
                    } else if (mouse_state == MOUSE_STATE_ROTATION) {
                        int degree_angle, xdelta, ydelta;

                        switch(orientation) {
                        case 90:
                            degree_angle = last_fixed_ypos - ypos;
                            xdelta = centeroffset/*radius*/ * sin(degree_angle * M_PI / 180.0);
                            ydelta = centeroffset/*radius*/ * cos(degree_angle * M_PI / 180.0);
                            l_finger_x = last_fixed_xpos + (int)xdelta;
                            l_finger_y = last_fixed_ypos + (int)ydelta;
                            r_finger_x = last_fixed_xpos - (int)xdelta;
                            r_finger_y = last_fixed_ypos - (int)ydelta;
                            break;
                        case 180:
                            degree_angle = xpos - last_fixed_xpos;
                            xdelta = centeroffset/*radius*/ * cos(degree_angle * M_PI / 180.0);
                            ydelta = centeroffset/*radius*/ * sin(degree_angle * M_PI / 180.0);
                            l_finger_x = last_fixed_xpos - (int)xdelta;
                            l_finger_y = last_fixed_ypos + (int)ydelta;
                            r_finger_x = last_fixed_xpos + (int)xdelta;
                            r_finger_y = last_fixed_ypos - (int)ydelta;
                            break;
                        case 270:
                            degree_angle = ypos - last_fixed_ypos;
                            xdelta = centeroffset/*radius*/ * sin(degree_angle * M_PI / 180.0);
                            ydelta = centeroffset/*radius*/ * cos(degree_angle * M_PI / 180.0);
                            l_finger_x = last_fixed_xpos - (int)xdelta;
                            l_finger_y = last_fixed_ypos - (int)ydelta;
                            r_finger_x = last_fixed_xpos + (int)xdelta;
                            r_finger_y = last_fixed_ypos + (int)ydelta;
                            break;
                        case 0:
                        default:
                            degree_angle = last_fixed_xpos - xpos;
                            xdelta = centeroffset/*radius*/ * cos(degree_angle * M_PI / 180.0);
                            ydelta = centeroffset/*radius*/ * sin(degree_angle * M_PI / 180.0);
                            l_finger_x = last_fixed_xpos - (int)xdelta;
                            l_finger_y = last_fixed_ypos + (int)ydelta;
                            r_finger_x = last_fixed_xpos + (int)xdelta;
                            r_finger_y = last_fixed_ypos - (int)ydelta;
                            break;
                        }
                    } else {
                        /* should never happen */
                        continue;
                    }
#if DEBUG_MT
                    SLOGD("MOUSE: right finger x:%d, y:%d left finger x:%d, y:%d, delta:%d",
                          r_finger_x, r_finger_y, l_finger_x, l_finger_y, delta);
#endif /* DEBUG_MT */

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
                mouse_state = MOUSE_STATE_CLICKED;

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
                // reset mouse state on release
                mouse_state = MOUSE_STATE_NO_CLICK;

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
            /**** MOUSE MULTITOUCH PRESSED ****/
            else if (!strcmp(pcmd,"MSMTPR")) {
		int xpos, ypos, r_finger_x, r_finger_y, l_finger_x, l_finger_y, mode;
                if (!p1 || !p2 || !p3 || !p4)
                    continue;
#if DEBUG_MT
                SLOGD("MSMTPR:%s:%s:%s:%s", p1, p2, p3, p4);
#endif /* DEBUG_MT */

                xpos = atoi(p1);
                ypos = atoi(p2);
                orientation = atoi(p3);
                mode = atoi(p4);

                /* Save current X and Y values to compute finger spacing in MOUSE event
                   handler, without altering pointer position */
                last_fixed_xpos = xpos;
                last_fixed_ypos = ypos;

                /* switch mouse state according to selected mode */
                switch (mode) {
                case MULTITOUCH_MODE_ZOOM:
                    mouse_state = MOUSE_STATE_PINCH_TO_ZOOM;
                    centeroffset = PINCH_TO_ZOOM_FINGERS_OFFSET;
                    break;
                case MULTITOUCH_MODE_ROTATION:
                    mouse_state = MOUSE_STATE_ROTATION;
                    centeroffset = ROTATION_FINGERS_OFFSET;
                    break;
                default:
                    /* ignore unknown mode */
                    continue;
                }

                // Make sure the centeroffset won't make the finger go out of the screen when rotated
                if (last_fixed_xpos - centeroffset < 0) {
                    centeroffset = last_fixed_xpos;
                }
                if (last_fixed_ypos - centeroffset < 0) {
                    centeroffset = last_fixed_ypos;
                }

                // Compute fingers horizontal position
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
            /**** MOUSE MULTITOUCH RELEASED ****/
            else if (!strcmp(pcmd,"MSMTRL")) {
                if (!p1 || !p2)
                    continue;
#if DEBUG_MT
                SLOGD("MSMTRL:%s:%s", p1, p2);
#endif /* DEBUG_MT */

                // reset mouse state on release
                mouse_state = MOUSE_STATE_NO_CLICK;

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
