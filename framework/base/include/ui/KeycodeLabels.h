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

#ifndef _UI_KEYCODE_LABELS_H
#define _UI_KEYCODE_LABELS_H

#include <android/keycodes.h>

struct KeycodeLabel {
    const char *literal;
    int value;
};

static const KeycodeLabel KEYCODES[] = {
    { "SOFT_LEFT", 1 },
    { "SOFT_RIGHT", 2 },
    { "HOME", 3 },
    { "BACK", 4 },
    { "CALL", 5 },
    { "ENDCALL", 6 },
    { "0", 7 },
    { "1", 8 },
    { "2", 9 },
    { "3", 10 },
    { "4", 11 },
    { "5", 12 },
    { "6", 13 },
    { "7", 14 },
    { "8", 15 },
    { "9", 16 },
    { "STAR", 17 },
    { "POUND", 18 },
    { "DPAD_UP", 19 },
    { "DPAD_DOWN", 20 },
    { "DPAD_LEFT", 21 },
    { "DPAD_RIGHT", 22 },
    { "DPAD_CENTER", 23 },
    { "VOLUME_UP", 24 },
    { "VOLUME_DOWN", 25 },
    { "POWER", 26 },
    { "CAMERA", 27 },
    { "CLEAR", 28 },
    { "A", 29 },
    { "B", 30 },
    { "C", 31 },
    { "D", 32 },
    { "E", 33 },
    { "F", 34 },
    { "G", 35 },
    { "H", 36 },
    { "I", 37 },
    { "J", 38 },
    { "K", 39 },
    { "L", 40 },
    { "M", 41 },
    { "N", 42 },
    { "O", 43 },
    { "P", 44 },
    { "Q", 45 },
    { "R", 46 },
    { "S", 47 },
    { "T", 48 },
    { "U", 49 },
    { "V", 50 },
    { "W", 51 },
    { "X", 52 },
    { "Y", 53 },
    { "Z", 54 },
    { "COMMA", 55 },
    { "PERIOD", 56 },
    { "ALT_LEFT", 57 },
    { "ALT_RIGHT", 58 },
    { "SHIFT_LEFT", 59 },
    { "SHIFT_RIGHT", 60 },
    { "TAB", 61 },
    { "SPACE", 62 },
    { "SYM", 63 },
    { "EXPLORER", 64 },
    { "ENVELOPE", 65 },
    { "ENTER", 66 },
    { "DEL", 67 },
    { "GRAVE", 68 },
    { "MINUS", 69 },
    { "EQUALS", 70 },
    { "LEFT_BRACKET", 71 },
    { "RIGHT_BRACKET", 72 },
    { "BACKSLASH", 73 },
    { "SEMICOLON", 74 },
    { "APOSTROPHE", 75 },
    { "SLASH", 76 },
    { "AT", 77 },
    { "NUM", 78 },
    { "HEADSETHOOK", 79 },
    { "FOCUS", 80 },
    { "PLUS", 81 },
    { "MENU", 82 },
    { "NOTIFICATION", 83 },
    { "SEARCH", 84 },
    { "MEDIA_PLAY_PAUSE", 85 },
    { "MEDIA_STOP", 86 },
    { "MEDIA_NEXT", 87 },
    { "MEDIA_PREVIOUS", 88 },
    { "MEDIA_REWIND", 89 },
    { "MEDIA_FAST_FORWARD", 90 },
    { "MUTE", 91 },
    { "PAGE_UP", 92 },
    { "PAGE_DOWN", 93 },
    { "PICTSYMBOLS", 94 },
    { "SWITCH_CHARSET", 95 },
    { "BUTTON_A", 96 },
    { "BUTTON_B", 97 },
    { "BUTTON_C", 98 },
    { "BUTTON_X", 99 },
    { "BUTTON_Y", 100 },
    { "BUTTON_Z", 101 },
    { "BUTTON_L1", 102 },
    { "BUTTON_R1", 103 },
    { "BUTTON_L2", 104 },
    { "BUTTON_R2", 105 },
    { "BUTTON_THUMBL", 106 },
    { "BUTTON_THUMBR", 107 },
    { "BUTTON_START", 108 },
    { "BUTTON_SELECT", 109 },
    { "BUTTON_MODE", 110 },
    // Add following key for tractor AKEY and SWC
    // Note: the definition here is confilict with later android(4.0)
    // So if we porting from 2.3 to 4.0, we need adjust the value according to Android 4.0 definition
    { "F1", 111 },
    { "F2", 112 },
    { "F3", 113 },
    { "F4", 114 },
    { "F5", 115 },
    { "F6", 116 },
    { "F7", 117 },
    { "F8", 118 },
    { "F9", 119 },
    { "F10", 120 },
    { "F11", 121 },
    { "F12", 122 },
    { "F13", 123 },
    { "F14", 124 },
    { "F15", 125 },
    { "F16", 126 },
    { "F17", 127 },
    { "F18", 128 },
    { "F19", 129 },
    { "F20", 130 },
    { "F21", 131 },
    { "F22", 132 },
    { "F23", 133 },
    { "F24", 134 },
    { "FN_F1" , 135 },
    { "FN_F2" , 136 },
    { "PROG1", 137 },
    { "PROG2", 138 },
    { "PLAYPAUSE", 139 },
    { "HOMEPAGE", 140 },
    { "SCROLLUP", 141 },
    { "SCROLLDOWN", 142 },
    // NOTE: If you add a new keycode here you must also add it to several other files.
    //       Refer to frameworks/base/core/java/android/view/KeyEvent.java for the full list.

    { NULL, 0 }
};

// See also policy flags in Input.h.
static const KeycodeLabel FLAGS[] = {
    { "WAKE", 0x00000001 },
    { "WAKE_DROPPED", 0x00000002 },
    { "SHIFT", 0x00000004 },
    { "CAPS_LOCK", 0x00000008 },
    { "ALT", 0x00000010 },
    { "ALT_GR", 0x00000020 },
    { "MENU", 0x00000040 },
    { "LAUNCHER", 0x00000080 },
    { "VIRTUAL", 0x00000100 },
    { NULL, 0 }
};

#endif // _UI_KEYCODE_LABELS_H
