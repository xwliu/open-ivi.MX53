/*
 * Copyright (C) 2012 TokenWireless Comm. Inc.
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
 *
 */
#define  LOG_TAG  "canbox"
#include <hardware/hardware.h>
#include <hardware/canboxhal.h>

#include <errno.h>
#include <pthread.h>
#include <termios.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <math.h>
#include <time.h>
#include <semaphore.h>
#include <signal.h>
#include <unistd.h>
#include <stdio.h>
#include <cutils/log.h>
#include <cutils/sockets.h>
#include <cutils/properties.h>
#include <sys/queue.h>
#include <semaphore.h>
#include "canbox_RZCDAS.h"

// following data is the main data we are using...
static int g_canbox_handle = 0;
struct thread_info 
{    /* Used as argument to thread_start() */
     pthread_t thread_id;        /* ID returned by pthread_create() */
     char     *argv_string;      /* not used right now */
};
thread_info read_thread_info;

// following data is using in the read thread to read buffer
#define MAX_READ_BYTE_ONCE  64
#define MAX_BUFFER_STRING   512
// the extra 16 bytes should never be used, as we just enlarge the buffer for safe...
static unsigned char tty_buffer[MAX_BUFFER_STRING + 16] = {0};
static unsigned char tmp_buffer[MAX_BUFFER_STRING + 16] = {0};

static int tty_buffer_pos = 0;
sem_t sem_count;
sem_t sem_command_count;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER; 
pthread_mutex_t command_mutex = PTHREAD_MUTEX_INITIALIZER; 
static int canbox_write(unsigned char* buffer, int length);

struct canbox_notification_node{
    canbox_notification a_canbox_notification;
    TAILQ_ENTRY(canbox_notification_node) pointers;
};

TAILQ_HEAD(tail_head, canbox_notification_node);

tail_head canbox_head;

void dump_buffer(void)
{
	LOGW("++++++++++++++CANBOX MAIN BUFFER DUMP+++++POS: 0x%d++++++\r\n", tty_buffer_pos);
	for(int i = 0; i < MAX_BUFFER_STRING; i+=8){
		LOGW("CANBOX <0x%02x> <0x%02x> <0x%02x> <0x%02x> <0x%02x> <0x%02x> <0x%02x> <0x%02x>\n", 
			tty_buffer[i], tty_buffer[i+1], tty_buffer[i+2], tty_buffer[i+3],
			tty_buffer[i+4], tty_buffer[i+5], tty_buffer[i+6], tty_buffer[i+7]);

	}
	LOGW("--------------CANBOX MAIN BUFFER DUMP---------------\r\n");
}


void dump_read_buffer(char* p, int size)
{
    	LOGW("++++++++++++++CANBOX READ BUFFER DUMP+++++size: 0x%d++++++", size);
	for(int i = 0; i < MAX_READ_BYTE_ONCE; i+=8){
			LOGI("canbox 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x\n", 
			p[i], p[i+1], p[i+2], p[i+3],
			p[i+4], p[i+5], p[i+6], p[i+7]);

	}
	LOGW("--------------CANBOX READ BUFFER DUMP---------------");
}

// add the new notification into the list end
static int push_canbox_notificaiton(canbox_notification* p_new_canbox_notificaiton)
{
    // step1. create a new canbox_notification and assign right value
    int ret = 0;    
    canbox_notification_node* p_new_node = (canbox_notification_node*)malloc(sizeof(canbox_notification_node));
    if(p_new_node == NULL){
        LOGE("canbox allocate memory err!!\n");
        ret = -ENOMEM;
    }
    memcpy(&p_new_node->a_canbox_notification, p_new_canbox_notificaiton, sizeof(canbox_notification));
    // step2. push the canbox_notificaion into queue 
    pthread_mutex_lock(&mutex);
    TAILQ_INSERT_TAIL(&canbox_head, p_new_node, pointers);
    pthread_mutex_unlock(&mutex);
    // step3. increase the sem, after this, if there any thread want to read the data, it can continue....
    sem_post(&sem_count);
    LOGD("canbox SEM++\n");
    return ret;
}

// got the first notification in the list and delete it from the list
static int pop_canbox_notificaiton(canbox_notification* p_pop_canbox_notificaition)
{
    int ret = 0;
    canbox_notification_node* temp = NULL;
    // if we do not have data in the queue, the sem_wait should never return and wait here
    sem_wait(&sem_count);
    LOGD("canbox SEM--\n");
    // if we get here, this means there is any canbox notification in the queue, so read it out and pass to up level
    pthread_mutex_lock(&mutex);
    temp = canbox_head.tqh_first;
    if(temp == NULL){
        LOGE("canbox Queue NULL while sem get access");
        // TODO: destroy all related resource
    }
    memcpy(p_pop_canbox_notificaition, &(temp->a_canbox_notification), sizeof(canbox_notification));
    TAILQ_REMOVE(&canbox_head, canbox_head.tqh_first, pointers);
    pthread_mutex_unlock(&mutex);
    free(temp);
    return ret;
}

static int canbox_get_notification(struct canbox_device_t *dev, canbox_notification* p_notification)
{
    pop_canbox_notificaiton(p_notification);
    return 0;
}

static unsigned char canbox_calc_checksum(unsigned char *data, int len)
{
        char checksum = 0;
        int i;

        for(i=0; i<len; i++){
            checksum += data[i];
        }
        checksum = checksum ^ 0xFF;
        return checksum;
}
void verify_notification(unsigned char* command, int nSize)
{
        unsigned char ack = 0xFF;    // ACK
        canbox_notification my_notification;
        int datalen = nSize - 2;
        if(datalen != *(command + 2) + 2){
            LOGE("canbox data len ERROR! Throw...\n");
            return;
        }
        int checksum = canbox_calc_checksum(command + 1, datalen);
        if(checksum != *(command + nSize - 1)){
            LOGE("canbox check full command checksum FAILED! Throw...\n");
            ack = 0xF0;     // CheckSum Err
            goto End;
        }
        my_notification.buffer_size = nSize;
        // TODO: check whether the nSize will bigger than data max size 64 bytes
        memcpy(&my_notification.data[0], command, nSize);
        if(0 != push_canbox_notificaiton(&my_notification)){
            ack = 0xFC;     // Busy
        }
End:        
        canbox_write(&ack, 1);
}

void check_full_command(void)
{
    unsigned char* p_head = NULL;
    unsigned char* p_ack = NULL;
    //dump_buffer();
    if(tty_buffer_pos == 0){
        LOGE("canbox check command but buffer pos is ZERO\r\n");
        return;
    }
    int search_byte = tty_buffer_pos;
    unsigned char* p_start = &tty_buffer[0];
    unsigned char command[MAX_COMMAND_BYTE_LENGTH] = {0};
    do{
        p_ack = (unsigned char*)memchr(p_start, CANBOX_PROTOCAL_FRAME_COMMAND_ACK, search_byte);
        //NOTE: we will throw it by searching the next head byte and we have to also trigger the write process...        
        //there may still happen that the 0xFF is inside the normal event, in this case, we may trigger the write process by wrong case.
        //On the other hand, if we meet this condition, this event will be ignore and request resend as the check sum must be wrong
        if(p_ack != NULL){
            LOGI("canbox ACK Found\r\n");
            sem_post(&sem_command_count);
        }
        //DEBUG_LOGW("tty_buffer_pos: %d\r\n", tty_buffer_pos);
        p_head = (unsigned char*)memchr(p_start, CANBOX_PROTOCAL_FRAME_START_BYTE, search_byte);
        if(p_head == NULL){
            LOGE("canbox No head found, clean buffer pos to ZERO\r\n");
            // as we have search all the buffer and do not found any head bytes, it's ok just return back to the start position in the main buffer
            tty_buffer_pos = 0;
            return;
        }
        // 0x2E is last valid byte, so we copy the 0x2E to the buffer head and move the counter to the next position
        if((p_head - &tty_buffer[0] + 1) == tty_buffer_pos){
            LOGE("canbox Find HEAD FIRST BYTE with Last Byte\r\n");
            memcpy(&tty_buffer[0], p_head, 1);
            tty_buffer_pos = 1;
            return;
        }
        // type is the last vaild byte
        if((p_head - &tty_buffer[0] + 2) == tty_buffer_pos){
            memcpy(&tty_buffer[0], p_head, 2);
            tty_buffer_pos = 2;
            return;
        }
        // length is the last valid byte
        else if((p_head - &tty_buffer[0] + 3) == tty_buffer_pos){
            memcpy(&tty_buffer[0], p_head, 3);
            tty_buffer_pos = 3;
            return;
        }
        else{   // case we have one more byte at least which means we should have length byte
            int length = *(p_head + 2); // although we are using int, the length will not bigger than 256
            // at this point, the tty_buffer_point will indicate the last valid position...
            if(length > MAX_COMMAND_BYTE_LENGTH){
                LOGE("canbox bad frame command, length bigger then 256!!\r\n");
                // TODO: take care this condition
                return;
            }
            int left_bytes = tty_buffer_pos - (((p_head + 2) - &tty_buffer[0]) + 1);
            if(length > left_bytes){
                //DEBUG_LOGW("canbox length bigger than left_bytes, we should wait next buffer read...");
                //avoid to cover the buffer while do memcpy, 1. copy to temp, 2.copy to buffer,3. init temp
                memcpy(&tmp_buffer[0], p_head, left_bytes + 3);
                memcpy(&tty_buffer[0], &tmp_buffer[0], left_bytes + 3);
                memset(&tmp_buffer[0], 0, 512+16);
                tty_buffer_pos = left_bytes + 3;
                return;
            }else {
                memset(&command[0], 0, MAX_COMMAND_BYTE_LENGTH);
                memcpy(&command[0], (p_head), length + 4);
                verify_notification(&command[0], length + 4);
                memset(&tty_buffer[0], 0,  (((p_head + 2) - &tty_buffer[0] + 1) + length + 1));
                //avoid to cover the buffer while do memcpy, 1. copy to temp, 2.copy to buffer,3. init temp
                memcpy(&tmp_buffer[0], (p_head + 2 + length + 1 + 1), ((left_bytes > length) ? (left_bytes - length - 1): 0));
                memcpy(&tty_buffer[0], &tmp_buffer[0], ((left_bytes > length) ? (left_bytes - length - 1): 0));
                memset(&tmp_buffer[0], 0, 512+16);
                tty_buffer_pos = ((left_bytes > length) ? (left_bytes - length - 1): 0); 
                if(tty_buffer_pos <= 0){ 
                    memset(&tty_buffer[0], 0, MAX_BUFFER_STRING + 16); 
                    return;  
                }else{ 
                    memset(&tty_buffer[0] + tty_buffer_pos, 0, MAX_BUFFER_STRING + 16 - tty_buffer_pos);
                    continue; 
                }
            }
        }
    }while(1);
    //dump_buffer();
}

/*!
 * The main purpose for the read thread is:
 * 1. read data from ttymxc2 if there is anything could read
 * 2. parse the buffer to see any complete command start with 4BB4 and command length byte is received and checksum is correct
 * 3. put the complete command into queue notify other thread to get it...
 * 4. clean the find command buffer and go step 1 again
 */
static void * read_thread_func(void *arg)
{
    int ret = 0;
    int retry_times = 0;
    const int MAX_RETRY_TIMES = 300;
    char buf[MAX_READ_BYTE_ONCE] = {0};
    if(g_canbox_handle == 0){
        LOGE("canbox file handle is ZERO!\n");
    }
    LOGI("canboxread thread started...");   
    fd_set readfds;
    do {
#if 1
        int read_bytes = 0;
        FD_ZERO(&readfds);
        FD_SET(g_canbox_handle, &readfds);
        ret = select(FD_SETSIZE, &readfds, NULL, NULL, NULL);
        if (ret < 0){
            LOGE("canbox select ttymxc2 got failed!\n");    
            continue;
        }
        read_bytes = read(g_canbox_handle, buf, sizeof(buf) );
        //dump_read_buffer(buf, read_bytes);
        // as we will wait at the select function if there is no data
        // if there is data but we can not read it, this should be critical error
        // so count 3 times is enough to handle such kind of case....
        if(read_bytes < 0){
            LOGE("canbox got bytes wrong!!!\n");
            retry_times++;
            if(retry_times > MAX_RETRY_TIMES){
                LOGE("canbox read bytes err!!!\n");
                break;
            }
            continue;
        }
        if(read_bytes <  MAX_READ_BYTE_ONCE){
            LOGI("canbox actual read bytes: %d", read_bytes);
        }
        // according to the MPU-MCU protocal, the max length for one command will never bigger than 2(one bytes) = 256 bytes length
        // and after each we cat the buf into the tty_buffer, we will check whether there is any complete command and clean the command buffer
        // so such kind of case won't be happen in fact...
        if((tty_buffer_pos + read_bytes) > MAX_BUFFER_STRING){
            LOGE("canbox tty buffer overflow! tty_buffer_pos: 0x%x, read_bytes: 0x%x\n", tty_buffer_pos, read_bytes);
            // TODO: reset buffer to handle this condition
        }
        // step1: copy the read buffer into the main buffer and increase the index
        memcpy(&tty_buffer[tty_buffer_pos], buf, read_bytes);
        tty_buffer_pos += read_bytes;
        // step2: check whether there is any complete command exist:
        check_full_command();
#endif      
    } while (1);

    return NULL;
}

static int canbox_write(unsigned char* buffer, int length)
{
    int ret = 0;
    int retry  = 0;
    int s = 0;        
    LOGE("canbox_write: %s length: %d\n", buffer, length);    
    // the command_mutex will be only call here, so it WILL NOT be possible to cause deadlock with sem_comand_count
    pthread_mutex_lock(&command_mutex);    
    // we will try to clean the sem_command_count before we continue...
    while((sem_trywait(&sem_command_count) == 0) &&  (retry < 100))
        retry++;
    // BUGBUG: there is still possible that the sem_command_count is not zero...
    retry = 0;
    do{
        retry++;    // count from 1
        write(g_canbox_handle, buffer, length);
        if(length == 1){ // which means we are writting ack/nack which we do not care reponse
            goto End;
        }
        struct timespec ts; 
        if(clock_gettime(CLOCK_REALTIME, &ts) == -1)
            goto End;
        ts.tv_nsec += 1000 * 1000 * 100;    // count with nano second so 100ms here...
        s = sem_timedwait(&sem_command_count, &ts);
    }while((s == (-1)) && (errno == ETIME) && (retry < 3));
End:
    if((s == (-1)) && (errno == ETIME) ){
        LOGE("canbox write command timeout: %d Retried: %d\n", buffer[1], retry);
        ret = (-1);
    }	
	usleep(100 * 1000);
    pthread_mutex_unlock(&command_mutex);        
    return ret;
}

static int canbox_send_command(struct canbox_device_t *dev, canbox_command* p_notification)
{
    canbox_write(p_notification->data, p_notification->buffer_size);
    return 0;
}

// following function is the standard hal function and structure
static int canbox_device_open(const struct hw_module_t* module, const char* name, struct hw_device_t** device);

static struct hw_module_methods_t canbox_module_methods = {
    open: canbox_device_open
};

struct canbox_module_t HAL_MODULE_INFO_SYM = {
    common: {
        tag: HARDWARE_MODULE_TAG,
        version_major: 1,
        version_minor: 0,
        id: CANBOX_HARDWARE_MODULE_ID,
        name: "canbox module",
        author: "TokenWireless",
        methods: &canbox_module_methods,
    },
    /* module api interface */
};

/*!
 * This function will be the init point for the canbox function where it could work 
 * It will initialize the ttymxc3(UART4) with suitable parameter setting
 * Then it will start the two thread to do the major job
 * the read thread will continue read the buffer from the ttymxc3 if there is any data inside ttymxc3
 * it will then bypass data into buffer queue
 * after that, if any of the thread is try to read the event from queue, we will make the upper thread get the queue data.
 */
static int canbox_open(struct canbox_device_t *dev)
{
    int ret = 0;
    LOGI("canbox_open\r\n");
    // initialize the QUEUE we need to save the canbox notification
    TAILQ_INIT(&canbox_head);
    // initialize the sem and we will only use the sem in this process and init value is ZERO
    if(sem_init(&sem_count, 0, 0) < 0){
        LOGE("canbox sem init ERROR!");
        return -1;
    }
    if(sem_init(&sem_command_count, 0, 0) < 0){
        LOGE("canbox sem command init ERROR!");
        return -1;
    }    
    if(pthread_mutex_init(&mutex, NULL) < 0){
        LOGE("canbox mutex init ERROR!");
        return -1;
    }
    if(pthread_mutex_init(&command_mutex, NULL) < 0){
        LOGE("canbox comand mutex init ERROR!");
        return -1;
    }    
    g_canbox_handle = open( "/dev/ttymxc3", O_RDWR | O_NOCTTY | O_NONBLOCK);
    if(g_canbox_handle == -1){
        LOGE("canbox open fail\r\n");
        return -1;
    }
        //TODO: set to the right baud rate
    struct termios  ios;
    tcgetattr( g_canbox_handle, &ios );
    LOGI("canbox default set to 38400bps, 8-N-1");
    bzero(&ios, sizeof(ios));
    // TODO:   actually baudrate shouble be pass down by the java level
    ios.c_cflag = B38400 | CS8 | CLOCAL | CREAD;
    ios.c_iflag = 0;
    ios.c_oflag = 0;
    ios.c_lflag = 0;  /* disable ECHO, ICANON, etc... */
    tcsetattr( g_canbox_handle, TCSANOW, &ios );
        
    ret = pthread_create(&read_thread_info.thread_id, NULL , &read_thread_func,  &read_thread_info);
    if (ret != 0){
        LOGE("canbox read thread create failed!\n");
        // TODO: close com port and release all the resource.   
    }
    return 0;
}

static int canbox_device_close(struct hw_device_t *device)
{
    struct canbox_device_t *dev = (struct canbox_device_t*)device;
    if(dev){
        free(dev);
    }
    return 0;
}

static int canbox_device_open(const struct hw_module_t* module, const char* name, struct hw_device_t** device)
{
    int status = -EINVAL;
    struct canbox_device_t *dev;
    LOGI("canbox_device_open\n");
    dev = (struct canbox_device_t*)malloc(sizeof(*dev));
    /* initialize our state here */
    memset(dev, 0, sizeof(*dev));
    /* initialize the procs */
    dev->common.tag = HARDWARE_DEVICE_TAG;
    dev->common.version = 0;
    dev->common.module = const_cast<hw_module_t*>(module);
    dev->common.close = canbox_device_close; 
    dev->canbox_open = canbox_open;
    dev->canbox_get_notification = canbox_get_notification;
    dev->canbox_send_command = canbox_send_command;
    *device = &dev->common;
    status = 0;
    return status;
}

