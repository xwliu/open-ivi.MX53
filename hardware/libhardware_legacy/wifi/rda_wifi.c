/*
 * Copyright 2008, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
/* Copyright (C) 2011 Freescale Semiconductor,Inc. */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <sys/ioctl.h>

#include "hardware_legacy/wifi.h"
#include "libwpa_client/wpa_ctrl.h"

#define LOG_TAG "WifiHW"
#include "cutils/log.h"
#include "cutils/memory.h"
#include "cutils/misc.h"
#include "cutils/properties.h"
#include "private/android_filesystem_config.h"
#ifdef HAVE_LIBC_SYSTEM_PROPERTIES
#define _REALLY_INCLUDE_SYS__SYSTEM_PROPERTIES_H_
#include <sys/_system_properties.h>
#endif

static struct wpa_ctrl *ctrl_conn;
static struct wpa_ctrl *monitor_conn;

extern int do_dhcp();
extern int ifc_init();
extern void ifc_close();
extern char *dhcp_lasterror();
extern void get_dhcp_info();
extern int init_module(void *, unsigned long, const char *);
extern int delete_module(const char *, unsigned int);

static char iface[PROPERTY_VALUE_MAX];
// TODO: use new ANDROID_SOCKET mechanism, once support for multiple
// sockets is in


#ifndef WIFI_DRIVER_MODULE_PATH
#define WIFI_DRIVER_MODULE_PATH         "/system/lib/modules/wlan.ko"
#endif
#ifndef WIFI_DRIVER_MODULE_NAME
#define WIFI_DRIVER_MODULE_NAME         "wlan"
#endif
#ifndef WIFI_DRIVER_MODULE_ARG
#define WIFI_DRIVER_MODULE_ARG          ""
#endif
#ifndef WIFI_FIRMWARE_LOADER
#define WIFI_FIRMWARE_LOADER		""
#endif
#define WIFI_TEST_INTERFACE		"sta"


#define WIFI_DRIVER_LOADER_DELAY	1000000


static const char IFACE_DIR[]           = "/data/system/wpa_supplicant";


static const char DRIVER_MODULE_NAME[]  = WIFI_DRIVER_MODULE_NAME;
static const char DRIVER_MODULE_TAG[]   = WIFI_DRIVER_MODULE_NAME " ";
static const char DRIVER_MODULE_PATH[]  = WIFI_DRIVER_MODULE_PATH;
static const char DRIVER_MODULE_ARG[]   = WIFI_DRIVER_MODULE_ARG;
static const char FIRMWARE_LOADER[]     = WIFI_FIRMWARE_LOADER;
static const char DRIVER_PROP_NAME[]    = "wlan.driver.status";
static const char SUPPLICANT_NAME[]     = "wpa_supplicant";
static const char SUPP_PROP_NAME[]      = "init.svc.wpa_supplicant";
static const char SUPP_CONFIG_TEMPLATE[]= "/system/etc/wifi/wpa_supplicant.conf";
static const char SUPP_CONFIG_FILE[]    = "/data/misc/wifi/wpa_supplicant.conf";
static const char MODULE_FILE[]         = "/proc/modules";
static const char MAC_FILE[]            = "/data/misc/wifi/softmac";

#define RDA5990_SUPPORT 1 

#ifdef RDA5990_SUPPORT

#define RDA_BT_IOCTL_MAGIC 'u'

#define RDABT_DRV_NAME "/dev/rdacombo"

#define RDA_WIFI_POWER_ON_IOCTL    _IO(RDA_BT_IOCTL_MAGIC ,0x10)
#define RDA_WIFI_POWER_OFF_IOCTL    _IO(RDA_BT_IOCTL_MAGIC ,0x11)
#define RDA_WIFI_POWER_SET_TEST_MODE_IOCTL    _IO(RDA_BT_IOCTL_MAGIC ,0x12)
#define RDA_WIFI_POWER_CANCEL_TEST_MODE_IOCTL    _IO(RDA_BT_IOCTL_MAGIC ,0x13)
#define RDA_WIFI_DEBUG_MODE_IOCTL    _IO(RDA_BT_IOCTL_MAGIC ,0x14)
#define RDA_WIFI_INJECT_CARD_IOCTL    _IO(RDA_BT_IOCTL_MAGIC ,0x20)
#define RDA_WIFI_EJECT_CARD_IOCTL    _IO(RDA_BT_IOCTL_MAGIC ,0x21)

#define RDA_WIFI_PF_AVAILABLE_IOCTL 	_IO(RDA_BT_IOCTL_MAGIC ,0x22)

static int rda_fd = -1;

int rdabt_send_cmd_to_drv(int cmd, unsigned char shutdown) 
{
	
	if(rda_fd <  0)
	    rda_fd = open(RDABT_DRV_NAME, O_RDWR);
		
	if (rda_fd < 0) {
		LOGE("Can't open rdabt device");
		return -1;
	}
	
	if(ioctl(rda_fd, cmd) == -1)
	{
		LOGE("rdabt_send_cmd_to_drv failed \n");
	}
		
	if(shutdown)
	{
		close(rda_fd);
		rda_fd = -1;
	}
	
	return 0;
}

static void wait_rdawifi_pf_available(void)
{
	int retry = 10;
	if(rda_fd <  0)
    	rda_fd = open(RDABT_DRV_NAME, O_RDWR);
		
	if (rda_fd < 0) {
		LOGE("Can't open rdabt device");
		close(rda_fd);
		rda_fd = -1;
		return;
	}
	
	do{
		if(0 == ioctl(rda_fd, RDA_WIFI_PF_AVAILABLE_IOCTL)){
			LOGE("RDA5990 sdio was ejected!");
			usleep(50000);
			break;
		}
		usleep(100000);
		if(0 == retry)
			LOGE("Wait sdio eject time out!");
	}while(retry--);
	
}

int wifi_set_power(int on)
{


	int ret = -1;

        LOGE("wifi_set_power %d\n", on);
	if(on)
	{
	ret = rdabt_send_cmd_to_drv(RDA_WIFI_POWER_ON_IOCTL,1);
	}
	else
	{
	ret = rdabt_send_cmd_to_drv(RDA_WIFI_POWER_OFF_IOCTL,1);
	}

	return ret;

}

int wifi_insert_sdio_card(int isInsert)
{


	int ret = -1;

        LOGE("wifi_insert_sdio_card %d\n", isInsert);
	if(isInsert)
	{
	ret = rdabt_send_cmd_to_drv(RDA_WIFI_INJECT_CARD_IOCTL,1);
	}
	else
	{
	ret = rdabt_send_cmd_to_drv(RDA_WIFI_EJECT_CARD_IOCTL,1);
	}

	return ret;

}

#endif

static int insmod(const char *filename, const char *args)
{
    void *module;
    unsigned int size;
    int ret;
    LOGD("insmod \"%s\": %s\n",filename,args);		
    module = load_file(filename, &size);
    if (!module)
        return -1;

    ret = init_module(module, size, args);

    free(module);

    return ret;
}

static int rmmod(const char *modname)
{
    int ret = -1;
    int maxtry = 10;

    while (maxtry-- > 0) {
        ret = delete_module(modname, O_NONBLOCK | O_EXCL);
        if (ret < 0 && errno == EAGAIN)
            usleep(500000);
        else
            break;
    }
    if (ret != 0)
        LOGD("Unable to unload driver module \"%s\": %s\n",
             modname, strerror(errno));
    return ret;
}

int do_dhcp_request(int *ipaddr, int *gateway, int *mask,
                    int *dns1, int *dns2, int *server, int *lease) {
    /* For test driver, always report success */
    if (strcmp(iface, WIFI_TEST_INTERFACE) == 0)
        return 0;

    if (ifc_init() < 0)
        return -1;

    if (do_dhcp(iface) < 0) {
        ifc_close();
        return -1;
    }
    ifc_close();
    get_dhcp_info(ipaddr, gateway, mask, dns1, dns2, server, lease);
    return 0;
}

const char *get_dhcp_error_string() {
    return dhcp_lasterror();
}

static int check_driver_loaded() {
    char driver_status[PROPERTY_VALUE_MAX];
    FILE *proc;
    char line[sizeof(DRIVER_MODULE_TAG)+10];

    if (!property_get(DRIVER_PROP_NAME, driver_status, NULL)
            || strcmp(driver_status, "ok") != 0) {
        return 0;  /* driver not loaded */
    }
    /*
     * If the property says the driver is loaded, check to
     * make sure that the property setting isn't just left
     * over from a previous manual shutdown or a runtime
     * crash.
     */
    if ((proc = fopen(MODULE_FILE, "r")) == NULL) {
        LOGW("Could not open %s: %s", MODULE_FILE, strerror(errno));
        property_set(DRIVER_PROP_NAME, "unloaded");
        return 0;
    }
    while ((fgets(line, sizeof(line), proc)) != NULL) {
        if (strncmp(line, DRIVER_MODULE_TAG, strlen(DRIVER_MODULE_TAG)) == 0) {
            fclose(proc);
            return 1;
        }
    }
    fclose(proc);
    property_set(DRIVER_PROP_NAME, "unloaded");
    return 0;
}

/* for Atheros HotSpot */
int wifi_load_ap_driver()
{
    char driver_status[PROPERTY_VALUE_MAX];
    int count = 100; /* wait at most 20 seconds for completion */

    if (check_driver_loaded()) {
        return 0;
    }

    if (strcmp(FIRMWARE_LOADER,"") == 0) {
        usleep(WIFI_DRIVER_LOADER_DELAY);
        property_set(DRIVER_PROP_NAME, "ok");
    }
    else {
        property_set("ctl.start", WIFI_FIRMWARE_LOADER ":load_ap");
    }
    sched_yield();
    while (count-- > 0) {
        if (property_get(DRIVER_PROP_NAME, driver_status, NULL)) {
            if (strcmp(driver_status, "ok") == 0)
                return 0;
            else if (strcmp(DRIVER_PROP_NAME, "failed") == 0) {
                wifi_unload_driver();
                return -1;
            }
        }
        usleep(200000);
    }
    property_set(DRIVER_PROP_NAME, "timeout");
    wifi_unload_driver();
    return -1;
}

int wifi_load_driver()
{
    char driver_status[PROPERTY_VALUE_MAX];
    int count = 100; /* wait at most 20 seconds for completion */

    if (check_driver_loaded()) {
        return 0;
    }
    //Power up rda5990 first
     wifi_set_power(0);	
     wifi_set_power(1);

#ifdef NO_BUILDIN_DRIVER
    if (insmod(DRIVER_MODULE_PATH, DRIVER_MODULE_ARG) < 0)
        return -1;
#endif
     LOGE("Waiting for card inserted!!!\n");
     wifi_insert_sdio_card(0);
     usleep(2000000);

     LOGE("Waiting for card inserted,done!!!\n");

    if (strcmp(FIRMWARE_LOADER,"") == 0) {
        usleep(WIFI_DRIVER_LOADER_DELAY);
        property_set(DRIVER_PROP_NAME, "ok");
    } else {

        property_set("ctl.start", FIRMWARE_LOADER);

    }
    sched_yield();
    while (count-- > 0) {
        if (property_get(DRIVER_PROP_NAME, driver_status, NULL)) {
            if (strcmp(driver_status, "ok") == 0)
                return 0;
            else if (strcmp(DRIVER_PROP_NAME, "failed") == 0) {
                wifi_unload_driver();
                return -1;
            }
        }
        usleep(200000);
    }
    property_set(DRIVER_PROP_NAME, "timeout");
    wifi_unload_driver();
    return -1;
}

int wifi_unload_driver()
{
    int count = 20; /* wait at most 10 seconds for completion */
    int ret = 0;
#ifdef NO_BUILDIN_DRIVER	
    if (rmmod(DRIVER_MODULE_NAME) == 0) {

	while (count-- > 0) {
	    if (!check_driver_loaded())
	    {
            wifi_insert_sdio_card(1);
			wait_rdawifi_pf_available();
			wifi_set_power(0);
	       break;
	    }
    	    usleep(500000);
	}
	if (count) {
         		
    	    ret = 0;
	}
	ret =  -1;

      wifi_set_power(0);
	  
    } else
        ret = -1;
#endif	

	  wifi_insert_sdio_card(1);
	  wait_rdawifi_pf_available();
	  wifi_set_power(0);


      return ret;

}

int ensure_config_file_exists()
{
    char buf[2048];
    int srcfd, destfd;
    int nread;

    if (access(SUPP_CONFIG_FILE, R_OK|W_OK) == 0) {
        return 0;
    } else if (errno != ENOENT) {
        LOGE("Cannot access \"%s\": %s", SUPP_CONFIG_FILE, strerror(errno));
        return -1;
    }

    srcfd = open(SUPP_CONFIG_TEMPLATE, O_RDONLY);
    if (srcfd < 0) {
        LOGE("Cannot open \"%s\": %s", SUPP_CONFIG_TEMPLATE, strerror(errno));
        return -1;
    }

    destfd = open(SUPP_CONFIG_FILE, O_CREAT|O_WRONLY, 0660);
    if (destfd < 0) {
        close(srcfd);
        LOGE("Cannot create \"%s\": %s", SUPP_CONFIG_FILE, strerror(errno));
        return -1;
    }

    while ((nread = read(srcfd, buf, sizeof(buf))) != 0) {
        if (nread < 0) {
            LOGE("Error reading \"%s\": %s", SUPP_CONFIG_TEMPLATE, strerror(errno));
            close(srcfd);
            close(destfd);
            unlink(SUPP_CONFIG_FILE);
            return -1;
        }
        write(destfd, buf, nread);
    }

    close(destfd);
    close(srcfd);

    if (chown(SUPP_CONFIG_FILE, AID_SYSTEM, AID_WIFI) < 0) {
        LOGE("Error changing group ownership of %s to %d: %s",
             SUPP_CONFIG_FILE, AID_WIFI, strerror(errno));
        unlink(SUPP_CONFIG_FILE);
        return -1;
    }
    return 0;
}

int wifi_start_supplicant()
{
    char supp_status[PROPERTY_VALUE_MAX] = {'\0'};
    int count = 200; /* wait at most 20 seconds for completion */
#ifdef HAVE_LIBC_SYSTEM_PROPERTIES
    const prop_info *pi;
    unsigned serial = 0;
#endif

    /* Check whether already running */
    if (property_get(SUPP_PROP_NAME, supp_status, NULL)
            && strcmp(supp_status, "running") == 0) {
        return 0;
    }

    /* Before starting the daemon, make sure its config file exists */
    if (ensure_config_file_exists() < 0) {
        LOGE("Wi-Fi will not be enabled");
        return -1;
    }

    /* Clear out any stale socket files that might be left over. */
    wpa_ctrl_cleanup();

#ifdef HAVE_LIBC_SYSTEM_PROPERTIES
    /*
     * Get a reference to the status property, so we can distinguish
     * the case where it goes stopped => running => stopped (i.e.,
     * it start up, but fails right away) from the case in which
     * it starts in the stopped state and never manages to start
     * running at all.
     */
    pi = __system_property_find(SUPP_PROP_NAME);
    if (pi != NULL) {
        serial = pi->serial;
    }
#endif
    property_set("ctl.start", SUPPLICANT_NAME);
    sched_yield();

    while (count-- > 0) {
#ifdef HAVE_LIBC_SYSTEM_PROPERTIES
        if (pi == NULL) {
            pi = __system_property_find(SUPP_PROP_NAME);
        }
        if (pi != NULL) {
            __system_property_read(pi, NULL, supp_status);
            if (strcmp(supp_status, "running") == 0) {
                return 0;
            } else if (pi->serial != serial &&
                    strcmp(supp_status, "stopped") == 0) {
                return -1;
            }
        }
#else
        if (property_get(SUPP_PROP_NAME, supp_status, NULL)) {
            if (strcmp(supp_status, "running") == 0)
                return 0;
        }
#endif
        usleep(100000);
    }
    return -1;
}

int wifi_stop_supplicant()
{
    char supp_status[PROPERTY_VALUE_MAX] = {'\0'};
    int count = 50; /* wait at most 5 seconds for completion */

    /* Check whether supplicant already stopped */
    if (property_get(SUPP_PROP_NAME, supp_status, NULL)
        && strcmp(supp_status, "stopped") == 0) {
        return 0;
    }

    property_set("ctl.stop", SUPPLICANT_NAME);
    sched_yield();

    while (count-- > 0) {
        if (property_get(SUPP_PROP_NAME, supp_status, NULL)) {
            if (strcmp(supp_status, "stopped") == 0)
                return 0;
        }
        usleep(100000);
    }
    return -1;
}

int wifi_connect_to_supplicant()
{
    char ifname[256];
    char supp_status[PROPERTY_VALUE_MAX] = {'\0'};

    /* Make sure supplicant is running */
    if (!property_get(SUPP_PROP_NAME, supp_status, NULL)
            || strcmp(supp_status, "running") != 0) {
        LOGE("Supplicant not running, cannot connect");
        return -1;
    }

#if ATHEROS_WIFI_SDK
    property_get("wifi.interface", iface, WIFI_DEF_IFNAME);
#else
    property_get("wifi.interface", iface, WIFI_TEST_INTERFACE);
#endif


    //snprintf(ifname, sizeof(ifname), "%s/%s", IFACE_DIR, iface);

    //snprintf(ifname, sizeof(ifname), "%s/%s", IFACE_DIR, "wpa_wlan0");

    snprintf(ifname, sizeof(ifname), "wlan0");   
    LOGE("ctrl ifname = %s %s\n", ifname,__TIME__);
	
    usleep(5000000);
#if 0
    { /* check iface file is ready */
        int cnt = 160; /* 8 seconds (160*50)*/
        sched_yield();
        while ( access(ifname, F_OK|W_OK)!=0 && cnt-- > 0) {
            usleep(50000);
        }
        if (access(ifname, F_OK|W_OK)==0) {
            LOGE("ifname %s is ready to read/write cnt=%d\n", ifname, cnt);
        } else {
            LOGE("ifname %s is not ready, cnt=%d\n", ifname, cnt);
        }
    }
#endif
#if ATHEROS_WIFI_SDK
    LOGE("wifi_connect_to_supplicant: ifname = %s\n", ifname);
#endif

    ctrl_conn = wpa_ctrl_open(ifname);
    if (ctrl_conn == NULL) {
        LOGE("Unable to open connection to supplicant on \"%s\": %s",
             ifname, strerror(errno));
        return -1;
    }
    monitor_conn = wpa_ctrl_open(ifname);
    if (monitor_conn == NULL) {
        wpa_ctrl_close(ctrl_conn);
        ctrl_conn = NULL;
        return -1;
    }
    if (wpa_ctrl_attach(monitor_conn) != 0) {
        wpa_ctrl_close(monitor_conn);
        wpa_ctrl_close(ctrl_conn);
        ctrl_conn = monitor_conn = NULL;
        return -1;
    }
    return 0;
}

int wifi_send_command(struct wpa_ctrl *ctrl, const char *cmd, char *reply, size_t *reply_len)
{
    int ret;

    if (ctrl_conn == NULL) {
        LOGV("Not connected to wpa_supplicant - \"%s\" command dropped.\n", cmd);
        return -1;
    }
    ret = wpa_ctrl_request(ctrl, cmd, strlen(cmd), reply, reply_len, NULL);
    if (ret == -2) {
        LOGD("'%s' command timed out.\n", cmd);
        return -2;
    } else if (ret < 0 || strncmp(reply, "FAIL", 4) == 0) {
        return -1;
    }
    if (strncmp(cmd, "PING", 4) == 0) {
        reply[*reply_len] = '\0';
    }
    return 0;
}

int wifi_wait_for_event(char *buf, size_t buflen)
{
    size_t nread = buflen - 1;
    int fd;
    fd_set rfds;
    int result;
    struct timeval tval;
    struct timeval *tptr;
    
    if (monitor_conn == NULL) {
        LOGD("Connection closed\n");
        strncpy(buf, WPA_EVENT_TERMINATING " - connection closed", buflen-1);
        buf[buflen-1] = '\0';
        return strlen(buf);
    }

    result = wpa_ctrl_recv(monitor_conn, buf, &nread);
    if (result < 0) {
        LOGD("wpa_ctrl_recv failed: %s\n", strerror(errno));
        strncpy(buf, WPA_EVENT_TERMINATING " - recv error", buflen-1);
        buf[buflen-1] = '\0';
        return strlen(buf);
    }
    buf[nread] = '\0';
    /* LOGD("wait_for_event: result=%d nread=%d string=\"%s\"\n", result, nread, buf); */
    /* Check for EOF on the socket */
    if (result == 0 && nread == 0) {
        /* Fabricate an event to pass up */
        LOGD("Received EOF on supplicant socket\n");
        strncpy(buf, WPA_EVENT_TERMINATING " - signal 0 received", buflen-1);
        buf[buflen-1] = '\0';
        return strlen(buf);
    }
    /*
     * Events strings are in the format
     *
     *     <N>CTRL-EVENT-XXX 
     *
     * where N is the message level in numerical form (0=VERBOSE, 1=DEBUG,
     * etc.) and XXX is the event name. The level information is not useful
     * to us, so strip it off.
     */
    if (buf[0] == '<') {
        char *match = strchr(buf, '>');
        if (match != NULL) {
            nread -= (match+1-buf);
            memmove(buf, match+1, nread+1);
        }
    }
    return nread;
}

void wifi_close_supplicant_connection()
{
    if (ctrl_conn != NULL) {
        wpa_ctrl_close(ctrl_conn);
        ctrl_conn = NULL;
    }
    if (monitor_conn != NULL) {
        wpa_ctrl_close(monitor_conn);
        monitor_conn = NULL;
    }
}

int wifi_command(const char *command, char *reply, size_t *reply_len)
{
    return wifi_send_command(ctrl_conn, command, reply, reply_len);
}