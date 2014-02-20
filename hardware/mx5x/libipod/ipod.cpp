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
#define  LOG_TAG  "ipod"
#include <hardware/hardware.h>
#include <hardware/ipodhal.h>

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
#include <hardware/ipodhal.h>
#include <sys/queue.h>
#include <semaphore.h>

// following data is the main data we are using...
static int g_ipod_handle = 0;
struct thread_info 
{    /* Used as argument to thread_start() */
     pthread_t thread_id;        /* ID returned by pthread_create() */
     char     *argv_string;      /* not used right now */
};
thread_info read_thread_info;
thread_info parse_thread_info;

// following data is using in the read thread to capture a complete command
#define MAX_READ_BYTE_ONCE	256
#define MAX_BUFFER_STRING   	512
// the extra 16 bytes should never be used, as we just enlarge the buffer for safe...
static unsigned char tty_buffer[MAX_BUFFER_STRING + 16] = {0};
static unsigned char tmp_buffer[MAX_BUFFER_STRING + 16] = {0};

static int tty_buffer_pos = 0;
sem_t sem_count;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER; 

struct ipod_notification_node{
    ipod_notification a_ipod_notification;
    TAILQ_ENTRY(ipod_notification_node) pointers;
};

TAILQ_HEAD(tail_head, ipod_notification_node);

tail_head ipod_head;

void dump_buffer(void)
{
	LOGW("++++++++++++++IPOD MAIN BUFFER DUMP+++++POS: 0x%d++++++\r\n", tty_buffer_pos);
	for(int i = 0; i < MAX_BUFFER_STRING; i+=8){
		LOGW("ipod <0x%02x> <0x%02x> <0x%02x> <0x%02x> <0x%02x> <0x%02x> <0x%02x> <0x%02x>\n", 
			tty_buffer[i], tty_buffer[i+1], tty_buffer[i+2], tty_buffer[i+3],
			tty_buffer[i+4], tty_buffer[i+5], tty_buffer[i+6], tty_buffer[i+7]);

	}
	LOGW("--------------IPOD MAIN BUFFER DUMP---------------\r\n");
}

void dump_read_buffer(char* p, int size)
{
	LOGW("++++++++++++++IPOD READ BUFFER DUMP+++++size: 0x%d++++++", size);
	for(int i = 0; i < MAX_READ_BYTE_ONCE; i+=8){
		//LOGW("ipod 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x\n", 
			LOGI("ipod 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x\n", 
			p[i], p[i+1], p[i+2], p[i+3],
			p[i+4], p[i+5], p[i+6], p[i+7]);

	}
	LOGW("--------------IPOD READ BUFFER DUMP---------------");
}

// add the new notification into the list end
static int push_ipod_notificaiton(ipod_notification* p_new_ipod_notificaiton)
{
	// step1. create a new ipod_notification and assign right value
	int ret = 0;	
	ipod_notification_node* p_new_node = (ipod_notification_node*)malloc(sizeof(ipod_notification_node));
	if(p_new_node == NULL){
		LOGE("ipod allocate memory err!!\n");
		ret = -ENOMEM;
	}
	memcpy(&p_new_node->a_ipod_notification, p_new_ipod_notificaiton, sizeof(ipod_notification));
	// step2. push the ipod_notificaion into queue 
	pthread_mutex_lock(&mutex);
	TAILQ_INSERT_TAIL(&ipod_head, p_new_node, pointers);
	pthread_mutex_unlock(&mutex);
	// step3. increase the sem, after this, if there any thread want to read the data, it can continue....
	sem_post(&sem_count);
	LOGI("ipod Command Inqueue SEM++\n");
	return ret;
}

// got the first notification in the list and delete it from the list
static int pop_ipod_notificaiton(ipod_notification* p_pop_ipod_notificaition)
{
	int ret = 0;
	ipod_notification_node* temp = NULL;
	// if we do not have data in the queue, the sem_wait should never return and wait here
	sem_wait(&sem_count);
	LOGI("ipod Command OutQueue SEM--\n");
	// if we get here, this means there is any ipod notification in the queue, so read it out and pass to up level
	pthread_mutex_lock(&mutex);
	temp = ipod_head.tqh_first;
	if(temp == NULL){
		LOGE("ipod Queue NULL while sem get access");
		// TODO: destroy all related resource
	}
	memcpy(p_pop_ipod_notificaition, &(temp->a_ipod_notification), sizeof(ipod_notification));
	TAILQ_REMOVE(&ipod_head, ipod_head.tqh_first, pointers);
	pthread_mutex_unlock(&mutex);
	free(temp);
	return ret;
}

static int ipod_get_notification(struct ipod_device_t *dev, ipod_notification* p_notification)
{
	pop_ipod_notificaiton(p_notification);
	return 0;
}

static unsigned char ipod_calc_checksum(unsigned char *data, int len)
{
	char checksum = 0;
	int i;

	for(i=0; i<len; i++){
		checksum += data[i];
	}
	checksum = (~checksum) + 1;

	LOGI("ipod_calc_checksum:0x%x, first data: 0x%x, len: %d\n", checksum, *data, len);

	return checksum;
}

void parse_command(unsigned char* command)
{
	ipod_notification my_notification;
	unsigned int length = command[0];
	unsigned int checksum = ipod_calc_checksum(command, length + 1);
	if(command[length + 1] != checksum){
		LOGE("ipod checksum error! 0x%x VS 0x%x\n", command[length + 1], checksum);
		//LOGE("ipod command[0]: 0x%x, command[length -1]: 0x%x, command[length]: 0x%x, command[length+1]: 0x%x\n", command[0], command[length - 1], command[length], command[length+1]);
	}
	int interface_id = command[1];
	switch(interface_id){
		case GENERAL_LINGO_TYPE:
			my_notification.the_report_type = (ipod_report_type)command[2];
			switch(my_notification.the_report_type){
			case GENERAL_ACK:
				my_notification.the_report_context.ack_report_general.command_result = command[3];
				my_notification.the_report_context.ack_report_general.command_id= command[4];                        
				if(command[3] == 0x6){   // command pending ack using different struct
					my_notification.the_report_context.ack_report_general.command_wait_time = (command[5] << 24) | (command[6] << 16) | (command[7] << 8) | (command[8]);
					LOGI("ipod wait time set: %d\n", my_notification.the_report_context.ack_report_general.command_wait_time);                    
				}else{
					my_notification.the_report_context.ack_report_general.command_wait_time = 0;
				}
				break;
			case GENERAL_RETURN_REMOTE_UI_MODE:
				my_notification.the_report_context.remote_ui_general.mode = command[3];
				LOGI("ipod Current Mode %d\n", my_notification.the_report_context.remote_ui_general.mode);           
				break;                     
			default:
				return;                
			}
			break;
		case EXTENDE_INTERFACE_MODE_TYPE:
			my_notification.the_report_type = (ipod_report_type)((command[2] << 8) | (command[3]));                
			switch(my_notification.the_report_type){
			case EIM_ACK:
				my_notification.the_report_context.ack_report_eim.command_result = command[4];
				my_notification.the_report_context.ack_report_eim.command_id= ((command[5] << 8) | (command[6]));                        
				break;
			case EIM_RETURN_IPOD_NAME:
				// TODO: handle large packet...                
				my_notification.the_report_context.ipod_name_report.length = length - 3;
				memcpy(&(my_notification.the_report_context.ipod_name_report.ipod_name[0]), &command[4], length-3);
				break;
			case EIM_RETURN_NUMBER_CATEGORIZEDDBRECORDS:
				my_notification.the_report_context.number_dbrecords_report.record_count = (command[4] << 24) | (command[5] << 16) | (command[6] << 8) | (command[7]);
				break;
			case EIM_RETURN_CATEGORIZEDDBRECORD:
				// TODO: handle large packet...                                
				my_notification.the_report_context.categorized_dbrecord_report.record_index = (command[4] << 24) | (command[5] << 16) | (command[6] << 8) | (command[7]); 
				my_notification.the_report_context.categorized_dbrecord_report.length = length - 7;
				memset(&(my_notification.the_report_context.categorized_dbrecord_report.record_string[0]), 0, 256);                
				memcpy(&(my_notification.the_report_context.categorized_dbrecord_report.record_string[0]), &command[8], length-7);
				break;
			case EIM_RETURN_PLAY_STATUS:
				my_notification.the_report_context.playstatus_report.track_total_time = (command[4] << 24) | (command[5] << 16) | (command[6] << 8) | (command[7]); 
				my_notification.the_report_context.playstatus_report.track_current_position = (command[8] << 24) | (command[9] << 16) | (command[10] << 8) | (command[11]);
				my_notification.the_report_context.playstatus_report.play_status = command[12]; 
				break;
			case EIM_RETURN_CURRENT_PLAYING_TRACK_INDEX:
				my_notification.the_report_context.playing_track_report.track_playing_index = (command[4] << 24) | (command[5] << 16) | (command[6] << 8) | (command[7]);                 
				break;
			case EIM_RETURN_CURRENT_PLAYING_TRACK_TITLE:
				// TODO: handle large packet...                                                
				my_notification.the_report_context.playing_track_title_report.length = length -3;
				memcpy(&(my_notification.the_report_context.playing_track_title_report.track_title[0]), &command[4], length-3);
				break;
			case EIM_RETURN_CURRENT_PLAYING_TRACK_ARTIST:
				// TODO: handle large packet...                                                
				my_notification.the_report_context.playing_track_artist_report.length = length -3;
				memcpy(&(my_notification.the_report_context.playing_track_artist_report.track_artist[0]), &command[4], length-3);                
				break;
			case EIM_RETURN_CURRENT_PLAYING_TRACK_ALBUM:
				// TODO: handle large packet...                                                
				my_notification.the_report_context.playing_track_album_report.length = length -3;				
				memcpy(&(my_notification.the_report_context.playing_track_album_report.track_album[0]), &command[4], length-3);                
				break;
			case EIM_PLAYSTATUS_CHANGENOTIFICATION:
				{
					unsigned char type = command[4];
					my_notification.the_report_context.play_status_change_report.play_change_type = type;
					LOGI("EIM_PLAYSTATUS_CHANGENOTIFICATION: %d\n", type);
					switch(type){
					case 0x06:  // PLAY STATUS CHANGE
						my_notification.the_report_context.play_status_change_report.play_value = command[5];
						LOGI("EIM_PLAYSTATUS_CHANGENOTIFICATION STATUS: %d\n", command[5]);                        
						break;
					case 0x01:  // PLAY TRACK INDEX CHANGE
						my_notification.the_report_context.play_status_change_report.play_value = (command[5] << 24) | (command[6] << 16) | (command[7] << 8) | (command[8]);
						LOGI("EIM_PLAYSTATUS_CHANGENOTIFICATION TRACK index: %d\n", my_notification.the_report_context.play_status_change_report.play_value);                                                
						break;                        
					default:
						return; // can not support, the notification will be throw...
					}
				}
				break;
			case EIM_RETURN_SHUFFLE:
				my_notification.the_report_context.shuffle_report.shuffle = command[4]; 
				break;
			case EIM_RETURN_REPEAT:
				my_notification.the_report_context.repeat_report.repeat = command[4];                 
				break;
			default:
				return;
			}
			break;
	}
	push_ipod_notificaiton(&my_notification);
}


void check_full_command(void)
{
	unsigned char* p_head = NULL;
	//dump_buffer();
	if(tty_buffer_pos == 0){
		LOGE("ipod check command but buffer pos is ZERO\r\n");
		return;
	}
	int search_byte = tty_buffer_pos;
	unsigned char* p_start = &tty_buffer[0];
	unsigned char command[MAX_COMMAND_BYTE_LENGTH] = {0};
	do{
		//LOGW("tty_buffer_pos: %d\r\n", tty_buffer_pos);
		p_head = (unsigned char*)memchr(p_start, IPOD_PROTOCAL_FRAME_START_BYTE_ONE, search_byte);
		if(p_head == NULL){
			LOGE("ipod No head found, clean buffer pos to ZERO\r\n");
			// as we have search all the buffer and do not found any head bytes, it's ok just return back to the start position in the main buffer
			tty_buffer_pos = 0;
			return;
		}
		// 0xFF is last valid byte, so we copy the 0xFF to the buffer head and move the counter to the next position
		// So if later we have 0x55 be copy into the buffer, we won't miss this command.
		if((p_head - &tty_buffer[0] + 1) == tty_buffer_pos){
			LOGE("ipod Find HEAD FIRST BYTE with Last Byte\r\n");
			memcpy(&tty_buffer[0], p_head, 1);
			tty_buffer_pos = 1;
			return;
		}
		// if we found two continue bytes 0xFF55
		else if((*p_head == IPOD_PROTOCAL_FRAME_START_BYTE_ONE) && ((char)*(p_head+1) == (char)IPOD_PROTOCAL_FRAME_START_BYTE_TWO)){
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
				int length = *(p_head + 2);
				// at this point, the tty_buffer_point will indicate the last valid position...
				if(length == 0){
					LOGE("ipod Large packet format found!\r\n");
					// TODO: MUST take care this condition otherwise we may meet critical issue
					return;
				}
				int left_bytes = tty_buffer_pos - (((p_head + 2) - &tty_buffer[0]) + 1);
				if((length +1) > left_bytes){
					//LOGW("ipod length bigger than left_bytes, we should wait next buffer read...");
					//avoid to cover the buffer while do memcpy, 1. copy to temp, 2.copy to buffer,3. init temp
					memcpy(&tmp_buffer[0], p_head, left_bytes + 3);
					memcpy(&tty_buffer[0], &tmp_buffer[0], left_bytes + 3);
					memset(&tmp_buffer[0], 0, MAX_BUFFER_STRING+16);
					tty_buffer_pos = left_bytes + 3;
					return;
				}else {
					memset(&command[0], 0, MAX_COMMAND_BYTE_LENGTH);
					memcpy(&command[0], (p_head + 2), length + 1 + 1);  // one byte length, one byte checksum
					parse_command(&command[0]);
					memset(&tty_buffer[0], 0,  (((p_head + 2) - &tty_buffer[0] + 1) + length));
					//avoid to cover the buffer while do memcpy, 1. copy to temp, 2.copy to buffer,3. init temp
					memcpy(&tmp_buffer[0], (p_head + 2 + length + 1), left_bytes - length);
					memcpy(&tty_buffer[0], &tmp_buffer[0], left_bytes - length);
					memset(&tmp_buffer[0], 0, MAX_BUFFER_STRING+16);
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
			LOGE("ipod No continue head found, search next in current buffer\r\n");
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
	if(g_ipod_handle == 0){
		LOGE("ipod file handle is ZERO!\n");
	}
	LOGI("ipod read thread started...");	
	fd_set readfds;
	do {
		int read_bytes = 0;
		FD_ZERO(&readfds);
		FD_SET(g_ipod_handle, &readfds);
		ret = select(FD_SETSIZE, &readfds, NULL, NULL, NULL);
		if (ret < 0){
			LOGW("ipod select ttymxc0 got failed!\n");	
			continue;
		}
		//LOGW("ipod select ttymxc0 select return: %d\n", ret);
		memset(buf, 0, sizeof(buf));
		read_bytes = read(g_ipod_handle, buf, sizeof(buf) );
		// as we will wait at the select function if there is no data
		// if there is data but we can not read it, this should be critical error
		// so count 3 times is enough to handle such kind of case....
		if(read_bytes < 0){
			LOGW("ipod got bytes wrong!!! read_bytes: 0x%x, errno: 0x%x\n", read_bytes,  errno);
			retry_times++;
			if(retry_times > MAX_RETRY_TIMES){
				LOGE("ipod read bytes err!!!\n");
				break;
			}
			continue;
		}
		if(read_bytes <  MAX_READ_BYTE_ONCE){
			LOGI("ipod actual read bytes: %d", read_bytes);
		}
		//dump_read_buffer(buf, read_bytes);        
		// according to the MPU-MCU protocal, the max length for one command will never bigger than 2(one bytes) = 256 bytes length
		// and after each we cat the buf into the tty_buffer, we will check whether there is any complete command and clean the command buffer
		// so such kind of case won't be happen in fact...
		if((tty_buffer_pos + read_bytes) > MAX_BUFFER_STRING){
			LOGE("ipod tty buffer overflow! tty_buffer_pos: 0x%x, read_bytes: 0x%x\n", tty_buffer_pos, read_bytes);
			// TODO: reset buffer to handle this condition
		}
		// step1: copy the read buffer into the main buffer and increase the index
		memcpy(&tty_buffer[tty_buffer_pos], buf, read_bytes);
		tty_buffer_pos += read_bytes;
		// step2: check whether there is any complete command exist:
		check_full_command();
	} while (1);

	return NULL;
}

static int prepareBuffer(unsigned char* buffer, ipod_command* p_notification)
{
	unsigned char* p = buffer;
	// two heads byte
	*p = 0xFF;
	p++;
	*p = 0x55;
	p++;
	unsigned char* pLength = p;
	// Length: CMD+PARAM+CHECKSUM
	//*p = countall; will be assign the correct value until we know the command type
	p++;
	LOGI("prepareBuffer: command id: 0x%x", p_notification->the_command_type);    
	// interface
	switch(p_notification->the_command_type){
		case GENERAL_IDENTIFY:
			*p = GENERAL_LINGO_TYPE;  //general type
			p++;
			*p = p_notification->the_command_type;    // command type			
			p++;
			*p = EXTENDE_INTERFACE_MODE_TYPE;
			p++;
			*pLength = 0x03;            
			break;         
		case GENERAL_REQUEST_REMOTEUIMODE:
		case GENERAL_ENTER_REMOTEUIMODE:
		case GENERAL_EXIT_REMOTEUIMODE:
			*p = GENERAL_LINGO_TYPE;  //general type
			p++;
			*p = p_notification->the_command_type;    // command type
			*pLength = 0x2; // fix for these three command
			p++;
			break;            
		default:
			*p = EXTENDE_INTERFACE_MODE_TYPE;   // entended 
			p++;
			*p = ((p_notification->the_command_type) & 0x0000FF00) >> 8;
			p++;            
			*p = ((p_notification->the_command_type) & 0x000000FF);            
			p++;
			*pLength = 0x03;  // for all command without extra info
			switch(p_notification->the_command_type){
				case EIM_REQUEST_IPODNAME:
					break; // nothing to do...
				case EIM_RESET_DBSELECTION:
					break; // nothing to do...
				case EIM_SELECTDBRECORD:
					*p = p_notification->the_command_context.select_db_record_command.category_type;
					p++;
					*p = (p_notification->the_command_context.select_db_record_command.record_index & 0xFF000000) >> 24;
					p++;                    
					*p = (p_notification->the_command_context.select_db_record_command.record_index & 0x00FF0000) >> 16;
					p++;                    
					*p = (p_notification->the_command_context.select_db_record_command.record_index & 0x0000FF00) >> 8;
					p++;                    
					*p = (p_notification->the_command_context.select_db_record_command.record_index & 0x000000FF);
					p++;
					*pLength = 0x08;
					break;                    
				case EIM_GET_NUMBERCATEGORIZEDDBRECORDS:
					*p = p_notification->the_command_context.get_number_categorized_dbrecords_command.category_type;
					p++;
					*pLength = 0x04;                    
					break;                    
				case EIM_RETRIVE_CATEGORIZEDDATABASERECORDS:
					*p = p_notification->the_command_context.categorized_dbrecords_command.category_type;
					p++;
					*p = (p_notification->the_command_context.categorized_dbrecords_command.record_start & 0xFF000000) >> 24;
					p++;                    
					*p = (p_notification->the_command_context.categorized_dbrecords_command.record_start & 0x00FF0000) >> 16;
					p++;                    
					*p = (p_notification->the_command_context.categorized_dbrecords_command.record_start & 0x0000FF00) >> 8;
					p++;                    
					*p = (p_notification->the_command_context.categorized_dbrecords_command.record_start & 0x000000FF);
					p++;                    
					*p = (p_notification->the_command_context.categorized_dbrecords_command.record_count & 0xFF000000) >> 24;
					p++;                    
					*p = (p_notification->the_command_context.categorized_dbrecords_command.record_count & 0x00FF0000) >> 16;
					p++;                    
					*p = (p_notification->the_command_context.categorized_dbrecords_command.record_count & 0x0000FF00) >> 8;
					p++;                    
					*p = (p_notification->the_command_context.categorized_dbrecords_command.record_count & 0x000000FF);
					p++;                       
					*pLength = 0x0C;                                        
					break;                    
				case EIM_GET_PLAYSTATUS:
					break; // nothing to do...
				case EIM_GET_CURRENTPLAYINGTRACKINDEX:
					break; // nothing to do...                
				case EIM_GET_INDEXEDPLAYINGTRACKTITLE:
					*p = (p_notification->the_command_context.get_indexed_playing_tract_title_command.playback_track_index & 0xFF000000) >> 24;
					p++;                    
					*p = (p_notification->the_command_context.get_indexed_playing_tract_title_command.playback_track_index & 0x00FF0000) >> 16;
					p++;                    
					*p = (p_notification->the_command_context.get_indexed_playing_tract_title_command.playback_track_index & 0x0000FF00) >> 8;
					p++;                    
					*p = (p_notification->the_command_context.get_indexed_playing_tract_title_command.playback_track_index & 0x000000FF);
					p++;          
					*pLength = 0x07;                    
					break;                    
				case EIM_GET_INDEXEDPLAYINGTRACKARTIST:
					*p = (p_notification->the_command_context.get_indexed_playing_track_artist_command.playback_track_index & 0xFF000000) >> 24;
					p++;                    
					*p = (p_notification->the_command_context.get_indexed_playing_track_artist_command.playback_track_index & 0x00FF0000) >> 16;
					p++;                    
					*p = (p_notification->the_command_context.get_indexed_playing_track_artist_command.playback_track_index & 0x0000FF00) >> 8;
					p++;                    
					*p = (p_notification->the_command_context.get_indexed_playing_track_artist_command.playback_track_index & 0x000000FF);
					p++;                    
					*pLength = 0x07;
					break;                                     
				case EIM_GET_INDEXEDPLAYINGTRACKALBUM:
					*p = (p_notification->the_command_context.get_indexed_playing_track_album_command.playback_track_index & 0xFF000000) >> 24;
					p++;                    
					*p = (p_notification->the_command_context.get_indexed_playing_track_album_command.playback_track_index & 0x00FF0000) >> 16;
					p++;                    
					*p = (p_notification->the_command_context.get_indexed_playing_track_album_command.playback_track_index & 0x0000FF00) >> 8;
					p++;                    
					*p = (p_notification->the_command_context.get_indexed_playing_track_album_command.playback_track_index & 0x000000FF);
					p++;                                        
					*pLength = 0x07;
					break;      
				case EIM_SET_PLAYSTATUSCHANGENOTIFICATION:
					// cause some ipod do not support advanced notification set, we have to roll back to using old notification set type.                    
					//*p = (p_notification->the_command_context.set_play_status_change_notification.enableMask & 0xFF000000) >> 24;
					//p++;                    
					//*p = (p_notification->the_command_context.set_play_status_change_notification.enableMask & 0x00FF0000) >> 16;
					//p++;                    
					//*p = (p_notification->the_command_context.set_play_status_change_notification.enableMask & 0x0000FF00) >> 8;
					//p++;                    
					//*p = (p_notification->the_command_context.set_play_status_change_notification.enableMask & 0x000000FF);
					//p++;                                        
					//*pLength = 0x07;           
					*p = 0x1;   // this will notification track time/track index change/track stop which lost the play/pause change that we have to poll...
					p++;
					*pLength = 0x04;                    
					break;                    
				case EIM_PLAY_CURRENTSELECTION:
					*p = (p_notification->the_command_context.play_current_selection_command.playback_track_index & 0xFF000000) >> 24;
					p++;                    
					*p = (p_notification->the_command_context.play_current_selection_command.playback_track_index & 0x00FF0000) >> 16;
					p++;                    
					*p = (p_notification->the_command_context.play_current_selection_command.playback_track_index & 0x0000FF00) >> 8;
					p++;                    
					*p = (p_notification->the_command_context.play_current_selection_command.playback_track_index & 0x000000FF);
					p++;                                                            
					*pLength = 0x07;
					break;                    
				case EIM_PLAY_CONTROL:
					*p = p_notification->the_command_context.play_control_command.playback_control;
					p++;                    
					*pLength = 0x04;
					break;                    
				case EIM_GET_SHUFFLE:
					break; // nothing to do...                    
				case EIM_SET_SHUFFLE:
					*p = p_notification->the_command_context.set_shuffle_command.shuffle_mode;
					p++;                    
					*p = p_notification->the_command_context.set_shuffle_command.restore_after_detach;
					p++;                                        
					*pLength = 0x05;
					break;                    
				case EIM_GET_REPEAT:
					break; // nothing to do...                    
				case EIM_SET_REPEAT:
					*p = p_notification->the_command_context.set_repeat_command.repeat_mode;
					p++;                    
					*p = p_notification->the_command_context.set_repeat_command.restore_after_detach;
					p++;                                         
					*pLength = 0x05;                    
					break;                    
			}
			break;            
	}
	// calculate the checksum from the payload bytes exclude the sync byte
	*p = ipod_calc_checksum(pLength, (*pLength) + 1);
	return ((*pLength) + 4);	// the command total length is base on the length + sync(two byte) + length byte + checksum byte
}

static int ipod_send_command(struct ipod_device_t *dev, ipod_command* p_notification)
{
	unsigned char length= 0;
	unsigned char command_buffer[MAX_READ_BYTE_ONCE] = {0};
	length = prepareBuffer(command_buffer, p_notification);
	if(p_notification->the_command_type == GENERAL_IDENTIFY){
		unsigned char sync = 0xFF;
		tcflush(g_ipod_handle, TCIOFLUSH);  // for the first command, we have to flush all the data
		write(g_ipod_handle, &sync, 1);
		sleep(1);   // according to ipod spec, we have to wait at least 20ms, but for actually test, 1 second seems a safe value or we may stuck at the first command
	}
	{
		//int i = 0;
		//LOGI("######ipod_send_command######");
		//for(i = 0; i < length; i++){
		//	LOGI(" [0x%x ]", command_buffer[i]);
		//	
		//}	
		//LOGI("@@@@@ipod_send_command@@@@@");
	}
	write(g_ipod_handle, command_buffer, length);
	if(p_notification->the_command_type == GENERAL_IDENTIFY){
		sleep(1);
	}
    
	return 0;
}

// following function is the standard hal function and structure
static int ipod_device_open(const struct hw_module_t* module, const char* name, struct hw_device_t** device);

static struct hw_module_methods_t ipod_module_methods = {
    open: ipod_device_open
};

struct ipod_module_t HAL_MODULE_INFO_SYM = {
	common: {
		tag: HARDWARE_MODULE_TAG,
		version_major: 1,
		version_minor: 0,
		id: IPOD_HARDWARE_MODULE_ID,
		name: "ipod module",
		author: "TokenWireless",
		methods: &ipod_module_methods,
	},
	/* module api interface */
};

/*!
 * This function will be the init point for the ipod function where it could work 
 * It will initialize the ttymxc0(UART0) with suitable parameter setting
 * Then it will start the two thread to do the major job
 * the read thread will continue read the buffer from the ttymxc0 if there is any data inside ttymxc0
 * it will then do a quick parse and put any known command into queue and increment the sem
 * after that, if any of the thread is try to read the event from queue, we will make the upper thread get the queue data.
 */
static int ipod_open(struct ipod_device_t *dev)
{
	int ret = 0;
	LOGI("ipod_open\r\n");
	// initialize the QUEUE we need to save the ipod notification
	TAILQ_INIT(&ipod_head);
	
	// initialize the sem and we will only use the sem in this process and init value is ZERO
	if(sem_init(&sem_count, 0, 0) < 0){
		LOGE("ipod sem init ERROR!");
		return -1;
	}
	if(pthread_mutex_init(&mutex, NULL) < 0){
		LOGE("ipod sem init ERROR!");
		return -1;
	}
	g_ipod_handle = open( "/dev/ttymxc0", O_RDWR | O_NOCTTY | O_NONBLOCK);
	if(g_ipod_handle == -1){
		LOGI("ipod open fail\r\n");
		return -1;
	}
	struct termios  ios;
        tcgetattr( g_ipod_handle, &ios );
        LOGI("ipod set to 57600bps, 8-N-1");
        bzero(&ios, sizeof(ios));        
        ios.c_cflag = B57600 | CS8 | CLOCAL | CREAD;
        ios.c_iflag = 0;
        ios.c_oflag = 0;
        ios.c_lflag = 0;  /* disable ECHO, ICANON, etc... */
        tcsetattr( g_ipod_handle, TCSANOW, &ios );
        tcflush(g_ipod_handle , TCIFLUSH);

	ret = pthread_create(&read_thread_info.thread_id, NULL , &read_thread_func,  &read_thread_info);
	if (ret != 0){
		LOGE("ipod read thread create failed!\n");
		// TODO: close com port and release all the resource.	
	}
	return 0;
}

static int ipod_device_close(struct hw_device_t *device)
{
    struct ipod_device_t *dev = (struct ipod_device_t*)device;
    if(dev){
        free(dev);
    }
    return 0;
}

static int ipod_device_open(const struct hw_module_t* module, const char* name, struct hw_device_t** device)
{
    int status = -EINVAL;
    struct ipod_device_t *dev;
    LOGI("ipod_device_open\n");
    dev = (struct ipod_device_t*)malloc(sizeof(*dev));
    /* initialize our state here */
    memset(dev, 0, sizeof(*dev));
    /* initialize the procs */
    dev->common.tag = HARDWARE_DEVICE_TAG;
    dev->common.version = 0;
    dev->common.module = const_cast<hw_module_t*>(module);
    dev->common.close = ipod_device_close; 
    dev->ipod_open = ipod_open;
    dev->ipod_get_notification = ipod_get_notification;
    dev->ipod_send_command = ipod_send_command;
    *device = &dev->common;
    status = 0;
    return status;
}

