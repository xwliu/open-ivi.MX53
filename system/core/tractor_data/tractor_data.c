/*
 * Copyright (C) 2010 Freescale Semiconductor, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>
#include <errno.h>
#include <fcntl.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <cutils/properties.h>

#define LOG_BUF_MAX 512

static const char dalvik_file[] = "/data/dalvik-cache/system@framework@framework.jar@classes.dex";
static const char sig_file[] = "/data/tractor/firstboot";
static const char sig_file_1[] = "/data/tractor/normalboot";
static const char bt_file[] = "/data/tractor/bt_service";


static int log_fd;
static int dalvik_fd;
static int sig_fd;
static int bt_fd;

static void log_write(const char *fmt, ...)
{
    char buf[LOG_BUF_MAX];
    va_list ap;

    if (log_fd < 0) return;

    va_start(ap, fmt);
    vsnprintf(buf, LOG_BUF_MAX, fmt, ap);
    buf[LOG_BUF_MAX - 1] = 0;
    va_end(ap);
    write(log_fd, buf, strlen(buf));
}

static int read_data(int *data)
{
#if 0
    char *buffer;
    int ret;
    struct stat s;

    /* check conf file */
    if (stat(cf_file, &s) == 0) {
		/* conf file already existed */
		cf_fd = open(cf_file, O_RDWR);
		if (cf_fd >= 0) {
		    buffer = calloc(1, s.st_size + 1);
		    read(cf_fd, buffer, s.st_size);
		    free(buffer);
		    close(cf_fd);

			return s.st_size;
		}
    }
#endif
	return 0;
}

static void write_data(char *data, int size)
{
	int len;
	char buf[16];
	
    sig_fd = open(sig_file, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
    if (sig_fd < 0) {
		log_write("create, open file %s error:%d\n", sig_file, errno);
		return;
    }

    len = sprintf(buf, "%d",data[0]);

    write(sig_fd, buf, len);
    close(sig_fd);
}

static void write_bt_data(char *data, int size)
{
	int len;
	char buf[16];
	
    bt_fd = open(bt_file, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
    if (bt_fd < 0) {
		log_write("create, open file %s error:%d\n", bt_file, errno);
		return;
    }

    len = sprintf(buf, "%d",data[0]);

    write(bt_fd, buf, len);
    close(bt_fd);
}

static int is_file_exist(void)
{
    struct stat s;

    /* check conf file */
    if (stat(dalvik_file, &s) == 0) {
		log_write("$$$firstboot file exist, normal boot\n");

		return 0;
    }
	log_write("$$$firstboot file not exist, first boot\n");
    return 1;
}

int main(int argc, char **argv)
{
	char sig;
	
	//log_write("$$$tractor_data main++\n");

    /* open log */
    log_fd = open("/dev/ttymxc0", O_RDWR);

	log_write("$$$tractor_data main++ 111\n");
	
    if (is_file_exist())
    {
    	log_write("android first boot\n");
		mkdir("/data/tractor", 0777);
		sig = 1;
    }
	else {
		sig = 0;
	}
	write_data(&sig, sizeof(char));

	write_bt_data(0, sizeof(char));
    //log_write("$$$tractor_data main--\n");

    close(log_fd);

    return 0;
}

