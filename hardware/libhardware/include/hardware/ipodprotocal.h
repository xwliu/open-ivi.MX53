
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

#ifndef __TRACTOR_IPOD_PROTOCAL_H
#define __TRACTOR_IPOD_PROTOCAL_H

#define IPOD_PROTOCAL_FRAME_START_BYTE_ONE 0xFF
#define IPOD_PROTOCAL_FRAME_START_BYTE_TWO 0x55

#define MAX_COMMAND_BYTE_LENGTH 256

#define GENERAL_LINGO_TYPE                          0x00
#define EXTENDE_INTERFACE_MODE_TYPE     0x04

typedef enum{
        GENERAL_ACK = 0x02,
        GENERAL_RETURN_REMOTE_UI_MODE = 0x04,
        EIM_ACK = 0x0001,
        EIM_RETURN_IPOD_NAME = 0x0015,
        EIM_RETURN_NUMBER_CATEGORIZEDDBRECORDS = 0x0019,
        EIM_RETURN_CATEGORIZEDDBRECORD = 0x001B,        
        EIM_RETURN_PLAY_STATUS = 0x001D,
        EIM_RETURN_CURRENT_PLAYING_TRACK_INDEX = 0x001F,
        EIM_RETURN_CURRENT_PLAYING_TRACK_TITLE = 0x0021,
        EIM_RETURN_CURRENT_PLAYING_TRACK_ARTIST = 0x0023,        
        EIM_RETURN_CURRENT_PLAYING_TRACK_ALBUM = 0x0025,            
        EIM_PLAYSTATUS_CHANGENOTIFICATION = 0x0027,
        EIM_RETURN_SHUFFLE = 0x002D,                        
        EIM_RETURN_REPEAT = 0x0030,                                
}ipod_report_type;

typedef struct{
        unsigned char command_result;
        unsigned char command_id;
        unsigned int command_wait_time;
}general_ack_report;

typedef struct{
        unsigned char mode;
}general_return_remote_ui_report;

typedef struct{
        unsigned char command_result;
        unsigned char command_id;
}eim_ack_report;

// NOTE: according to the IPOD protocal, the ipod_name will may exceed 256 bytes limited
// But for tractor platform, we currently only support 256 byte long which means 128 UTF CHAR
// the other buffer will be ignore during parse
typedef struct{
        char ipod_name[256];
        unsigned char length;
}eim_ipod_name_report;

typedef struct{
        unsigned int record_count;
}eim_number_categorized_dbrecords_report;

// NOTE:  like the ipod name, we currently only support 256 bytes of the data
typedef struct{
        unsigned int record_index;
        char record_string[256];        
        unsigned char length;        
}eim_categorized_dbrecords_report;

typedef struct{
        unsigned char play_status;
        unsigned int track_current_position;
        unsigned int track_total_time;        
}eim_playstatus_report;

typedef  union{
        unsigned int track_playing_index;
}eim_current_playing_track_report;

typedef  union{
        char track_title[256];        
        unsigned char length;        
}eim_current_playing_track_title_report;

typedef  union{
        char track_artist[256];        
        unsigned char length;        
}eim_current_playing_track_artist_report;

typedef  union{
        char track_album[256];        
        unsigned char length;        
}eim_current_playing_track_album_report;

typedef struct{
        unsigned char play_change_type;
        unsigned int play_value;
}eim_play_change_notification_report;

typedef  union{
        unsigned char shuffle;        
}eim_shuffle_report;

typedef  union{
        unsigned char repeat;        
}eim_repeat_report;


typedef  union{
    general_ack_report                                            ack_report_general;
    general_return_remote_ui_report                      remote_ui_general;
    eim_ack_report                                                  ack_report_eim;
    eim_ipod_name_report                                      ipod_name_report;
    eim_number_categorized_dbrecords_report     number_dbrecords_report;
    eim_categorized_dbrecords_report                   categorized_dbrecord_report;
    eim_playstatus_report                                       playstatus_report;
    eim_current_playing_track_report                     playing_track_report;
    eim_current_playing_track_title_report             playing_track_title_report;
    eim_current_playing_track_artist_report           playing_track_artist_report;
    eim_current_playing_track_album_report         playing_track_album_report;
    eim_play_change_notification_report               play_status_change_report;
    eim_shuffle_report                                            shuffle_report;
    eim_repeat_report                                             repeat_report;
}ipod_report_context;

typedef struct {
        ipod_report_type                      the_report_type;
        ipod_report_context                 the_report_context;
        //int                                         notification_size;
}ipod_notification;
//==================================================================================================================
typedef enum{
        GENERAL_IDENTIFY    = 0x01,
        GENERAL_REQUEST_REMOTEUIMODE = 0x03,
        GENERAL_ENTER_REMOTEUIMODE = 0x05,
        GENERAL_EXIT_REMOTEUIMODE = 0x06,
        EIM_REQUEST_IPODNAME = 0x0014,
        EIM_RESET_DBSELECTION = 0x0016,
        EIM_SELECTDBRECORD = 0x0017,
        EIM_GET_NUMBERCATEGORIZEDDBRECORDS = 0x0018,
        EIM_RETRIVE_CATEGORIZEDDATABASERECORDS = 0x001A,
        EIM_GET_PLAYSTATUS = 0x001C,
        EIM_GET_CURRENTPLAYINGTRACKINDEX = 0x001E,
        EIM_GET_INDEXEDPLAYINGTRACKTITLE = 0x0020,
        EIM_GET_INDEXEDPLAYINGTRACKARTIST = 0x0022,
        EIM_GET_INDEXEDPLAYINGTRACKALBUM = 0x0024,
        EIM_SET_PLAYSTATUSCHANGENOTIFICATION = 0x0026,
        EIM_PLAY_CURRENTSELECTION = 0x0028,
        EIM_PLAY_CONTROL  = 0x0029,
        EIM_GET_SHUFFLE = 0x002C,
        EIM_SET_SHUFFLE = 0x002E,
        EIM_GET_REPEAT = 0x002F,
        EIM_SET_REPEAT  = 0x0031,
}ipod_command_type;

typedef struct{
        unsigned char support_lingo;
}general_identify_command;

typedef struct{
        unsigned char dummy;
}general_request_remote_ui_command;

typedef struct{
        unsigned char dummy;
}general_enter_remote_ui_command;

typedef struct{
        unsigned char dummy;    
}general_exit_remote_ui_command;

typedef struct{
        unsigned char dummy;    
}eim_request_ipod_command;

typedef struct{
        unsigned char dummy;    
}eim_reset_db_selection_command;

typedef struct{
        unsigned char category_type;    
        unsigned int record_index;
}eim_select_db_record_command;

typedef struct{
        unsigned char category_type;    
}eim_get_number_categorized_dbrecords_command;

typedef struct{
        unsigned char category_type;
        unsigned int record_start;
        unsigned int record_count;
}eim_categorized_dbrecords_command;

typedef struct{
        unsigned char dummy;    
}eim_get_play_status_command;

typedef struct{
        unsigned char dummy;    
}eim_get_current_playing_track_command;

typedef struct{
        unsigned int playback_track_index; 
}eim_get_indexed_playing_track_title_command;

typedef struct{
        unsigned int playback_track_index; 
}eim_get_indexed_playing_track_artist_command;

typedef struct{
        unsigned int playback_track_index; 
}eim_get_indexed_playing_track_album_command;

typedef struct{
        unsigned int enableMask;
}eim_set_play_status_change_notification;

typedef struct{
        unsigned int playback_track_index; 
}eim_play_current_selection_command;

typedef struct{
        unsigned char playback_control; 
}eim_play_control;


typedef struct{
        unsigned char dummy; 
}eim_get_shuffle_command;

typedef struct{
        unsigned char shuffle_mode; 
        unsigned char restore_after_detach;
}eim_set_shuffle_command;

typedef struct{
        unsigned char dummy; 
}eim_get_repeat_command;

typedef struct{
        unsigned char repeat_mode; 
        unsigned char restore_after_detach;
}eim_set_repeat_command;

typedef  union{
    general_identify_command                                               identify_command;
    general_enter_remote_ui_command                                 enter_remote_ui_command;
    general_exit_remote_ui_command                                   exit_remote_ui_command;
    eim_request_ipod_command                                             request_ipod_command;
    eim_reset_db_selection_command                                    reset_db_selection_command;
    eim_select_db_record_command                                       select_db_record_command;
    eim_get_number_categorized_dbrecords_command         get_number_categorized_dbrecords_command;
    eim_categorized_dbrecords_command                              categorized_dbrecords_command;
    eim_get_play_status_command                                         get_play_status_command;
    eim_get_current_playing_track_command                         get_playing_track_title_command;
    eim_get_indexed_playing_track_title_command                get_indexed_playing_tract_title_command;
    eim_get_indexed_playing_track_artist_command              get_indexed_playing_track_artist_command;
    eim_get_indexed_playing_track_album_command            get_indexed_playing_track_album_command;
    eim_set_play_status_change_notification                           set_play_status_change_notification;
    eim_play_current_selection_command                               play_current_selection_command;
    eim_play_control                                                                 play_control_command;
    eim_get_shuffle_command                                                 get_shuffle_command;
    eim_set_shuffle_command                                                 set_shuffle_command;
    eim_get_repeat_command                                                  get_repeat_command;
    eim_set_repeat_command                                                   set_repeat_command;
}ipod_command_context;


typedef struct {
        ipod_command_type               the_command_type;
        ipod_command_context          the_command_context;
}ipod_command;

#endif
