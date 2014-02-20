/*
 *   Copyright (C) 2011 Freescale Semiconductor, Inc. All Rights Reserved.
 */
/*
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *	http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#define LOG_TAG "RIL"
#include <utils/Log.h>

#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/poll.h>
#include <linux/netlink.h>

#include <signal.h>
#include <unistd.h>
#include "runtime.h"
#include "private/android_filesystem_config.h"
#include "cutils/log.h"

int current_modem_type = UNKNOWN_MODEM;

#define FAKE_PORT "/dev/ttyFAKEPort"
/* Rild need a fake port to pass continue init job,
 * return a fake port make it runable.
 * Or the system will enter 15s in early suspend.
 */
typedef enum{
	UC_NEED_MODESWITCH,
	UC_NONEED_MODESWITCH,
}USB_COMPOSITE_MODEM;

struct modem_3g_device {
	const char *idVendor;
	const char *idProduct;
	const char *deviceport;	/* sending AT command */
	const char *dataport;	/* sending 3g data */
	const char *name;
	const int   type;
	const int   model:UNKNOWN_MODEL;
	USB_COMPOSITE_MODEM needswitch;
};

#define PATH_SIZE 1024
#define ARRAY_SIZE(a) (sizeof(a)/sizeof(a[0]))
static const char *USB_DIR_BASE = "/sys/class/usb_device/";

static struct modem_3g_device modem_3g_device_table[] = {
	{
		.name		= "Huawei-E261-WCDMA",
		.idVendor	= "12d1",
		.idProduct	= "1446",
		.deviceport	= "/dev/ttyUSB2",
		.dataport	= "/dev/ttyUSB0",
		.type		= HUAWEI_MODEM,
		.model 		= WCDMA_MODEL,
		.needswitch = UC_NEED_MODESWITCH,		
	},	
	{
		.name		= "Huawei-E302S-TDSCDMA",
		.idVendor	= "12d1",
		.idProduct	= "1d50",
		.deviceport	= "/dev/ttyUSB2",
		.dataport	= "/dev/ttyUSB2",
		.type		= HUAWEI_MODEM,
		.model 		= TD_SCDMA_MODEL,
		.needswitch = UC_NEED_MODESWITCH,		
	},	
	{
		.name		= "Huawei-E369_WCDMA",
		.idVendor	= "12d1",
		.idProduct	= "1505",
		.deviceport	= "/dev/ttyUSB2",
		.dataport	= "/dev/ttyUSB0",
		.type		= HUAWEI_MODEM,
		.model 		= WCDMA_MODEL,
		.needswitch = UC_NEED_MODESWITCH,		
	},	
	{
		.name		= "ZTE-MF627-MF110_WCDMA",
		.idVendor	= "19d2",
		.idProduct	= "2000",
		.deviceport	= "/dev/ttyUSB2",
		.dataport	= "/dev/ttyUSB0",
		.type		= UNKNOWN_MODEM,
		.model 		= WCDMA_MODEL,
		.needswitch = UC_NEED_MODESWITCH,		
	},	
	{
		.name		= "ZTE-AC591-EVDO",
		.idVendor	= "19d2",
		.idProduct	= "0026",
		.deviceport	= "/dev/ttyUSB2",
		.dataport	= "/dev/ttyUSB0",
		.type		= ZTE_MODEM,
		.model 		= CDMA_MODEL,
		.needswitch = UC_NEED_MODESWITCH,		
	},
///////////////////////////////////////////////////////////////////////////////////////////////////
	{
		.name		= "Huawei-EM770",
		.idVendor	= "12d1",
		.idProduct	= "1001",
		.deviceport	= "/dev/ttyUSB0",
		.dataport	= "/dev/ttyUSB0",
		.type		= HUAWEI_MODEM,
		.needswitch = UC_NONEED_MODESWITCH,
	},

	{
		.name		= "Huawei-E261",
		.idVendor	= "12d1",
		.idProduct	= "1436",
		.deviceport	= "/dev/ttyUSB2",
		.dataport	= "/dev/ttyUSB0",
		.type		= HUAWEI_MODEM,
		.needswitch = UC_NONEED_MODESWITCH,		
	},	
	{
		.name		= "Huawei-EC199-EVDO",
		.idVendor	= "12d1",
		.idProduct	= "140C",
		.deviceport	= "/dev/ttyUSB3",
		.dataport	= "/dev/ttyUSB0",
		.type		= HUAWEI_MODEM,
		.model 		= CDMA_MODEL,
		.needswitch = UC_NONEED_MODESWITCH,		
	},	
	{
		.name		= "Huawei-E369-WCDMA",
		.idVendor	= "12d1",
		.idProduct	= "1506",
		.deviceport	= "/dev/ttyUSB1",
		.dataport	= "/dev/ttyUSB0",
		.type		= HUAWEI_MODEM,
		.model 		= WCDMA_MODEL,
		.needswitch = UC_NONEED_MODESWITCH,		
	},	
	{
		.name		= "ZTE-MF110-WCDMA",
		.idVendor	= "19d2",
		.idProduct	= "0016",
		.deviceport	= "/dev/ttyUSB1",
		.dataport	= "/dev/ttyUSB2",
		.type		= ZTE_MODEM,
		.model 		= WCDMA_MODEL,
		.needswitch = UC_NONEED_MODESWITCH,		
	},		
	{
		.name		= "Huawei-EM770W",
		.idVendor	= "12d1",
		.idProduct	= "1404",
		.deviceport	= "/dev/ttyUSB2",
		.dataport	= "/dev/ttyUSB0",
		.type		= HUAWEI_MODEM,
		.needswitch = UC_NONEED_MODESWITCH,		
	},
	{
		.name		= "Huawei-E180",
		.idVendor	= "12d1",
		.idProduct	= "1003",
		.deviceport	= "/dev/ttyUSB1",
		.dataport	= "/dev/ttyUSB0",
		.type		= HUAWEI_MODEM,
		.needswitch = UC_NONEED_MODESWITCH,		
	},
	{
		.name		= "Huawei-EM750",
		.idVendor	= "12d1",
		.idProduct	= "1413",
		.deviceport	= "/dev/ttyUSB3",
		.dataport	= "/dev/ttyUSB0",
		.type		= HUAWEI_MODEM,
		.needswitch = UC_NONEED_MODESWITCH,		
	},
	{
		.name		= "InnoComm-Amazon1",
		.idVendor	= "1519",
		.idProduct	= "1001",
		.deviceport	= "/dev/ttyACM3",
		.dataport	= "/dev/ttyACM0",
		.type		= AMAZON_MODEM,
		.needswitch = UC_NONEED_MODESWITCH,		
	},
	{
		.name		= "InnoComm-Amazon1",
		.idVendor	= "1519",
		.idProduct	= "0020",
		.deviceport	= "/dev/ttyACM3",
		.dataport	= "/dev/ttyACM0",
		.type		= AMAZON_MODEM,
		.needswitch = UC_NONEED_MODESWITCH,		
	},
	{
		.name		= "ZTE-AC591-EVDO",
		.idVendor	= "19d2",
		.idProduct	= "0152",
		.deviceport	= "/dev/ttyUSB2",
		.dataport	= "/dev/ttyUSB0",
		.type		= ZTE_MODEM,
		.model 		= CDMA_MODEL,
		.needswitch = UC_NONEED_MODESWITCH,		
	},
	{
		.name		= "THINKWELL-CDMA",
		.idVendor	= "19f5",
		.idProduct	= "9013",
	    .deviceport = "/dev/ttyUSB2",
		.dataport	= "/dev/ttyUSB3",
		.type		= ZTE_MODEM,
		.needswitch = UC_NONEED_MODESWITCH, 	
	}

};

/* -------------------------------------------------------------- */

#define DEBUG_UEVENT 0
#define UEVENT_PARAMS_MAX 32

enum uevent_action { action_add, action_remove, action_change };

struct uevent {
    char *path;
    enum uevent_action action;
    char *subsystem;
    char *param[UEVENT_PARAMS_MAX];
    unsigned int seqnum;
};

static void dump_uevent(struct uevent *event);

int readfile(char *path, char *content, size_t size)
{
	int ret;
	FILE *f;
	f = fopen(path, "r");
	if (f == NULL)
		return -1;

	ret = fread(content, 1, size, f);
	fclose(f);
	return ret;
}

int is_device_equal(struct modem_3g_device *device,
		     const char *idv, const char *idp)
{
	long pvid = 0xffff, ppid = 0xffff;
	long t_vid, t_pid;
	if (device == NULL)
		return 0;
	t_vid = strtol(device->idVendor, NULL, 16);
	t_pid = strtol(device->idProduct, NULL, 16);
	pvid = strtol(idv, NULL, 16);
	ppid = strtol(idp, NULL, 16);

	return (t_vid == pvid && t_pid == ppid);
}

#define UM_Conf_File_Name "/data/3g/3g.conf"
int copyConfFile(char *filename)
{
	int fdt = -1;
	int fds = -1;
	int size = -1;
	unsigned char buf[16];
	fds = open(filename,O_RDONLY);
	if(fds==-1)
	{
		
		LOGE("Open Source File Failed!%s[%x]\n",filename,fds);
		return -1;
	}
	else
	{
		LOGE("Found Config file!%s[%x]\n",filename,fds);


	}

	
	fdt = open(UM_Conf_File_Name, O_WRONLY | O_CREAT | O_TRUNC, 00666);
	if(fdt==-1)
	{
		LOGE("Open dist File Failed!%s[%x]\n",filename,fdt);
		close(fds);
		return -1;
	}


	while(size)
	{
		size= read(fds,buf,16);
		if(size==-1)
		{
		        close(fdt);
			close(fds);		
			return -1;
		}
		else
		{
			if(size>0)
				write(fdt,buf,size);
		}

	}

        close(fdt);
	close(fds);		
	return 0;
}	

void doSwichMode(struct modem_3g_device * device)
{
	int argc ,ret = 0;
	char configPath[256];
	unsigned char buf[64];
	int fd;
	int retryCount = 30; // retry  3S for waiting device ready


	sprintf(configPath, "/system/etc/conf3g/%s_%s.conf",device->idVendor,device->idProduct);

	LOGE("switch config file %s [ret] %d\n",configPath,ret);

	copyConfFile(configPath);	

	usleep(1000*1000);

	property_set("ctl.start", "usb_modeswitch");

	
#if 1
	for(;retryCount;retryCount--)
	{
	fd = open (device->deviceport, O_RDWR);
	if (fd < 0)
	{
		LOGE(".");	
	}
	else
	{
		LOGE("Found device ");
		break;
	}
	usleep(100000);
	}

	close(fd);
#endif
	
		
}


struct modem_3g_device *
find_devices_in_table(const char *idvendor, const char *idproduct)
{
	int i;
	int size = ARRAY_SIZE(modem_3g_device_table);
	struct modem_3g_device *device;

	for (i = 0; i < size; i++) {
		device = &modem_3g_device_table[i];

		if (is_device_equal(device, idvendor, idproduct)) {
			LOGE("Runtime 3G port found matched device with "
			     "Name:%s idVendor:%s idProduct:%s",
			     device->name, device->idVendor, device->idProduct);
			if( UC_NEED_MODESWITCH ==device->needswitch )
			{
				LOGE("Modem need switch mode \n");
				doSwichMode(device);
				return device;

			}
			return device;
		}
	}

	return NULL;
}

struct modem_3g_device *find_matched_device(void)
{
	struct dirent *dent;
	DIR *usbdir;
	struct modem_3g_device *device = NULL;
	char *path, *path2;
	char idvendor[64];
	char idproduct[64];
	int ret, i;

	path = malloc(PATH_SIZE);
	path2 = malloc(PATH_SIZE);
	if (!path || !path2)
		return NULL;

	usbdir = opendir(USB_DIR_BASE);
	if (usbdir == NULL) {
		free(path);
		free(path2);
		return NULL;
	}

	memset(path, 0, PATH_SIZE);
	memset(path2, 0, PATH_SIZE);

	while ((dent = readdir(usbdir)) != NULL) {
		if (strcmp(dent->d_name, ".") == 0
		    || strcmp(dent->d_name, "..") == 0)
			continue;
		memset(idvendor, 0, sizeof(idvendor));
		memset(idproduct, 0, sizeof(idproduct));
		path = strcpy(path, USB_DIR_BASE);
		path = strcat(path, dent->d_name);
		strcpy(path2, path);
		path = strcat(path, "/device/idVendor");
		path2 = strcat(path2, "/device/idProduct");

		ret = readfile(path, idvendor, 4);
		if (ret <= 0)
			continue;
		ret = readfile(path2, idproduct, 4);
		if (ret <= 0)
			continue;
		device = find_devices_in_table(idvendor, idproduct);
		if (device != NULL)
			goto out;
	}

	if (device == NULL)
		LOGE("Runtime 3G can't find supported modem");
out:
	closedir(usbdir);
	free(path);
	free(path2);

	return device;
}


const char *runtime_3g_port_device(void)
{
	struct modem_3g_device *device;

	device = find_matched_device();
	if (device == NULL)
		return FAKE_PORT;

	/* Set gobal modem type. */
	current_modem_type = device->type;

	LOGE("Current modem type = %d", current_modem_type);

	return device->deviceport;
}

const char *runtime_3g_port_data(void)
{
	struct modem_3g_device *device;

	device = find_matched_device();
	if (device == NULL)
		return FAKE_PORT;
	return device->dataport;
}

int runtime_3g_modem_model(void)
{
	struct modem_3g_device *device;

	device = find_matched_device();
	if (device == NULL)
		return UNKNOWN_MODEL;
	return device->model;
}

static void free_uevent(struct uevent *event)
{
    int i;
    free(event->path);
    free(event->subsystem);
    for (i = 0; i < UEVENT_PARAMS_MAX; i++) {
	    if (!event->param[i])
		    break;
	    free(event->param[i]);
    }
    free(event);
}

static int dispatch_uevent(struct uevent *event)
{
	/* if it's a usb tty event in our table. make the rild reboot. */
	int i;
	int ret;
	for (i = 0; i < UEVENT_PARAMS_MAX; i++) {
		if (!event->param[i])
			break;
		if (strncmp(event->param[i], "PRODUCT=", 8) == 0) {
			char vbuf[5], pbuf[5];
			ret = sscanf(event->param[i],
				     "PRODUCT=%4s/%4s/", vbuf, pbuf);
			LOGE("event->param[i] %s, event->action: %d\n",event->param[i], event->action);
			if (ret < 0)
				return -1;
			if (find_devices_in_table(vbuf, pbuf)){
					
				alarm(1);
			/* Restart in 1 second, since USB usually have
			 * many devices, this avoid rild restart too
			 * many times. */
			}
		}
	}
	return 0;
}

int process_uevent_message(int sock)
{
	char buffer[64 * 1024];
	char *s = buffer, *p;
	char *end;
	int count, param_idx = 0, ret;
	struct uevent *event;
	count = recv(sock, buffer, sizeof(buffer), 0);
	if (count < 0) {
		LOGE("Error receiving uevent (%s)", strerror(errno));
		return -errno;
	}
	event = malloc(sizeof(struct uevent));
	if (!event) {
		LOGE("Error allcating memroy (%s)", strerror(errno));
		return -errno;
	}
	memset(event, 0, sizeof(struct uevent));

	end = s + count;

	for (p = s; *p != '@'; p++)
		;
	p++;
	event->path = strdup(p);
	s += strlen(s) + 1;
	LOGE("UEVENT %s\n",s);
	while (s < end) {
		if (!strncmp(s, "ACTION=", strlen("ACTION="))) {
			char *a = s + strlen("ACTION=");
			if (!strcmp(a, "add"))
				event->action = action_add;
			else if (!strcmp(a, "change"))
				event->action = action_change;
			else if (!strcmp(a, "remove"))
				event->action = action_remove;
		} else if (!strncmp(s, "SEQNUM=", strlen("SEQNUM=")))
			event->seqnum = atoi(s + strlen("SEQNUM="));
		else if (!strncmp(s, "SUBSYSTEM=", strlen("SUBSYSTEM=")))
			event->subsystem = strdup(s + strlen("SUBSYSTEM="));
		else
			event->param[param_idx++] = strdup(s);
		s += strlen(s) + 1;
	}

	ret = dispatch_uevent(event);
#if DEBUG_UEVENT
	dump_uevent(event);
#endif
	free_uevent(event);
	return ret;
}

static void dump_uevent(struct uevent *event)
{
    int i;

    LOGD("[UEVENT] Sq: %u S: %s A: %d P: %s",
	      event->seqnum, event->subsystem, event->action, event->path);
    for (i = 0; i < UEVENT_PARAMS_MAX; i++) {
	    if (!event->param[i])
		    break;
	    LOGD("%s", event->param[i]);
    }
}

void restart_rild(int p)
{
	LOGE("3G Modem changed,RILD will restart...");
	exit(-1);
}

void *usb_tty_monitor_thread(void *arg)
{
	struct sockaddr_nl nladdr;
	struct pollfd pollfds[2];
	int uevent_sock;
	int ret, max = 0;
	int uevent_sz = 64 * 1024;
	int timeout = -1;
	struct sigaction timeoutsigact;

	LOGE("3G modem monitor thread is start");

	timeoutsigact.sa_handler = restart_rild;
	sigemptyset(&timeoutsigact.sa_mask);
	sigaddset(&timeoutsigact.sa_mask, SIGALRM);
	sigaction(SIGALRM, &timeoutsigact, 0);

	memset(&nladdr, 0, sizeof(nladdr));
	nladdr.nl_family = AF_NETLINK;
	nladdr.nl_pid = getpid();
	nladdr.nl_groups = 0xffffffff;

	uevent_sock = socket(PF_NETLINK, SOCK_DGRAM, NETLINK_KOBJECT_UEVENT);
	if (uevent_sock < 0) {
		LOGE(" Netlink socket faild, usb monitor exiting...");
		return NULL;
	}

	if (setsockopt(uevent_sock, SOL_SOCKET, SO_RCVBUFFORCE, &uevent_sz,
		       sizeof(uevent_sz)) < 0) {
		LOGE("Unable to set uevent socket options: %s", strerror(errno));
		return NULL;
	}

	if (bind(uevent_sock, (struct sockaddr *) &nladdr, sizeof(nladdr)) < 0) {
		   LOGE("Unable to bind uevent socket: %s", strerror(errno));
		   return NULL;
	}
	pollfds[0].fd = uevent_sock;
	pollfds[0].events = POLLIN;

	ret = fcntl(uevent_sock,F_SETFL, O_NONBLOCK);
	if (ret < 0)
		LOGE("Error on fcntl:%s", strerror(errno));

	while (1) {
		ret = poll(pollfds, 1, timeout);

		switch (ret) {
		case 0:
			LOGD("poll timeout");
			continue;
		case -1:
			LOGD("poll error:%s", strerror(errno));
			break;

		default:
			if (pollfds[0].revents & POLLIN)
				process_uevent_message(uevent_sock);
		}
	}

	close(uevent_sock);
}

int start_uevent_monitor(void)
{
	pthread_t pth_uevent_monitor;
	return pthread_create(&pth_uevent_monitor, NULL,
			      usb_tty_monitor_thread, NULL);
}


/*
* pppd	ttyUSB0 115200 mru 1280 mtu 1280 nodetach debug dump defaultroute usepeerdns novj novjccomp noipdefault ipcp-accept-local ipcp-accept-remote connect-delay 5000 linkname ppp0 connect "/system/bin/chat -v -s -f /system/etc/ppp/chat-wcdma-connect"
*
*/

#define ARG_FILE "/data/3g/pppd-runtime.conf"

#define CHAT_FILE "/data/3g/chat-runtime.conf"

static char *pppd_wcdma_arg[] = {
	"115200",
//	"mru 1280",
//	"mtu 1280",
	"nodetach",
	"debug",
//	"kdebug 4",
//	"dump",
	"defaultroute",
	"usepeerdns",
	"novj",
	"novjccomp",
	"noipdefault",
	"ipcp-accept-local",
	"ipcp-accept-remote",
	"connect-delay 5000",
	"linkname ppp0",
	"connect \"/system/bin/chat -v -s -f /data/3g/chat-runtime.conf\"",
	NULL
};

static char *pppd_cdma_arg[] = {
	"debug",
	"nodetach",
	"115200",
//	"mru 1280",
//	"mtu 1280",
	"modem",
	"user \"ctnet@mycdma.cn\"",
	"password \"vnet.mobi\"",
	"crtscts",
	"show-password",
	"usepeerdns",
	"noipdefault",
	"novj",
	"novjccomp",
	"noccp",
	"defaultroute",
	"ipcp-accept-local",
	"ipcp-accept-remote",
	"linkname ppp0",
	"connect \"/system/bin/chat -v -s -f /data/3g/chat-runtime.conf\"",
	NULL
};

static char *pppd_td_arg[] = {
	"115200",
	"nodetach",
	"debug",
	"modem",
	"defaultroute",
	"usepeerdns",
	"novj",
	"novjccomp",
	"noipdefault",
	"ipcp-accept-local",
	"ipcp-accept-remote",
	"connect-delay 5000",
	"linkname ppp0",
	NULL
};

static char *pppd_wcdma_chat[] = {
	"ABORT \'NO CARRIER\'",
	"ABORT \'ERROR\'",
	"ABORT \'NO DIALTONE\'",
	"ABORT \'BUSY\'",
	"ABORT \'NO ANSWER\'",
	"\'\' \\rAT",
	"OK \\rATZ",
	"OK \\rAT+CGDCONT=1,\"IP\",\"3gnet\",,0,0",
	"OK-AT-OK ATDT*99#",
	"CONNECT \\d\\c",
	NULL
};

static char *pppd_cdma_chat[] = {
	"ABORT \'NO CARRIER\'",
	"ABORT \'ERROR\'",
	"ABORT \'NO DIALTONE\'",
	"ABORT \'BUSY\'",
	"ABORT \'NO ANSWER\'",
	"\'\' \\rATZ",
	"OK ATDT#777",
	"CONNECT \\d\\c",
	NULL
};

static inline int write_append_line(int fd, const char * src, int num){
	int ret = -1;
	if(lseek(fd, 0, SEEK_END) < 0){
		LOGE("write_append_line, SEEK_END failed");
		return -1;
	}
	if(write(fd, src, num) < 0){
		LOGE("write_append_line, write SRC: %s failed", src);
		return -1;
	}
	if(lseek(fd, 0, SEEK_END) < 0){
		LOGE("write_append_line, SEEK_END failed");
		return -1;
	}
	if(write(fd, "\n", strlen("\n")) < 0){
		LOGE("write_append_line, write \\n failed");
		return -1;
	}
	return 0;
}

void write_pppd_arg(const char * data_port,  char *src[]){

    int fd = -1, ret = -1, i = 0;

    fd = open(ARG_FILE, O_WRONLY | O_CREAT | O_TRUNC, 00666);
	if(fd == -1){
		LOGE("Open pppd conf File Failed!%s[%x]\n", ARG_FILE, fd);
		close(fd);
		return;
	}

	LOGE("PPPD: %s", ARG_FILE);

	ret = write_append_line(fd, data_port, strlen(data_port));
	if(ret < 0)
		goto error;
	else
		LOGE("%s", data_port);
	
	for(i = 0; src[i] != NULL; i++){
		ret = write_append_line(fd, src[i], strlen(src[i]));
		if(ret < 0)
			goto error;
		else
			LOGE("%s", src[i]);
	}
error:
	
    close(fd);

}

void write_pppd_chat(char *src[]){

    int fd = -1, ret = -1, i = 0;

    fd = open(CHAT_FILE, O_WRONLY | O_CREAT | O_TRUNC, 00666);
	if(fd == -1){
		LOGE("Open pppd conf File Failed!%s[%x]\n", CHAT_FILE, fd);
		close(fd);
		return;
	}

	LOGE("PPPD: %s", CHAT_FILE);
	
	for(i = 0; src[i] != NULL; i++){
		ret = write_append_line(fd, src[i], strlen(src[i]));
		if(ret < 0)
			goto error;
		else
			LOGE("%s", src[i]);
	}
error:
	
    close(fd);

}

void write_runtime_conf(const char * data_port, int model){
	switch(model){
		case WCDMA_MODEL:
			LOGE("PPPD: WCDMA connect");
			write_pppd_arg(data_port, pppd_wcdma_arg);
			write_pppd_chat(pppd_wcdma_chat);
			break;
		case CDMA_MODEL:
			LOGE("PPPD: CDMA connect");
			write_pppd_arg(data_port, pppd_cdma_arg);
			write_pppd_chat(pppd_cdma_chat);
			break;
		case TD_SCDMA_MODEL:
			LOGE("PPPD: TD_SCDMA connect");
			write_pppd_arg(data_port, pppd_td_arg);
			break;
		default:
			LOGE("Warning: set_pppd_conf unknow model\n");
			break;
	}
}


//PPPD end

