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
#define  LOG_TAG  "mpeg"
#include <hardware/hardware.h>
#include <hardware/mpeghal.h>

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
#include <hardware/mpeghal.h>
#include <sys/queue.h>
#include <semaphore.h>

#include <mcu_ioctl.h>

#define DEBUG 0
#if DEBUG
#define DEBUG_LOGI      LOGI
#define DEBUG_LOGW      LOGW
#define DEBUG_LOGE      LOGE
#else DEBUG
#define DEBUG_LOGI
#define DEBUG_LOGW
#define DEBUG_LOGE
#endif


// following data is the main data we are using...
static int g_mpeg_handle = 0;
struct thread_info 
{    /* Used as argument to thread_start() */
     pthread_t thread_id;        /* ID returned by pthread_create() */
     char     *argv_string;      /* not used right now */
};
thread_info read_thread_info;
thread_info parse_thread_info;

// following data is using in the read thread to capture a complete command
#define MAX_READ_BYTE_ONCE	256
#define MAX_BUFFER_STRING	512
// the extra 16 bytes should never be used, as we just enlarge the buffer for safe...
static unsigned char tty_buffer[MAX_BUFFER_STRING + 16] = {0};
static unsigned char tmp_buffer[MAX_BUFFER_STRING + 16] = {0};

static int tty_buffer_pos = 0;
static int Mpeg_debug_level = 0;
sem_t sem_count;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER; 

struct mpeg_notification_node{
    mpeg_notification a_mpeg_notification;
    TAILQ_ENTRY(mpeg_notification_node) pointers;
};

TAILQ_HEAD(tail_head, mpeg_notification_node);

tail_head mpeg_head;

void dump_buffer(void)
{
	DEBUG_LOGW("++++++++++++++MPEG MAIN BUFFER DUMP+++++POS: 0x%d++++++\r\n", tty_buffer_pos);
	for(int i = 0; i < MAX_BUFFER_STRING; i+=8){
		DEBUG_LOGW("mpeg <0x%02x> <0x%02x> <0x%02x> <0x%02x> <0x%02x> <0x%02x> <0x%02x> <0x%02x>\n", 
			tty_buffer[i], tty_buffer[i+1], tty_buffer[i+2], tty_buffer[i+3],
			tty_buffer[i+4], tty_buffer[i+5], tty_buffer[i+6], tty_buffer[i+7]);

	}
	DEBUG_LOGW("--------------MPEG MAIN BUFFER DUMP---------------\r\n");
}

void dump_read_buffer(char* p, int size)
{
	DEBUG_LOGW("++++++++++++++MPEG READ BUFFER DUMP+++++size: 0x%d++++++", size);
	for(int i = 0; i < MAX_READ_BYTE_ONCE; i+=8){
		//DEBUG_LOGW("mpeg 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x\n", 
			LOGI("mpeg 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x\n", 
			p[i], p[i+1], p[i+2], p[i+3],
			p[i+4], p[i+5], p[i+6], p[i+7]);

	}
	DEBUG_LOGW("--------------MPEG READ BUFFER DUMP---------------");
}

// add the new notification into the list end
static int push_mpeg_notificaiton(mpeg_notification* p_new_mpeg_notificaiton)
{
	// step1. create a new mpeg_notification and assign right value
	int ret = 0;	
	mpeg_notification_node* p_new_node = (mpeg_notification_node*)malloc(sizeof(mpeg_notification_node));
	if(p_new_node == NULL){
		DEBUG_LOGE("mpeg allocate memory err!!\n");
		ret = -ENOMEM;
	}
	memcpy(&p_new_node->a_mpeg_notification, p_new_mpeg_notificaiton, sizeof(mpeg_notification));
	// step2. push the mpeg_notificaion into queue 
	pthread_mutex_lock(&mutex);
	TAILQ_INSERT_TAIL(&mpeg_head, p_new_node, pointers);
	pthread_mutex_unlock(&mutex);
	// step3. increase the sem, after this, if there any thread want to read the data, it can continue....
	sem_post(&sem_count);
	DEBUG_LOGI("mpeg SEM++\n");
	return ret;
}

// got the first notification in the list and delete it from the list
static int pop_mpeg_notificaiton(mpeg_notification* p_pop_mpeg_notificaition)
{
	int ret = 0;
	mpeg_notification_node* temp = NULL;
	// if we do not have data in the queue, the sem_wait should never return and wait here
	sem_wait(&sem_count);
	DEBUG_LOGI("mpeg SEM--\n");
	// if we get here, this means there is any mpeg notification in the queue, so read it out and pass to up level
	pthread_mutex_lock(&mutex);
	temp = mpeg_head.tqh_first;
	if(temp == NULL){
		DEBUG_LOGE("mpeg Queue NULL while sem get access");
		// TODO: destroy all related resource
	}
	memcpy(p_pop_mpeg_notificaition, &(temp->a_mpeg_notification), sizeof(mpeg_notification));
	TAILQ_REMOVE(&mpeg_head, mpeg_head.tqh_first, pointers);
	pthread_mutex_unlock(&mutex);
	free(temp);
	return ret;
}

static int mpeg_get_notification(struct mpeg_device_t *dev, mpeg_notification* p_notification)
{
	pop_mpeg_notificaiton(p_notification);
	return 0;
}

static unsigned char mpeg_calc_checksum(unsigned char *data, int len)
{
	char checksum = 0;
	int i;

	for(i=0; i<len; i++){
		checksum += data[i];
	}
	checksum = (~checksum) + 1;

	//printk(KERN_INFO "%s mpeg_calc_checksum:0x%x\n", DEBUG_LOGTAG, checksum);

	return checksum;
}
char nty_buf[128];
void nty_buf_output()
{
	if(Mpeg_debug_level == 1) {
		LOGI("%s", nty_buf);
	}
}
void parse_command(unsigned char* command)
{
	mpeg_notification my_notification;
	unsigned int length = command[0];
	unsigned int checksum = mpeg_calc_checksum(command, length);
	if(command[length] != checksum){
		DEBUG_LOGE("mpeg checksum error! 0x%x VS 0x%x\n", command[length], checksum);
	}
	int command_id = command[1];
	my_notification.the_report_type = (mpeg_report_type)command_id;
	sprintf(nty_buf, "MPEG.CPP GOT NOTIFICATION TYPE 0x%02x", my_notification.the_report_type);
	LOGI("MPEG.CPP GOT NOTIFICATION TYPE 0x%02x", my_notification.the_report_type);

	nty_buf_output();
	switch(command_id & 0xFF){
		case MPEG_HOST_IND:
		{
			DEBUG_LOGI("MPEG_HOST_IND Found");
			break;
		}
		case MPEG_INIT_CFM:
		{
			DEBUG_LOGI("MPEG_INIT_CFM Found: 0x%x", command[2]);
			// for the init cfm, the version byte is just behind command id
			my_notification.the_report_context.init_cfm_report.version = command[2]; 
			break;
		}
		case MPEG_DEVICE_STATUS_CFM:
		case MPEG_DEVICE_STATUS_IND:
		{
			DEBUG_LOGI("MPEG_DEVICE_STATUS Found: 0x%x", command[2]);
			my_notification.the_report_context.device_status_report.disc_status = command[2] & (0x03);
			my_notification.the_report_context.device_status_report.spindle_status = (command[2] & (0x04)) >> 2;
			my_notification.the_report_context.device_status_report.video_mode = (command[2] & (0x08)) >> 3;
			my_notification.the_report_context.device_status_report.file_status = (command[2] & (0x10)) >> 4;
			if(command[2]&0x60){
				my_notification.the_report_context.device_status_report.root_menu_status = 1;
			} else {
				my_notification.the_report_context.device_status_report.root_menu_status = 0;
			}
			break;
		}
		case MPEG_PLAY_STATUS_RSP:
		case MPEG_PLAY_STATUS_IND:
		{
			DEBUG_LOGI("MPEG_PLAY_STATUS Found: 0x%x", command[2]);
			my_notification.the_report_context.play_status_report.disc_type = command[2];
			my_notification.the_report_context.play_status_report.play_status = command[3];
			my_notification.the_report_context.play_status_report.repeat_mode = (command[4] & (0x07));
			my_notification.the_report_context.play_status_report.shuffle_mode = (command[4] & (0x08)) >> 3;
			my_notification.the_report_context.play_status_report.scan_mode = (command[4] & (0x10)) >> 4;
			my_notification.the_report_context.play_status_report.play_type = (command[4] & (0x80)) >> 7;
			break;
		}
		case MPEG_PLAY_INFO_RSP:
		case MPEG_PLAY_INFO_IND:
		{
			DEBUG_LOGI("MPEG_PLAY_INFO Found: 0x%x", command[2]);
			my_notification.the_report_context.play_info_report.file_type = command[2];
			my_notification.the_report_context.play_info_report.curr_index = (command[3] << 8) | command[4];
			my_notification.the_report_context.play_info_report.all_index = (command[5] << 8) | command[6];
			my_notification.the_report_context.play_info_report.curr_playtime = (command[7] << 8) | command[8];
			my_notification.the_report_context.play_info_report.all_playtime = (command[9] << 8) | command[10];
			my_notification.the_report_context.play_info_report.curr_folder_index = (command[11] << 8) | command[12];
			my_notification.the_report_context.play_info_report.all_folder_count = (command[13] << 8) | command[14];
			my_notification.the_report_context.play_info_report.all_file_count = (command[15] << 8) | command[16];
			break;
		}
		case MPEG_PLAYLIST_INDEX_RSP:
		{			
			DEBUG_LOGI("MPEG_PLAYLIST_INDEX_RSP Found: 0x%x", command[2]);
			my_notification.the_report_context.play_index_report.isSucc = command[2];
			my_notification.the_report_context.play_index_report.all_folder_count = (command[3] << 8) | command[4];
			my_notification.the_report_context.play_index_report.all_file_count = (command[5] << 8) | command[6];
			my_notification.the_report_context.play_index_report.all_audio_folder_count= (command[7] << 8) | command[8];
			my_notification.the_report_context.play_index_report.all_video_folder_count= (command[9] << 8) | command[10];
			my_notification.the_report_context.play_index_report.all_photo_folder_count= (command[11] << 8) | command[12];
			my_notification.the_report_context.play_index_report.all_audio_file_count= (command[13] << 8) | command[14];
			my_notification.the_report_context.play_index_report.all_video_file_count= (command[15] << 8) | command[16];
			my_notification.the_report_context.play_index_report.all_photo_file_count= (command[17] << 8) | command[18];
			break;
		}	
		case MPEG_PLAYLIST_INDEX_INFO:
		{		
			/*
			int i = 0;
			int j = 0;
			DEBUG_LOGI("MPEG_PLAYLIST_INDEX_IND Found: 0x%x", command[2]);
			my_notification.the_report_context.play_index_ind_report.curr_frame_count = command[2];
			my_notification.the_report_context.play_index_ind_report.frame_file_count = (length - 2) / 5;	// each 5 bytes contains two file info
			unsigned char* p = &command[3];	// should be the first file position
			for(i = 0, j = 0; i < my_notification.the_report_context.play_index_ind_report.frame_file_count; i++, j+=2){
				my_notification.the_report_context.play_index_ind_report.file_type[j] = (*p) & 0x0F;
				my_notification.the_report_context.play_index_ind_report.file_type[j+1] = (*p) & 0xF0 >> 4;
				my_notification.the_report_context.play_index_ind_report.parent_index[j] = ((*(p+1)) << 8) | (*(p+2));
				my_notification.the_report_context.play_index_ind_report.parent_index[j+1] = ((*(p+3)) << 8) | (*(p+4));
				p += 5;
			}
			*/
			return;
		}
		case MPEG_PLAYLIST_NAME_RSP:
		{
			/*get how much files or folders*/
			my_notification.the_report_context.play_name_cfm_report.file_num = command[2];
			//dump_read_buffer((char *)&(command[2]), length);
			/*get the file msgs to the array and pass to JNI and then to JAVA to handle these*/
			memset(&(my_notification.the_report_context.play_name_cfm_report.file_msg[0]), 0, 255);
			memcpy(&(my_notification.the_report_context.play_name_cfm_report.file_msg[0]),&command[3],length);
			break;
		}	
		case MPEG_KEY_CONTROL_CFM:
		{
			DEBUG_LOGI("MPEG_KEY_CONTROL_CFM Found: 0x%x", command[2]);
			my_notification.the_report_context.key_control_cfm.isSucc = command[2];
			break;
		}
		case MPEG_DBG_MSG:
		{
			DEBUG_LOGI("MPEG_DEG_MSG Found");
			length -= 2;	/* subtruct the bytes of cmd and checksum*/
			my_notification.the_report_context.device_dbg_msg.dbg_msg[length] = '\0';
			while(length--) {
				my_notification.the_report_context.device_dbg_msg.dbg_msg[length] = command[2 + length];		
			}
			if(Mpeg_debug_level == 1) {
				LOGI("MPEG debug msg:%s", my_notification.the_report_context.device_dbg_msg.dbg_msg);
			}
			return;
		}
		case MPEG_SOURCE_CFM:
			DEBUG_LOGI("MPEG_SOURCE_CFM Found");
			my_notification.the_report_context.set_mpeg_source_cfm_report.source_off = command[2];
			break;
		case MPEG_DISC_STATUS:
			//DEBUG_LOGI("MPEG_DISC_STATUS Found");
			//my_notification.the_report_context.disc_status.disc_sts = command[2];
			//break;
			return;
		case MPEG_DVD_DISP_INFO_RSP:
		case MPEG_DVD_DISP_INFO_IND:
			my_notification.the_report_context.dvd_disp_info_report.audio_num = command[2];
			my_notification.the_report_context.dvd_disp_info_report.audio_index = command[3];
			my_notification.the_report_context.dvd_disp_info_report.audio_coding_type= command[4];
			my_notification.the_report_context.dvd_disp_info_report.audio_channel_num = command[5];
			my_notification.the_report_context.dvd_disp_info_report.audio_iso693_val= (command[6] << 8) | command[7];
			my_notification.the_report_context.dvd_disp_info_report.subtitle_iso693_val= (command[8] << 8) | command[9];
			my_notification.the_report_context.dvd_disp_info_report.subtitle_all_num= command[10];
			my_notification.the_report_context.dvd_disp_info_report.subtitle_curr_index= command[11];
			break;
		case MPEG_ID3_INFO_RSP:
		case MPEG_ID3_INFO_IND:
			//dump_read_buffer((char *)&(command[2]), length);
			/*get the id3 msgs to the array and pass to JNI and then to JAVA to handle these*/
			/*-2: cmd&crc*/
			memset(&(my_notification.the_report_context.id3_info_report.id3_info[0]), 0, 255);
			memcpy(&(my_notification.the_report_context.id3_info_report.id3_info[0]),&command[2],length-2);
			break;
		case MPEG_SET_SUSPEND_CFM:
			my_notification.the_report_context.set_suspend_report.command_result = command[2];
			break;
		case MPEG_POWER_OFF_CFM:
			int i ;
			my_notification.the_report_context.power_off_cfm.command_result = command[2];
			for(i = 0; i < 64; i++){
				my_notification.the_report_context.power_off_cfm.data[i]		    = command[3+i];
			}
			break;
		case MPEG_GET_BOOKMARK:
			break;
		default:
			return;
	}
	push_mpeg_notificaiton(&my_notification);
}


void check_full_command(void)
{
	unsigned char* p_head = NULL;
	//dump_buffer();
    if(tty_buffer_pos == 0){
        DEBUG_LOGE("mpeg check command but buffer pos is ZERO\r\n");
        return;
    }
	int search_byte = tty_buffer_pos;
	unsigned char* p_start = &tty_buffer[0];
	unsigned char command[MAX_COMMAND_BYTE_LENGTH] = {0};
	do{
        //DEBUG_LOGW("tty_buffer_pos: %d\r\n", tty_buffer_pos);
		p_head = (unsigned char*)memchr(p_start, MPEG_PROTOCAL_FRAME_START_BYTE_ONE, search_byte);
		if(p_head == NULL){
			DEBUG_LOGE("mpeg No head found, clean buffer pos to ZERO\r\n");
			// as we have search all the buffer and do not found any head bytes, it's ok just return back to the start position in the main buffer
			tty_buffer_pos = 0;
			return;
		}
		// 0x4B is last valid byte, so we copy the 0x4B to the buffer head and move the counter to the next position
		// So if later we have 0xB4 be copy into the buffer, we won't miss this command.
		if((p_head - &tty_buffer[0] + 1) == tty_buffer_pos){
            DEBUG_LOGE("mpeg Find HEAD FIRST BYTE with Last Byte\r\n");
			memcpy(&tty_buffer[0], p_head, 1);
			tty_buffer_pos = 1;
			return;
		}
		// if we found two continue bytes 0x4BB4
		else if((*p_head == MPEG_PROTOCAL_FRAME_START_BYTE_ONE) && ((char)*(p_head+1) == (char)MPEG_PROTOCAL_FRAME_START_BYTE_TWO)){
			// length does not read in yet...
			if((p_head - &tty_buffer[0] + 2) == tty_buffer_pos){
				memcpy(&tty_buffer[0], p_head, 2);
				tty_buffer_pos = 2;
				return;
			}
			// length read in but no data read in yet...
			else if((p_head - &tty_buffer[0] + 3) == tty_buffer_pos){
				memcpy(&tty_buffer[0], p_head, 3);
				tty_buffer_pos = 3;
				return;
			}
			else {
				int length = *(p_head + 2);	// although we are using int, the length will not bigger than 256
				// at this point, the tty_buffer_point will indicate the last valid position...
				if(length > MAX_COMMAND_BYTE_LENGTH){
					DEBUG_LOGE("mpeg bad frame command, length bigger then 256!!\r\n");
					// TODO: take care this condition
					return;
				}
				int left_bytes = tty_buffer_pos - (((p_head + 2) - &tty_buffer[0]) + 1);
				if(length > left_bytes){
					//DEBUG_LOGW("mpeg length bigger than left_bytes, we should wait next buffer read...");
					//avoid to cover the buffer while do memcpy, 1. copy to temp, 2.copy to buffer,3. init temp
					memcpy(&tmp_buffer[0], p_head, left_bytes + 3);
					memcpy(&tty_buffer[0], &tmp_buffer[0], left_bytes + 3);
					memset(&tmp_buffer[0], 0, 512+16);
					tty_buffer_pos = left_bytes + 3;
					return;
				}else {
					memset(&command[0], 0, MAX_COMMAND_BYTE_LENGTH);
					memcpy(&command[0], (p_head + 2), length + 1);
					parse_command(&command[0]);
					memset(&tty_buffer[0], 0,  (((p_head + 2) - &tty_buffer[0] + 1) + length));
					//avoid to cover the buffer while do memcpy, 1. copy to temp, 2.copy to buffer,3. init temp
					memcpy(&tmp_buffer[0], (p_head + 2 + length + 1), left_bytes - length);
					memcpy(&tty_buffer[0], &tmp_buffer[0], left_bytes - length);
					memset(&tmp_buffer[0], 0, 512+16);
					tty_buffer_pos = left_bytes - length; 
					if(tty_buffer_pos <= 0){ 
						memset(&tty_buffer[0], 0, MAX_BUFFER_STRING + 16); 
						return;  
					}else{ 
						memset(&tty_buffer[0] + tty_buffer_pos, 0, MAX_BUFFER_STRING + 16 - tty_buffer_pos);
						continue; 
					}
				}
			}
		}else{ // no continue header bytes found 
			DEBUG_LOGE("mpeg No continue head found, search next in current buffer\r\n");
			search_byte = tty_buffer_pos - ( (p_head - &tty_buffer[0]) + 1);
			p_start = p_head + 1;
			continue;
		}
	}while(1);
	//dump_buffer();
}

/*!
 * The main purpose for the read thread is:
 * 1. read data from ttymxc2 if there is anything could read
 * 2. parse the buffer to see any complete command start with 4BB4 and command length byte is received
 * 3. notify the parse thread to continue the parse the command 
 * 4. clean the find command buffer and go step 1 again
 */
static void * read_thread_func(void *arg)
{
	int ret = 0;
	int retry_times = 0;
	const int MAX_RETRY_TIMES = 3;
	char buf[MAX_READ_BYTE_ONCE] = {0};
	if(g_mpeg_handle == 0){
		DEBUG_LOGE("mpeg file handle is ZERO!\n");
	}
	DEBUG_LOGI("mpeg read thread started...");	
	fd_set readfds;
//	char test_buf[] = "123456789";
//	write(g_mpeg_handle, test_buf, sizeof(test_buf) );
	do {
#if 1
		int read_bytes = 0;
		FD_ZERO(&readfds);
		FD_SET(g_mpeg_handle, &readfds);
		ret = select(FD_SETSIZE, &readfds, NULL, NULL, NULL);
		if (ret < 0){
			DEBUG_LOGW("mpeg select ttymxc2 got failed!\n");	
			continue;
		}
		read_bytes = read(g_mpeg_handle, buf, sizeof(buf) );
		//dump_read_buffer(buf, read_bytes);
		// as we will wait at the select function if there is no data
		// if there is data but we can not read it, this should be critical error
		// so count 3 times is enough to handle such kind of case....
		if(read_bytes < 0){
			DEBUG_LOGW("mpeg got bytes wrong!!!\n");
			retry_times++;
			if(retry_times > MAX_RETRY_TIMES){
				DEBUG_LOGE("mpeg read bytes err!!!\n");
				break;
			}
			continue;
		}
		if(read_bytes <  MAX_READ_BYTE_ONCE){
			DEBUG_LOGI("mpeg actual read bytes: %d", read_bytes);
		}
		// according to the MPU-MCU protocal, the max length for one command will never bigger than 2(one bytes) = 256 bytes length
		// and after each we cat the buf into the tty_buffer, we will check whether there is any complete command and clean the command buffer
		// so such kind of case won't be happen in fact...
		if((tty_buffer_pos + read_bytes) > MAX_BUFFER_STRING){
			DEBUG_LOGE("mpeg tty buffer overflow! tty_buffer_pos: 0x%x, read_bytes: 0x%x\n", tty_buffer_pos, read_bytes);
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

static int prepareBuffer(unsigned char* buffer, mpeg_command* p_notification)
{
	unsigned char* p = buffer;
	// two heads byte
	*p = 0x4B;
	p++;
	*p = 0xB4;
	p++;
	unsigned char* pLength = p;
	// Length: CMD+PARAM+CHECKSUM
	//*p = countall; will be assign the correct value until we know the command type
	p++;
	// CMD
	*p = p_notification->the_command_type;
	LOGW("MPEG.CPP SEND COMMAND TYPE 0x%x", *p);
	p++;
	// PARAM
	switch(p_notification->the_command_type){
		case MPEG_INIT_REQ:
			*pLength = 1 + 1 + 1; // 1 CMD + 1 PARAM + 1 CHECKSUM
			*p = p_notification->the_command_context.init_req_command.mpu_version;
			p++;
			break;
		case MPU_SET_MPEG_SOURCE_REQ:
			*pLength = 1 + 1 + 1; // 1 CMD + 1 PARAM + 1 CHECKSUM
			*p = p_notification->the_command_context.set_mpeg_source_req_command.source_off;
			p++;
			break;
		case MPU_REPORT_STATUS_REQ:
			*pLength = 1 + 1 + 1; // 1 CMD + 1 PARAM + 1 CHECKSUM
			*p = p_notification->the_command_context.device_status_status_req_command.disc_status;
			p++;
			break;

		case MPEG_PLAYLIST_INDEX_REQ:
			*pLength = 1 +0 + 1; // 1 CMD + 0 PARAM + 1 CHECKSUM
			//*p = p_notification->the_command_context.set_mpeg_source_req_command.source_off;
			//p++;
			break;
		case MPEG_DEVICE_STATUS_REQ:
			*pLength = 1 +0 + 1; // 1 CMD + 1 PARAM + 1 CHECKSUM
			break;
		case MPEG_KEY_CONTROL_REQ:
			*pLength = 1 + 5 + 1;// 1 CMD + 5 PARAM + 1 CHECKSUM
			 *p = p_notification->the_command_context.key_control_req_command.key_type;
			 p++;
			 *p = p_notification->the_command_context.key_control_req_command.param[0];
			 p++;
			 *p = p_notification->the_command_context.key_control_req_command.param[1];
			 p++;
			 *p = p_notification->the_command_context.key_control_req_command.param[2];
			 p++;
			 *p = p_notification->the_command_context.key_control_req_command.param[3];
			 p++;
			 break;
		case MPEG_PLAY_STATUS_REQ:
			*pLength = 1 +0 + 1; // 1 CMD + 0 PARAM + 1 CHECKSUM
			break;
		case MPEG_PLAY_INFO_REQ:
			*pLength = 1 +0 + 1; // 1 CMD + 0 PARAM + 1 CHECKSUM
			break;
		case MPU_REPORT_TS_POS_REQ:
			*pLength = 1 +5 + 1;
			*p= p_notification->the_command_context.report_ts_req_command.op_status;
			p++;
			*p= p_notification->the_command_context.report_ts_req_command.X_range[0];
			p++;
			*p= p_notification->the_command_context.report_ts_req_command.X_range[1];
			p++;
			*p= p_notification->the_command_context.report_ts_req_command.Y_range[0];
			p++;
			*p= p_notification->the_command_context.report_ts_req_command.Y_range[1];
			p++;
			break;
		case MPEG_PLAYLIST_NAME_REQ:
			*pLength = 1 +3 + 1;
			*p= p_notification->the_command_context.playlist_name_req_command.file_folder_ind;
			p++;
			*p= p_notification->the_command_context.playlist_name_req_command.index_start_num[0];
			p++;
			*p= p_notification->the_command_context.playlist_name_req_command.index_start_num[1];
			p++;
			break;
		case MPEG_DVD_DISP_INFO_REQ:
			*pLength = 1 +0 + 1; // 1 CMD + 0 PARAM + 1 CHECKSUM
			break;
		case MPEG_ID3_INFO_REQ:
			*pLength = 1 +0 + 1; // 1 CMD + 0 PARAM + 1 CHECKSUM
			break;
		case MPU_SET_MPEG_SUSPEND_REQ:
			*pLength = 1 + 1 + 1;
			*p =  p_notification->the_command_context.set_suspend_command.command_type;
			p++;
			break;
		case MPEG_SET_MPEG_POWER_OFF_REQ:
			*pLength = 1 +0 + 1; // 1 CMD + 0 PARAM + 1 CHECKSUM
			break;
		case MPU_RET_BOOTMARK:
		{
			int i;
			*pLength = 1 +65 + 1; // 1 CMD + 0 PARAM + 1 CHECKSUM
			*p = p_notification->the_command_context.ret_bootmark_cfm.bootmark_status;
			p++;
			for( i = 0 ; i < 64 ; i ++) {
				*p = p_notification->the_command_context.ret_bootmark_cfm.bootmark_data[i];
				p++;
			}
			break;
		}
		case MPEG_RESET_REQ:
			*pLength = 1 +0 + 1; // 1 CMD + 1 PARAM + 1 CHECKSUM
			break;
		default:
			break;
	}
	// calculate the checksum from the lenght byte and end before the checksum byte
	// so the length = (1 cmd + N para + 1 checksum) - 1 checksum byte + 1 length byte
	*p = mpeg_calc_checksum(pLength, *pLength - 1 + 1);
	return ((*pLength) + 3);	// the command total length is base on the length + cmd + headbytes(4b b4)
}

static int mpeg_send_command(struct mpeg_device_t *dev, mpeg_command* p_notification)
{
	// TODO: check whether it can send buffer now and then send out the buffer...
	unsigned char length= 0;
	unsigned char command_buffer[MAX_READ_BYTE_ONCE] = {0};
	length = prepareBuffer(command_buffer, p_notification);
	/*{
		int i = 0;
		LOGI("######mpeg_send_command######");
		for(i = 0; i < length; i++){
			LOGI(" [0x%x ]", command_buffer[i]);
			
		}	
		LOGI("@@@@@mpeg_send_command@@@@@");
	}*/
	write(g_mpeg_handle, command_buffer, length);
	return 0;
}

// following function is the standard hal function and structure
static int mpeg_device_open(const struct hw_module_t* module, const char* name, struct hw_device_t** device);

static struct hw_module_methods_t mpeg_module_methods = {
    open: mpeg_device_open
};

struct mpeg_module_t HAL_MODULE_INFO_SYM = {
	common: {
		tag: HARDWARE_MODULE_TAG,
		version_major: 1,
		version_minor: 0,
		id: MPEG_HARDWARE_MODULE_ID,
		name: "mpeg module",
		author: "TokenWireless",
		methods: &mpeg_module_methods,
	},
	/* module api interface */
};

/*!
 * This function will be the init point for the mpeg function where it could work 
 * It will initialize the ttymxc2(UART3) with suitable parameter setting
 * Then it will start the two thread to do the major job
 * the read thread will continue read the buffer from the ttymxc2 if there is any data inside ttymxc2
 * it will then do a quick parse and put any known command into queue and increment the sem
 * after that, if any of the thread is try to read the event from queue, we will make the upper thread get the queue data.
 */
static int mpeg_open(struct mpeg_device_t *dev)
{	
	DEBUG_LOGI("mpeg_open\r\n");
	// initialize the QUEUE we need to save the mpeg notification
	TAILQ_INIT(&mpeg_head);

	int mcu_fd;
	int ret = 0;
	mcu_fd = open( "/dev/mcu_i2c", O_RDWR | O_CREAT | O_TRUNC );
	int value = 0;	// keep reset as low before power on
	ret = ioctl(mcu_fd, MPEG_RESET, &value);	
	//DEBUG_LOGI("mpeg_reset_disable\r\n");	
	//value = 0;	// insure mpeg power off
	//ret = ioctl(mcu_fd, MPEG_POWER, &value);
	//DEBUG_LOGI("mpeg_power_disable\r\n");	
	usleep(30000);
	//value = 1;	// power on mpeg
	//ret = ioctl(mcu_fd, MPEG_POWER, &value);
	//DEBUG_LOGI("mpeg_reset_enable\r\n");	
	//usleep(20000);
	value = 1;	// enable mpeg
	ret = ioctl(mcu_fd, MPEG_RESET, &value);	
	//DEBUG_LOGI("mpeg_power_enable\r\n");	
	close(mcu_fd);
	
	// initialize the sem and we will only use the sem in this process and init value is ZERO
	if(sem_init(&sem_count, 0, 0) < 0){
		DEBUG_LOGE("mpeg sem init ERROR!");
		return -1;
	}
	if(pthread_mutex_init(&mutex, NULL) < 0){
		DEBUG_LOGE("mpeg sem init ERROR!");
		return -1;
	}
	g_mpeg_handle = open( "/dev/ttymxc2", O_RDWR | O_NOCTTY | O_NONBLOCK);
	if(g_mpeg_handle == -1){
		DEBUG_LOGI("mpeg open fail\r\n");
		return -1;
	}
	struct termios  ios;
        tcgetattr( g_mpeg_handle, &ios );
        DEBUG_LOGI("mpeg set to 115200bps, 8-N-1");
        bzero(&ios, sizeof(ios));
        ios.c_cflag = B115200 | CS8 | CLOCAL | CREAD;
        ios.c_iflag = 0;
        ios.c_oflag = 0;
        ios.c_lflag = 0;  /* disable ECHO, ICANON, etc... */
        tcsetattr( g_mpeg_handle, TCSANOW, &ios );

	ret = pthread_create(&read_thread_info.thread_id, NULL , &read_thread_func,  &read_thread_info);
	if (ret != 0){
		DEBUG_LOGE("mpeg read thread create failed!\n");
		// TODO: close com port and release all the resource.	
	}
	return 0;
}

static int mpeg_device_close(struct hw_device_t *device)
{
    struct mpeg_device_t *dev = (struct mpeg_device_t*)device;
    if(dev){
        free(dev);
    }
    return 0;
}

static int mpeg_debug_level_setting(struct mpeg_device_t *dev, int *Debug_level){
	Mpeg_debug_level = *Debug_level;
	return 0;
}

static int mpeg_device_open(const struct hw_module_t* module, const char* name, struct hw_device_t** device)
{
    int status = -EINVAL;
    struct mpeg_device_t *dev;
    DEBUG_LOGI("mpeg_device_open\n");
    dev = (struct mpeg_device_t*)malloc(sizeof(*dev));
    /* initialize our state here */
    memset(dev, 0, sizeof(*dev));
    /* initialize the procs */
    dev->common.tag = HARDWARE_DEVICE_TAG;
    dev->common.version = 0;
    dev->common.module = const_cast<hw_module_t*>(module);
    dev->common.close = mpeg_device_close; 
    dev->mpeg_open = mpeg_open;
    dev->mpeg_get_notification = mpeg_get_notification;
    dev->mpeg_send_command = mpeg_send_command;
	dev->mpeg_debug_Level_setting = mpeg_debug_level_setting;
    *device = &dev->common;
    status = 0;
    return status;
}

