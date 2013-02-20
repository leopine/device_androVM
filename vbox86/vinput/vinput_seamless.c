#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <strings.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <cutils/properties.h>
#include <cutils/log.h>

#include <linux/input.h>
#include <linux/uinput.h>

#define LOG_TAG "vinput-seamless"

#define PS2_DEVICE_NAME "ImExPS/2 Generic Explorer Mouse"
#define MOUSE_INTEGRATION_DEVICE_NAME "VirtualBox mouse integration"

int main(int argc, char *argv[]) {
    int uinp_fd;
    struct uinput_user_dev uinp;
    struct input_event event;
    int csocket;
    int i;
    int fd_ps2=-1;
    int fd_seamless=-1;
    int fd_max=-1;
    int fd_last_move=-1;
    char input_dev_name[128];
    int uinp_fd_ps2=-1;
    int uinp_fd_seamless=-1;

    for (i=0;1;i++) {
        int lfd;
        char name[256];

        sprintf(input_dev_name, "/dev/input/event%d", i);
        lfd = open(input_dev_name, O_RDONLY);
        if (lfd<0)
            break;
        if (ioctl(lfd, EVIOCGNAME(sizeof(name)), name) < 0) {
            close(lfd);
            continue;
        }
        if (!strcmp(name, PS2_DEVICE_NAME)) {
            fd_ps2 = lfd;
            if (fd_ps2>fd_max)
                fd_max = fd_ps2;
            ALOGE("found PS2 device");
            continue;
        }
        if (!strcmp(name, MOUSE_INTEGRATION_DEVICE_NAME)) {
            fd_seamless = lfd;
            if (fd_seamless>fd_max)
                fd_max = fd_seamless;
            ALOGE("found seamless mouse device");
            continue;
        }
    }

    if ((fd_ps2<0) && (fd_seamless<0)) {
        ALOGE("no device found");
        exit(-1);
    }

    if (fd_ps2>=0) {
        struct uinput_user_dev uinp;

        if (!(uinp_fd_ps2 = open("/dev/uinput", O_WRONLY|O_NDELAY))) {
            ALOGE("Unable to open /dev/uinput for PS2!");
            exit(-1);
        }

        memset(&uinp, 0, sizeof(uinp));
        strncpy(uinp.name, "androVM via VirtualBox PS/2 mouse", UINPUT_MAX_NAME_SIZE);
        uinp.id.vendor=0x1234;
        uinp.id.product=1;
        uinp.id.version=2;
        ioctl(uinp_fd_ps2, UI_SET_EVBIT, EV_SYN);
        ioctl(uinp_fd_ps2, UI_SET_EVBIT, EV_KEY);
        ioctl(uinp_fd_ps2, UI_SET_KEYBIT, BTN_LEFT);
        ioctl(uinp_fd_ps2, UI_SET_KEYBIT, BTN_RIGHT);
        ioctl(uinp_fd_ps2, UI_SET_KEYBIT, BTN_MIDDLE);
        ioctl(uinp_fd_ps2, UI_SET_KEYBIT, BTN_SIDE);
        ioctl(uinp_fd_ps2, UI_SET_KEYBIT, BTN_EXTRA);
        ioctl(uinp_fd_ps2, UI_SET_EVBIT, EV_REL);
        ioctl(uinp_fd_ps2, UI_SET_RELBIT, REL_X);
        ioctl(uinp_fd_ps2, UI_SET_RELBIT, REL_Y);
        ioctl(uinp_fd_ps2, UI_SET_RELBIT, REL_HWHEEL);
        ioctl(uinp_fd_ps2, UI_SET_RELBIT, REL_WHEEL);
        if (write(uinp_fd_ps2, &uinp, sizeof(uinp))!=sizeof(uinp)) {
            ALOGE("Error writing init data for uinput PS2");
            exit(-1);
        }
        if (ioctl(uinp_fd_ps2, UI_DEV_CREATE)) {
            ALOGE("Unable to create PS2 uinput device...");
            exit(-1);
        }
    }

    if (fd_seamless>=0) {
        struct uinput_user_dev uinp;

        if (!(uinp_fd_seamless = open("/dev/uinput", O_WRONLY|O_NDELAY))) {
            ALOGE("Unable to open /dev/uinput for seamless!");
            exit(-1);
        }

        memset(&uinp, 0, sizeof(uinp));
        strncpy(uinp.name, "androVM via VirtualBox seamless mouse", UINPUT_MAX_NAME_SIZE);
        uinp.id.vendor=0x1234;
        uinp.id.product=1;
        uinp.id.version=3;
        uinp.absmin[0]=0;
        uinp.absmax[0]=65535;
        uinp.absmin[1]=0;
        uinp.absmax[1]=65535;
        ioctl(uinp_fd_seamless, UI_SET_EVBIT, EV_SYN);
        ioctl(uinp_fd_seamless, UI_SET_EVBIT, EV_KEY);
        ioctl(uinp_fd_seamless, UI_SET_KEYBIT, BTN_LEFT);
        ioctl(uinp_fd_seamless, UI_SET_EVBIT, EV_ABS);
        ioctl(uinp_fd_seamless, UI_SET_ABSBIT, ABS_X);
        ioctl(uinp_fd_seamless, UI_SET_ABSBIT, ABS_Y);
        if (write(uinp_fd_seamless, &uinp, sizeof(uinp))!=sizeof(uinp)) {
            ALOGE("Error writing init data for uinput seamless");
            exit(-1);
        }
        if (ioctl(uinp_fd_seamless, UI_DEV_CREATE)) {
            ALOGE("Unable to create seamless uinput device...");
            exit(-1);
        }
    }


    while (1) {
        fd_set rfds;

        FD_ZERO(&rfds);
        if (fd_ps2>=0)
            FD_SET(fd_ps2, &rfds);
        if (fd_seamless>=0)
            FD_SET(fd_seamless, &rfds);

        if (select(fd_max+1, &rfds, NULL, NULL, NULL)>0) {
            struct input_event ev_read;

            if(FD_ISSET(fd_ps2, &rfds)) {
                if (read(fd_ps2, &ev_read, sizeof(ev_read))!=sizeof(ev_read)) {
                    ALOGE("Error reading data from 'PS2' input device");
                    exit(-1);
	        }
                if ((ev_read.type == EV_KEY) && (ev_read.code == BTN_LEFT) && (fd_last_move == fd_seamless)) {
                    struct input_event ev_sync;

                    write(uinp_fd_seamless, &ev_read, sizeof(ev_read));
                    ev_sync.type = EV_SYN;
                    ev_sync.code = SYN_REPORT;
                    ev_sync.value = 0;
                    write(uinp_fd_seamless, &ev_sync, sizeof(ev_sync));
	        } 

                else
                    write(uinp_fd_ps2, &ev_read, sizeof(ev_read));
                if ((ev_read.type == EV_REL) && ((ev_read.code == REL_X) || (ev_read.code == REL_Y)))
                    fd_last_move = fd_ps2;
	    } 
            if(FD_ISSET(fd_seamless, &rfds)) {
                if (read(fd_seamless, &ev_read, sizeof(ev_read))!=sizeof(ev_read)) {
                    ALOGE("Error reading data from 'Seamless' input device");
                    exit(-1);
	        }
                write(uinp_fd_seamless, &ev_read, sizeof(ev_read));
                if (ev_read.type == EV_ABS)
                    fd_last_move = fd_seamless;
	    }
	}
    }
    return 0;    
}
