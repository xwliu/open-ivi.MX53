#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <linux/netlink.h>

#define LOG_TAG "Rescan_disk"

#include <cutils/log.h>
#include <fcntl.h>

#define UEVENT_MSG_LEN 4096
struct luther_gliethttp {
    const char *action;
    const char *path;
    const char *subsystem;
    const char *firmware;
    int major;
    int minor;
};
static int open_luther_gliethttp_socket(void);
static void parse_event(const char *msg, struct luther_gliethttp *luther_gliethttp);
char sd_dev[32];
pthread_t tid;
#define THREAD_CTRL_RUN		1
#define THREAD_CTRL_STOP		0
int thread_ctr = THREAD_CTRL_STOP;

int main(int argc, char* argv[])
{
    int device_fd = -1;
    char msg[UEVENT_MSG_LEN+2];
    int n;
    
    device_fd = open_luther_gliethttp_socket();
    SLOGW("device_fd = %p\n", device_fd);

    do {
        while((n = recv(device_fd, msg, UEVENT_MSG_LEN, 0)) > 0) {
            struct luther_gliethttp luther_gliethttp;

            if(n == UEVENT_MSG_LEN) /* overflow -- discard */
                continue;

            msg[n] = '\0';
            msg[n+1] = '\0';

            parse_event(msg, &luther_gliethttp);
        }
    } while(1);
}

static int open_luther_gliethttp_socket(void)
{
    struct sockaddr_nl addr;
    int sz = 64*1024;
    int s;

    memset(&addr, 0, sizeof(addr));
    addr.nl_family = AF_NETLINK;
    addr.nl_pid = getpid();
    addr.nl_groups = 0xffffffff;

    s = socket(PF_NETLINK, SOCK_DGRAM, NETLINK_KOBJECT_UEVENT);
    if (s < 0)
        return -1;

    setsockopt(s, SOL_SOCKET, SO_RCVBUFFORCE, &sz, sizeof(sz));

    if (bind(s, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
        close(s);
        return -1;
    }

    return s;
}

static void *pthread_fdisk_rescan(void *arg)
{
	char buf[128];
	while(1) {
		if(thread_ctr ==  THREAD_CTRL_RUN) {
			sprintf(buf, "/system/bin/busybox fdisk -l /dev%s", sd_dev);
			SLOGW("cmd:%s", buf);
			system(buf);
			usleep(3000000);
		} else {
			break;
		}
	}
    return NULL;
}


static void parse_event(const char *msg, struct luther_gliethttp *luther_gliethttp)
{
    luther_gliethttp->action = "";
    luther_gliethttp->path = "";
    luther_gliethttp->subsystem = "";
    luther_gliethttp->firmware = "";
    luther_gliethttp->major = -1;
    luther_gliethttp->minor = -1;
	int fd = -1;
	char buf[32];
	char name_path[128];
	char *t_dev_tmp = NULL;
    /* currently ignoring SEQNUM */
    while (*msg) {

        SLOGW("%s\n", msg);

        if (!strncmp(msg, "ACTION=", 7)) {
            msg += 7;
            luther_gliethttp->action = msg;
        } else if (!strncmp(msg, "DEVPATH=", 8)) {
            msg += 8;
            luther_gliethttp->path = msg;
        } else if (!strncmp(msg, "SUBSYSTEM=", 10)) {
            msg += 10;
            luther_gliethttp->subsystem = msg;
        } else if (!strncmp(msg, "FIRMWARE=", 9)) {
            msg += 9;
            luther_gliethttp->firmware = msg;
        } else if (!strncmp(msg, "MAJOR=", 6)) {
            msg += 6;
            luther_gliethttp->major = atoi(msg);
        } else if (!strncmp(msg, "MINOR=", 6)) {
            msg += 6;
            luther_gliethttp->minor = atoi(msg);
        }

        /* advance to after the next \0 */
        while(*msg++);
    }
	
	
	if(!strcmp("add", luther_gliethttp->action) && !strcmp("block", luther_gliethttp->subsystem)) {

		strcpy(name_path, "/sys");
		strcpy(name_path + strlen(name_path), luther_gliethttp->path);
		strcpy(name_path + strlen(name_path), "/size");
		
		t_dev_tmp= strstr(luther_gliethttp->path, "/block/sd");
		if(t_dev_tmp == NULL) {
			return;
		}
		SLOGW("Aibing debug path:%s",name_path);
		fd = open(name_path, O_RDONLY);
		if(fd >= 0) {
			read(fd, buf, 32);
			close(fd);
			if (atoi(buf) <= 0)
			{
				memset(sd_dev, 0, sizeof(sd_dev));
				strcpy(sd_dev, t_dev_tmp);
				SLOGW("SD_DEV:%s", sd_dev);
				thread_ctr = THREAD_CTRL_RUN;
				pthread_create(&tid, NULL, pthread_fdisk_rescan, NULL);
			}
		} else {
			if(thread_ctr == THREAD_CTRL_RUN) {
				memset(sd_dev, 0, sizeof(sd_dev));	
				pthread_kill(tid, 0);
				thread_ctr = THREAD_CTRL_STOP;
			}
			SLOGW("Aibing debug open error");
			return;
		}
	}
	if(!strcmp("remove", luther_gliethttp->action) && !strcmp("block", luther_gliethttp->subsystem)) {
		t_dev_tmp= strstr(luther_gliethttp->path, "/block/sd");
		if(!strcmp(sd_dev, t_dev_tmp)) {
			SLOGW("REMOVE dev:%s", t_dev_tmp);
			memset(sd_dev, 0, sizeof(sd_dev));	
			pthread_kill(tid, 0);
			thread_ctr = THREAD_CTRL_STOP;
		}
	}
/*
	if(!strcmp("change", luther_gliethttp->action) && !strcmp("block", luther_gliethttp->subsystem)) {
		t_dev_tmp= strstr(luther_gliethttp->path = msg, "/block/sd");
		if(!strcmp(sd_dev, t_dev_tmp)) {
			SLOGW("changed dev:%s", t_dev_tmp);
			memset(sd_dev, 0, sizeof(sd_dev));	
			pthread_kill(tid, 0);
		}
	}
*/
//    SLOGW("event { '%s', '%s', '%s', '%s', %d, %d }\n",
//                   luther_gliethttp->action, luther_gliethttp->path, luther_gliethttp->subsystem,
//                    luther_gliethttp->firmware, luther_gliethttp->major, luther_gliethttp->minor);
}
