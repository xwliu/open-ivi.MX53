
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

#ifndef __TRACTOR_CANBOX_PROTOCAL_H
#define __TRACTOR_CANBOX_PROTOCAL_H

#define MAX_COMMAND_BYTE_LENGTH 256

typedef struct {
	char data[64];
	int buffer_size ;        
}canbox_notification;

// although same with notification, we still make a new name to diff it...
typedef struct {
	unsigned char data[64];
	int buffer_size ;        
}canbox_command;

#endif
