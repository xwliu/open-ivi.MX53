
/*
 * Copyright (C) 2008 The Android Open Source Project
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

#ifndef __TRACTOR_MPEG_PROTOCAL_H
#define __TRACTOR_MPEG_PROTOCAL_H

#define MPEG_PROTOCAL_FRAME_START_BYTE_ONE 0x4B
#define MPEG_PROTOCAL_FRAME_START_BYTE_TWO 0xB4

#define MAX_COMMAND_BYTE_LENGTH 256

typedef enum{
 MPEG_HOST_IND		       	= 0x80,
 MPEG_INIT_CFM					= 0x81,
 MPEG_DEVICE_STATUS_CFM		= 0x82,
 MPEG_DEVICE_STATUS_IND		= 0x84,
 MPEG_KEY_CONTROL_CFM		= 0x85,
 MPEG_KEY_IND					= 0x86,
 MPEG_TS_POS_IND				= 0x88,
 MPEG_PLAY_STATUS_RSP			= 0x89,
 MPEG_PLAY_STATUS_IND			= 0x8A,
 MPEG_PLAY_INFO_RSP			= 0x8B,
 MPEG_PLAY_INFO_IND			= 0x8C,
 MPEG_ID3_INFO_RSP			= 0x8D,
 MPEG_ID3_INFO_IND				= 0x8E,
 MPEG_PLAYLIST_INDEX_RSP		= 0x8F,
 MPEG_PLAYLIST_INDEX_INFO		= 0x90,
 MPEG_PLAYLIST_NAME_RSP		= 0x91,
 MPEG_DVD_DISP_INFO_IND		= 0x92,
 MPEG_DVD_DISP_INFO_RSP		= 0x93,
 MPEG_SOURCE_CFM				= 0x95,
 MPEG_SET_SUSPEND_CFM		= 0x96,
 MPEG_POWER_OFF_CFM				= 0x97,
 MPEG_GET_BOOKMARK				= 0x98,
 MPEG_COMM_STATUS			= 0x9F,
 MPEG_UPGRADE_START			= 0xA0,
 MPEG_UPGRADE_END				= 0xA1,
 MPEG_DIVX_CODE_CFM				= 0xA3,
 MPEG_DIVX_CODE_IND			= 0xA4,
 MPEG_DBG_MSG				= 0xF0,
 MPEG_DISC_STATUS			= 0xE0,
}mpeg_report_type;

typedef struct{
	int dummy;
}mpeg_host_init_report;

typedef struct{
	int version;
}mpeg_init_cfm_report;

/*
typedef enum {
	MPEG_DISC_NONE		= 0x00,
	MPEG_DISC_NORMAL	= 0x01,
	MPEG_DISC_BAD		= 0x02,
}mpeg_disc_status;

typedef enum {
	MPEG_SPINDLE_STOP	= 0x00,
	MPEG_SPINDLE_ROTATE	= 0x01,
}mpeg_spindle_status;

typedef enum{
	MPEG_VIDEO_NONE		= 0x00,
	MPEG_VIDEO_MODE		= 0x01,
}mpeg_video_mode;

typedef enum{
	MPEG_SUPPORT_FOUND	= 0x00,
	MPEG_SUPPORT_NONE	= 0x01,
}mpeg_support_file_status;

typedef enum{
	MPEG_DVD_NONEROOTMENU	= 0x00,
	MPEG_DVD_ROOTMENU	= 0x01,
}mpeg_dvd_root_menu_status;
*/


typedef struct{
	char source_off;
}mpeg_set_mpeg_source_cfm_report;

typedef struct{
	int	filefolder_index;
	int   filename[256];	// ?? how long it will be for the file name??
}mpeg_playlist_name_ind_report;

typedef struct{
	int isSucc;
}mpeg_key_control_cfm_report;


//following struct is used in mpeg_playlist_name_cfm_report
typedef struct{
	int is_file;
	int file_type;
	int parent_num;
	int file_index;
	char file_name[12];
}mpeg_file_msg;

typedef struct{
	int file_num;
	char file_msg[256];
}mpeg_playlist_name_cfm_report;

typedef struct{
	int	curr_frame_count;
	int   file_type[80];
	int	parent_index[80];
	int   frame_file_count;
}mpeg_playlist_index_ind_report;

typedef struct{
	int	isSucc;
	int	all_file_count;
	int	all_folder_count;
	int all_audio_folder_count;
	int all_video_folder_count;
	int all_photo_folder_count;
	int all_audio_file_count;
	int all_video_file_count;
	int all_photo_file_count;	
}mpeg_playlist_index_report;

typedef struct{
	int file_type;
	int curr_index;
	int all_index;
	int curr_playtime;
	int all_playtime;
	int curr_folder_index;
	int all_folder_count;
	int all_file_count;
}mpeg_play_info_report;

typedef struct{
	int 	disc_type;
	int 	play_status;
	int 	repeat_mode;
	int 	shuffle_mode;
	int 	scan_mode;
	int 	play_type;
}mpeg_play_status_report;

typedef struct{
	int		disc_status;
	int		spindle_status;
	int		video_mode;
	int		file_status;
	int		root_menu_status;
}mpeg_device_status_report;

typedef struct{
	char dbg_msg[256];
}mpeg_device_dbg_msg;

typedef struct{
	char disc_sts;
}mpeg_disc_status;

typedef struct{
	int audio_num;
	int audio_index;
	int audio_coding_type;
	int audio_channel_num;
	int audio_iso693_val;
	int subtitle_iso693_val;
	int subtitle_all_num;
	int subtitle_curr_index;
}mpeg_dvd_disp_info_report;

typedef struct{
	char id3_info[256];
}mpeg_id3_info_report;

typedef struct{
	int command_result;
}mpeg_set_suspend_report;

typedef struct{
	int command_result;
	char data[64];
}mpeg_power_off_cfm;

typedef struct{
}mpeg_get_bootmark_req;
typedef  union{
	mpeg_host_init_report				host_init_report;
	mpeg_init_cfm_report				init_cfm_report;
	mpeg_device_status_report			device_status_report;
	mpeg_play_status_report				play_status_report;
	mpeg_play_info_report				play_info_report;
	mpeg_playlist_index_report			play_index_report;
	mpeg_playlist_index_ind_report		play_index_ind_report;
	mpeg_playlist_name_cfm_report		play_name_cfm_report;
	mpeg_playlist_name_ind_report		play_name_ind_report;
	mpeg_key_control_cfm_report		key_control_cfm;
	mpeg_device_dbg_msg			device_dbg_msg;
	mpeg_set_mpeg_source_cfm_report		set_mpeg_source_cfm_report;
	mpeg_disc_status			disc_status;
	mpeg_dvd_disp_info_report	dvd_disp_info_report;
	mpeg_id3_info_report		id3_info_report;
	mpeg_set_suspend_report		set_suspend_report;
	mpeg_power_off_cfm			power_off_cfm;
	mpeg_get_bootmark_req		get_bootmark_req;
}mpeg_report_context;

typedef struct {
	mpeg_report_type 		the_report_type;
	mpeg_report_context		the_report_context;
	//int 				notification_size;
}mpeg_notification;

//==================================================================================================================


typedef enum{
 MPEG_INIT_REQ		        = 0x01,
 MPEG_DEVICE_STATUS_REQ		= 0x02,
 MPU_REPORT_STATUS_REQ		= 0x03,
 MPEG_KEY_CONTROL_REQ		= 0x05,
 MPU_REPORT_TS_POS_REQ		= 0x07,
 MPEG_PLAY_STATUS_REQ		= 0x09,
 MPEG_PLAY_INFO_REQ		= 0x0B,
 MPEG_ID3_INFO_REQ		= 0x0D,
 MPEG_PLAYLIST_INDEX_REQ	= 0x0F,
 MPEG_PLAYLIST_NAME_REQ	        = 0x11,
 MPEG_DVD_DISP_INFO_REQ	        = 0x13,
 MPU_SET_MPEG_SOURCE_REQ	= 0x15,
 MPU_SET_MPEG_SUSPEND_REQ	= 0x16,
 MPEG_SET_MPEG_POWER_OFF_REQ		=0x17,
 MPU_RET_BOOTMARK					=0x18,
 MPEG_UPGRADE_START_REQ		= 0x20,
 MPEG_VERSION_REQ		= 0x22,
 MPEG_DIVX_CODE_REQ		= 0x23,
 MPEG_COMM_STATUS_REQ		= 0X30,
 MPEG_RESET_REQ			= 0xFE
}mpeg_command_type;

typedef struct{
	char mpu_version;
}mpeg_init_req_command;

typedef struct{
}mpeg_device_status_req_command;

typedef struct{
	char disc_status;
}mpeg_device_status_status_req_command;

typedef struct{
	char key_type;
	char param[4];
}mpeg_key_control_req_command;

typedef struct{
	char op_status;
	char X_range[2];
	char Y_range[2];
}mpu_report_ts_req_command;

typedef struct{
}mpeg_play_status_req_command;

typedef struct{
}mpeg_play_info_req_command;

typedef struct{
}mpeg_id3_info_req_command;

typedef struct{
}mpeg_playlist_index_req_command;

typedef struct{
	char file_folder_ind;
	char index_start_num[2];
}mpeg_playlist_name_req_command;

typedef struct{
}mpeg_dvd_disp_info_req_command;

typedef struct{
	char source_off;
}mpeg_set_mpeg_source_req_command;

typedef struct{
}mpeg_upgrade_start_req_command;

typedef struct{
}mpeg_version_req_command;

typedef struct{
}mpeg_divx_code_req_command;

typedef struct{
	char comm_status;
}mpeg_comm_status_cfm_req_command;

typedef struct{
	char command_type;
}mpeg_set_suspend_command;

typedef struct{
}mpu_req_mpeg_power_off_command;

typedef struct{
	char bootmark_status;
	char bootmark_data[64];
}mpu_ret_bootmark_cfm;
typedef  union{
	mpeg_init_req_command			init_req_command;
	mpeg_device_status_req_command		device_status_req_command;
	mpeg_device_status_status_req_command 	device_status_status_req_command;
	mpeg_key_control_req_command 		key_control_req_command;
	mpu_report_ts_req_command 		report_ts_req_command;
	mpeg_play_status_req_command 		play_status_req_command;
	mpeg_play_info_req_command 		play_info_req_command;
	mpeg_id3_info_req_command 		id3_info_req_command;
	mpeg_playlist_index_req_command		playlist_index_req_command;
	mpeg_playlist_name_req_command 		playlist_name_req_command;
	mpeg_dvd_disp_info_req_command	 	dvd_disp_info_req_command;
	mpeg_set_mpeg_source_req_command	set_mpeg_source_req_command;
	mpeg_upgrade_start_req_command		upgrade_start_req_command;
	mpeg_version_req_command		version_req_command;
	mpeg_divx_code_req_command		divx_code_req_command;
	mpeg_comm_status_cfm_req_command	comm_status_cfm_req_command;
	mpeg_set_suspend_command		set_suspend_command;
	mpu_req_mpeg_power_off_command  req_mpeg_power_off_command;
	mpu_ret_bootmark_cfm			ret_bootmark_cfm;
}mpeg_command_context;


typedef struct {
	mpeg_command_type		the_command_type;
	mpeg_command_context	the_command_context;
}mpeg_command;


#endif
