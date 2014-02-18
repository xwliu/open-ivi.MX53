/*
 * Copyright (C) 2007 The Android Open Source Project
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

#include <linux/input.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/reboot.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#include "common.h"
#include "minui/minui.h"
#include "recovery_ui.h"

#include <time.h>
#include <errno.h>

#define MAX_COLS 96
#define MAX_ROWS 32

#define CHAR_WIDTH 10
#define CHAR_HEIGHT 18

#define PROGRESSBAR_INDETERMINATE_STATES 6
#define PROGRESSBAR_INDETERMINATE_FPS 15

pthread_mutex_t gUpdateMutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t gSystemUpdateMutex = PTHREAD_MUTEX_INITIALIZER;
static gr_surface gBackgroundIcon[NUM_BACKGROUND_ICONS];

static gr_surface gBurningSeekbarIcon[BURNING_ICON_NUM];
static gr_surface gBurningTypeIcon[BURNING_TYPE_NUM];
static gr_surface gBurningSuccTypeIcon[BURNING_SUCC_NUM];
static gr_surface gBurningMiscIcon[BURNING_MISC_NUM];

static gr_surface gProgressBarIndeterminate[PROGRESSBAR_INDETERMINATE_STATES];
static gr_surface gProgressBarEmpty;
static gr_surface gProgressBarFill;

static const struct { gr_surface* surface; const char *name; } BITMAPS[] = {
/*	
    { &gBackgroundIcon[BACKGROUND_ICON_INSTALLING], "icon_installing" },
    { &gBackgroundIcon[BACKGROUND_ICON_ERROR],      "icon_error" },
    { &gBackgroundIcon[BACKGROUND_ICON_FACTORY_TEST],"icon_test" },
    { &gProgressBarIndeterminate[0],    "indeterminate1" },
    { &gProgressBarIndeterminate[1],    "indeterminate2" },
    { &gProgressBarIndeterminate[2],    "indeterminate3" },
    { &gProgressBarIndeterminate[3],    "indeterminate4" },
    { &gProgressBarIndeterminate[4],    "indeterminate5" },
    { &gProgressBarIndeterminate[5],    "indeterminate6" },
    { &gProgressBarEmpty,               "progress_empty" },
    { &gProgressBarFill,                "progress_fill" },
*/
/*the following is used to display updating status*/
	{ &gBurningSeekbarIcon[BURNING_SEEKBAR_0],                "seekbar_0" },
	{ &gBurningSeekbarIcon[BURNING_SEEKBAR_2P],			  	  "seekbar_2p" },
	{ &gBurningSeekbarIcon[BURNING_SEEKBAR_10P], 			  "seekbar_10p" },

	{ &gBurningTypeIcon[BURNING_TYPE_SYSTEM],			  	  	"recovery_system" },
	{ &gBurningTypeIcon[BURNING_TYPE_MPEG],			  	  		"recovery_mpeg" },
	{ &gBurningTypeIcon[BURNING_TYPE_MCU], 			  			"recovery_mcu" },
	{ &gBurningTypeIcon[BURNING_TYPE_CLEANBOOT], 			  	"clean_data_reset" },

	{ &gBurningSuccTypeIcon[BURNING_SUCC_TYPE_SYSTEM],				  		"recovery_system_success" },
	{ &gBurningSuccTypeIcon[BURNING_SUCC_TYPE_MPEG],			  		  	"recovery_mpeg_success" },
	{ &gBurningSuccTypeIcon[BURNING_SUCC_TYPE_MCU],							"recovery_mcu_success" },
	{ &gBurningSuccTypeIcon[BURNING_FAIL_TYPE_SYSTEM],				  		"recovery_system_failure" },
	{ &gBurningSuccTypeIcon[BURNING_FAIL_TYPE_MPEG],			  	  		"recovery_mpeg_failure" },
	{ &gBurningSuccTypeIcon[BURNING_FAIL_TYPE_MCU],				  			"recovery_mcu_failure" },
	{ &gBurningSuccTypeIcon[BURNING_NOT_EXIST_TYPE_SYSTEM],				  	"recovery_system_not_existed" },
	{ &gBurningSuccTypeIcon[BURNING_NOT_EXIST_TYPE_MPEG],				  	"recovery_mpeg_not_existed" },
	{ &gBurningSuccTypeIcon[BURNING_NOT_EXIST_TYPE_MCU], 			  	  	"recovery_mcu_not_existed" },
	{ &gBurningSuccTypeIcon[BURNING_SUCC_TYPE_CLEANBOOT],				  	"clean_reset_success" },
	{ &gBurningSuccTypeIcon[BURNING_FAIL_TYPE_CLEANBOOT],				  	"clean_reset_failure" },


	{ &gBurningMiscIcon[BURNING_MISC_UPDATING],			  	  	 "recovery_updating" },
	{ &gBurningMiscIcon[BURNING_MISC_PREPARE], 			  	"recovery_prepare" },
	{ &gBurningMiscIcon[BURNING_MISC_UPDATING_CLEANBOOT],		 "clean_recovery_reset" },
	{ &gBurningMiscIcon[BURNING_MISC_BACKGROUND],			  	 "recovery_bg" },
	{ &gBurningMiscIcon[BURNING_MISC_BACKGROUND_CLEANBOOT],		 "clean_recovery_bg" },
	{ &gBurningMiscIcon[BURNING_RESULT_SUCCESS],			  	 "updating_success" },
	{ &gBurningMiscIcon[BURNING_RESULT_FAILURE], 			  	 "updating_failure" },
	
    { NULL,                             NULL },
};

static gr_surface gCurrentIcon = NULL;

static enum ProgressBarType {
    PROGRESSBAR_TYPE_NONE,
    PROGRESSBAR_TYPE_INDETERMINATE,
    PROGRESSBAR_TYPE_NORMAL,
} gProgressBarType = PROGRESSBAR_TYPE_NONE;

// Progress bar scope of current operation
static float gProgressScopeStart = 0, gProgressScopeSize = 0, gProgress = 0;
static time_t gProgressScopeTime, gProgressScopeDuration;

// Set to 1 when both graphics pages are the same (except for the progress bar)
static int gPagesIdentical = 0;

// Log text overlay, displayed when a magic key is pressed
static char text[MAX_ROWS][MAX_COLS];
static int text_cols = 0, text_rows = 0;
static int text_col = 0, text_row = 0, text_top = 0;
static int show_text = 1;

static char menu[MAX_ROWS][MAX_COLS];
static int show_menu = 0;
static int menu_top = 0, menu_items = 0, menu_sel = 0;

// Key event input queue
static pthread_mutex_t key_queue_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t key_queue_cond = PTHREAD_COND_INITIALIZER;
static int key_queue[256], key_queue_len = 0;
static volatile char key_pressed[KEY_MAX + 1];

// Clear the screen and draw the currently selected background icon (if any).
// Should only be called with gUpdateMutex locked.
static void draw_background_locked(gr_surface icon)
{
    gPagesIdentical = 0;
    gr_color(0, 0, 0, 255);
    gr_fill(0, 0, gr_fb_width(), gr_fb_height());
    if (icon) {
        int iconWidth = gr_get_width(icon);
        int iconHeight = gr_get_height(icon);
        int iconX = (gr_fb_width() - iconWidth) / 2;
        int iconY = (gr_fb_height() - iconHeight) / 2;
        gr_blit(icon, 0, 0, iconWidth, iconHeight, iconX, iconY);
    }
}
/*for update status, doing by Aibing*/
extern char version_buf[];

static void draw_prepare_background_locked()
{
    gPagesIdentical = 0;

	int iconWidth;
	int iconHeight;
	int iconX;
	int iconY;

	static gr_surface gBackgroundIcon= NULL;
	gBackgroundIcon = gBurningMiscIcon[BURNING_MISC_PREPARE];
	iconWidth = gr_get_width(gBackgroundIcon);
	iconHeight = gr_get_height(gBackgroundIcon);
	iconX = 0;
	iconY = 0;
	gr_blit(gBackgroundIcon, 0, 0, iconWidth, iconHeight, iconX, iconY);

    gr_color(80, 80, 80, 128);
	gr_text(150,470,version_buf);
}

static void draw_burning_background_locked(gr_surface icon)
{
    gPagesIdentical = 0;

	int iconWidth;
	int iconHeight;
	int iconX;
	int iconY;

	static gr_surface gBackgroundIcon= NULL;
	gBackgroundIcon = gBurningMiscIcon[BURNING_MISC_BACKGROUND];
	iconWidth = gr_get_width(gBackgroundIcon);
	iconHeight = gr_get_height(gBackgroundIcon);
	iconX = 0;
	iconY = 0;
	gr_blit(gBackgroundIcon, 0, 0, iconWidth, iconHeight, iconX, iconY);

    if (icon) {
        iconWidth = gr_get_width(icon);
        iconHeight = gr_get_height(icon);
        iconX = 8;
        iconY = 79;
        gr_blit(icon, 0, 0, iconWidth, iconHeight, iconX, iconY);
    }
    gr_color(80, 80, 80, 128);
	gr_text(150,470,version_buf);
}

static void draw_burning_type_locked(gr_surface icon)
{
    gPagesIdentical = 0;
    if (icon) {
        int iconWidth = gr_get_width(icon);
        int iconHeight = gr_get_height(icon);
        int iconX = 25;
		int iconY = 191;
        gr_blit(icon, 0, 0, iconWidth, iconHeight, iconX, iconY);
    }
}

int burning_seekbar_status = 0;
int burning_dsp10_status = 0;

static void draw_burning_status_locked(void)
{
	int iconWidth;
    int iconHeight;
    int iconX;
    int iconY; 
    gPagesIdentical = 0;
	if(burning_seekbar_status == 0) {
		gCurrentIcon = gBurningSeekbarIcon[BURNING_SEEKBAR_0];
        iconWidth = gr_get_width(gCurrentIcon);
        iconHeight = gr_get_height(gCurrentIcon);
        iconX = 25;
        iconY = 328;
        gr_blit(gCurrentIcon, 0, 0, iconWidth, iconHeight, iconX, iconY);
		burning_seekbar_status = 1;
		return;
	}
	
	gCurrentIcon = gBurningSeekbarIcon[BURNING_SEEKBAR_10P];
	iconWidth = gr_get_width(gCurrentIcon);
    iconHeight = gr_get_height(gCurrentIcon);
    iconX = 25 + burning_dsp10_status * iconWidth;
    iconY = 328;
    burning_dsp10_status++;
	gr_blit(gCurrentIcon, 0, 0, iconWidth, iconHeight, iconX, iconY);  
}

int burning_detail_seekbar_status = 0;
int burning_dsp_status = 0;
static void draw_burning_status_detail_locked(void)
{
	int iconWidth;
    int iconHeight;
    int iconX;
    int iconY; 
    gPagesIdentical = 0;
	if(burning_detail_seekbar_status == 0) {
		gCurrentIcon = gBurningSeekbarIcon[BURNING_SEEKBAR_0];
        iconWidth = gr_get_width(gCurrentIcon);
        iconHeight = gr_get_height(gCurrentIcon);
        iconX = 25;
        iconY = 365;
        gr_blit(gCurrentIcon, 0, 0, iconWidth, iconHeight, iconX, iconY);
		burning_detail_seekbar_status = 1;
		return;
	}
	
	gCurrentIcon = gBurningSeekbarIcon[BURNING_SEEKBAR_2P];
	iconWidth = gr_get_width(gCurrentIcon);
    iconHeight = gr_get_height(gCurrentIcon);
    iconX = 25 + burning_dsp_status * iconWidth;
    iconY = 365;
    burning_dsp_status++;
	gr_blit(gCurrentIcon, 0, 0, iconWidth, iconHeight, iconX, iconY);  
}


int list_step = 0; 
static void draw_burning_succ_locked(gr_surface icon)
{
    gPagesIdentical = 0;
    if (icon) {
        int iconWidth = gr_get_width(icon);
        int iconHeight = gr_get_height(icon);
        int iconX = 25 + list_step;
        int iconY = 134;
        list_step = list_step + iconWidth;
        gr_blit(icon, 0, 0, iconWidth, iconHeight, iconX, iconY);
    }
}

static void draw_burning_reflesh_locked(int status)
{
	int icon_ind;

	gCurrentIcon = gBurningMiscIcon[BURNING_MISC_UPDATING];
	draw_burning_background_locked(gCurrentIcon);
	
	list_step = 0;
	burning_detail_seekbar_status = 0;
	burning_dsp_status = 0;
	burning_seekbar_status = 0;
	burning_dsp10_status = 0;
	
	icon_ind = status&0x03;
	if(icon_ind != 0x03) {
		gCurrentIcon = gBurningSuccTypeIcon[icon_ind * 3 ];
		draw_burning_succ_locked(gCurrentIcon);
	}
	
	icon_ind = (status >> 2)&0x03;
	if(icon_ind != 0x03) {
		gCurrentIcon = gBurningSuccTypeIcon[icon_ind * 3 + 1];
		draw_burning_succ_locked(gCurrentIcon);
	}

	icon_ind = (status >> 4)&0x03;
	if(icon_ind != 0x03) {
		gCurrentIcon = gBurningSuccTypeIcon[icon_ind * 3 + 2];
		draw_burning_succ_locked(gCurrentIcon);
	}
}

// Draw the progress bar (if any) on the screen.  Does not flip pages.
// Should only be called with gUpdateMutex locked.
static void draw_progress_locked()
{
    if (gProgressBarType == PROGRESSBAR_TYPE_NONE) return;

    int iconHeight = gr_get_height(gBackgroundIcon[BACKGROUND_ICON_INSTALLING]);
    int width = gr_get_width(gProgressBarEmpty);
    int height = gr_get_height(gProgressBarEmpty);

    int dx = (gr_fb_width() - width)/2;
    int dy = (3*gr_fb_height() + iconHeight - 2*height)/4;

    // Erase behind the progress bar (in case this was a progress-only update)
    gr_color(0, 0, 0, 255);
    gr_fill(dx, dy, width, height);

    if (gProgressBarType == PROGRESSBAR_TYPE_NORMAL) {
        float progress = gProgressScopeStart + gProgress * gProgressScopeSize;
        int pos = (int) (progress * width);

        if (pos > 0) {
          gr_blit(gProgressBarFill, 0, 0, pos, height, dx, dy);
        }
        if (pos < width-1) {
          gr_blit(gProgressBarEmpty, pos, 0, width-pos, height, dx+pos, dy);
        }
    }

    if (gProgressBarType == PROGRESSBAR_TYPE_INDETERMINATE) {
        static int frame = 0;
        gr_blit(gProgressBarIndeterminate[frame], 0, 0, width, height, dx, dy);
        frame = (frame + 1) % PROGRESSBAR_INDETERMINATE_STATES;
    }
}

static void draw_text_line(int row, const char* t) {
  if (t[0] != '\0') {
    gr_text(0, (row+1)*CHAR_HEIGHT-1, t);
  }
}

// Redraw everything on the screen.  Does not flip pages.
// Should only be called with gUpdateMutex locked.
static void draw_screen_locked(void)
{
    draw_background_locked(gCurrentIcon);
    draw_progress_locked();

    if (show_text) {
        gr_color(0, 0, 0, 160);
        gr_fill(0, 0, gr_fb_width(), gr_fb_height());

        int i = 0;
        if (show_menu) {
            gr_color(64, 96, 255, 255);
            gr_fill(0, (menu_top+menu_sel) * CHAR_HEIGHT,
                    gr_fb_width(), (menu_top+menu_sel+1)*CHAR_HEIGHT+1);
 
            for (; i < menu_top + menu_items; ++i) {
                if (i == menu_top + menu_sel) {
                    gr_color(255, 255, 255, 255);
                    draw_text_line(i, menu[i]);
                    gr_color(64, 96, 255, 255);
                } else {
                    draw_text_line(i, menu[i]);
                }
            }
            gr_fill(0, i*CHAR_HEIGHT+CHAR_HEIGHT/2-1,
                    gr_fb_width(), i*CHAR_HEIGHT+CHAR_HEIGHT/2+1);
            ++i;
        }

        gr_color(255, 255, 0, 255);

        for (; i < text_rows; ++i) {
            draw_text_line(i, text[(i+text_top) % text_rows]);
        }
    }
}

// Redraw everything on the screen and flip the screen (make it visible).
// Should only be called with gUpdateMutex locked.
static void update_screen_locked(void)
{
    draw_screen_locked();
    gr_flip();
}
/*for updating status, doing by Aibing*/
static void set_prepare_background_locked(void)
{
	draw_prepare_background_locked();
    gr_flip();
}

static void update_burning_background_locked(void)
{
	draw_burning_background_locked(gCurrentIcon);
    gr_flip();
}

static void update_burning_type_locked(void)
{
	draw_burning_type_locked(gCurrentIcon);
    gr_flip();
}

static void update_burning_status_locked(void)
{
	draw_burning_status_locked();
    gr_flip();
}

static void update_burning_status_detail_locked(void)
{
	draw_burning_status_detail_locked();
    gr_flip();
}


static void update_burning_succ_locked(void)
{
	draw_burning_succ_locked(gCurrentIcon);
    gr_flip();
}

static void update_burning_reflesh_locked(int status)
{
	draw_burning_reflesh_locked(status);
    gr_flip();
}


// Updates only the progress bar, if possible, otherwise redraws the screen.
// Should only be called with gUpdateMutex locked.
static void update_progress_locked(void)
{
    if (show_text || !gPagesIdentical) {
        draw_screen_locked();    // Must redraw the whole screen
        gPagesIdentical = 1;
    } else {
        draw_progress_locked();  // Draw only the progress bar
    }
    gr_flip();
}

// Keeps the progress bar updated, even when the process is otherwise busy.
static void *progress_thread(void *cookie)
{
    for (;;) {
        usleep(1000000 / PROGRESSBAR_INDETERMINATE_FPS);
        pthread_mutex_lock(&gUpdateMutex);

        // update the progress bar animation, if active
        // skip this if we have a text overlay (too expensive to update)
        if (gProgressBarType == PROGRESSBAR_TYPE_INDETERMINATE && !show_text) {
            update_progress_locked();
        }

        // move the progress bar forward on timed intervals, if configured
        int duration = gProgressScopeDuration;
        if (gProgressBarType == PROGRESSBAR_TYPE_NORMAL && duration > 0) {
            int elapsed = time(NULL) - gProgressScopeTime;
            float progress = 1.0 * elapsed / duration;
            if (progress > 1.0) progress = 1.0;
            if (progress > gProgress) {
                gProgress = progress;
                update_progress_locked();
            }
        }

        pthread_mutex_unlock(&gUpdateMutex);
    }
    return NULL;
}

// Reads input events, handles special hot keys, and adds to the key queue.
static void *input_thread(void *cookie)
{
    int rel_sum = 0;
    int fake_key = 0;
    for (;;) {
        // wait for the next key event
        struct input_event ev;
        do {
            ev_get(&ev, 0);

            if (ev.type == EV_SYN) {
                continue;
            } else if (ev.type == EV_REL) {
                if (ev.code == REL_Y) {
                    // accumulate the up or down motion reported by
                    // the trackball.  When it exceeds a threshold
                    // (positive or negative), fake an up/down
                    // key event.
                    rel_sum += ev.value;
                    if (rel_sum > 3) {
                        fake_key = 1;
                        ev.type = EV_KEY;
                        ev.code = KEY_DOWN;
                        ev.value = 1;
                        rel_sum = 0;
                    } else if (rel_sum < -3) {
                        fake_key = 1;
                        ev.type = EV_KEY;
                        ev.code = KEY_UP;
                        ev.value = 1;
                        rel_sum = 0;
                    }
                }
            } else {
                rel_sum = 0;
            }
        } while (ev.type != EV_KEY || ev.code > KEY_MAX);

        pthread_mutex_lock(&key_queue_mutex);
        if (!fake_key) {
            // our "fake" keys only report a key-down event (no
            // key-up), so don't record them in the key_pressed
            // table.
            key_pressed[ev.code] = ev.value;
        }
        fake_key = 0;
        const int queue_max = sizeof(key_queue) / sizeof(key_queue[0]);
        if (ev.value > 0 && key_queue_len < queue_max) {
            key_queue[key_queue_len++] = ev.code;
            pthread_cond_signal(&key_queue_cond);
        }
        pthread_mutex_unlock(&key_queue_mutex);
/*
        if (ev.value > 0 && device_toggle_display(key_pressed, ev.code)) {
            pthread_mutex_lock(&gUpdateMutex);
            show_text = !show_text;
            update_screen_locked();
            pthread_mutex_unlock(&gUpdateMutex);
        }
*/
        if (ev.value > 0 && device_reboot_now(key_pressed, ev.code)) {
            reboot(RB_AUTOBOOT);
        }
    }
    return NULL;
}

void ui_init(void)
{
    gr_init();
    ev_init();

    text_col = text_row = 0;
    text_rows = gr_fb_height() / CHAR_HEIGHT;
    if (text_rows > MAX_ROWS) text_rows = MAX_ROWS;
    text_top = 1;

    text_cols = gr_fb_width() / CHAR_WIDTH;
    if (text_cols > MAX_COLS - 1) text_cols = MAX_COLS - 1;

    int i;
    for (i = 0; BITMAPS[i].name != NULL; ++i) {
        int result = res_create_surface(BITMAPS[i].name, BITMAPS[i].surface);
        if (result < 0) {
            if (result == -2) {
                LOGI("Bitmap %s missing header\n", BITMAPS[i].name);
            } else {
                LOGE("Missing bitmap %s\n(Code %d)\n", BITMAPS[i].name, result);
            }
            *BITMAPS[i].surface = NULL;
        }
    }

    pthread_t t;
//    pthread_create(&t, NULL, progress_thread, NULL);
    pthread_create(&t, NULL, input_thread, NULL);
}

void ui_set_background(int icon)
{
/*
    pthread_mutex_lock(&gUpdateMutex);
    gCurrentIcon = gBackgroundIcon[icon];
    update_screen_locked();
    pthread_mutex_unlock(&gUpdateMutex);
*/
}
/*for updating status, doing by Aibing*/
void ui_set_prepare_background()
{
    pthread_mutex_lock(&gUpdateMutex);
    set_prepare_background_locked();
    pthread_mutex_unlock(&gUpdateMutex);
}

void ui_set_burning_background(int icon)
{
    pthread_mutex_lock(&gUpdateMutex);
    gCurrentIcon = gBurningMiscIcon[icon];
    update_burning_background_locked();
    pthread_mutex_unlock(&gUpdateMutex);
}


void ui_set_burning_type(int icon)
{
    pthread_mutex_lock(&gUpdateMutex);
    gCurrentIcon = gBurningTypeIcon[icon];
    update_burning_type_locked();
    pthread_mutex_unlock(&gUpdateMutex);
}

void ui_set_burning_status(void)
{
    pthread_mutex_lock(&gUpdateMutex);
    update_burning_status_locked();
    pthread_mutex_unlock(&gUpdateMutex);
}

void ui_set_burning_status_detail(void)
{
    pthread_mutex_lock(&gUpdateMutex);
    update_burning_status_detail_locked();
    pthread_mutex_unlock(&gUpdateMutex);
}

void ui_set_burning_succ(int icon)
{
    pthread_mutex_lock(&gUpdateMutex);
    gCurrentIcon = gBurningSuccTypeIcon[icon];
    update_burning_succ_locked();
    pthread_mutex_unlock(&gUpdateMutex);
}

void ui_set_burning_reflesh(int status)
{
    pthread_mutex_lock(&gUpdateMutex);
    update_burning_reflesh_locked(status);
    pthread_mutex_unlock(&gUpdateMutex);
}

// only used in system update mode, as to make user see the program is running while  
// updating and formating, this thread will end while it's update end, and never called again 
volatile int system_update_end = 0;
static void *progress_system_update(void *cookie)
{
    for (;;) {
		if (burning_dsp_status < 50) {
			pthread_mutex_lock(&gSystemUpdateMutex);
			ui_set_burning_status_detail();
			pthread_mutex_unlock(&gSystemUpdateMutex);
		} else {
			ui_set_burning_status();
			system_update_end = 1;
			break;
		}
        usleep(2000000);
    }
	printf("system_update_end == %d\n\r",system_update_end);
    return NULL;
}


void ui_set_burning_system(void)
{
    pthread_t t;
    pthread_create(&t, NULL, progress_system_update, NULL);
}
/*****************************************************************************/
/* 	For cleanboot
  */
void ui_set_cleanboot_background()
{
    pthread_mutex_lock(&gUpdateMutex);
    // update_burning_background_locked();
    gPagesIdentical = 0;
   
    int iconWidth;
    int iconHeight;
    int iconX;
    int iconY;
   
    static gr_surface gCleanbootIcon= NULL;
    gCleanbootIcon = gBurningMiscIcon[BURNING_MISC_BACKGROUND_CLEANBOOT];
    iconWidth = gr_get_width(gCleanbootIcon);
    iconHeight = gr_get_height(gCleanbootIcon);
    iconX = 0;
    iconY = 0;
    gr_blit(gCleanbootIcon, 0, 0, iconWidth, iconHeight, iconX, iconY);
   
    gCleanbootIcon = gBurningMiscIcon[BURNING_MISC_UPDATING_CLEANBOOT];
    iconWidth = gr_get_width(gCleanbootIcon);
    iconHeight = gr_get_height(gCleanbootIcon);
    iconX = 8;
    iconY = 79;
    gr_blit(gCleanbootIcon, 0, 0, iconWidth, iconHeight, iconX, iconY);

	gCleanbootIcon = gBurningTypeIcon[BURNING_TYPE_CLEANBOOT];
	iconWidth = gr_get_width(gCleanbootIcon);
	iconHeight = gr_get_height(gCleanbootIcon);
	iconX = 25;
	iconY = 191;
	gr_blit(gCleanbootIcon, 0, 0, iconWidth, iconHeight, iconX, iconY);

	
    gr_color(80, 80, 80, 128);
	gr_text(150,470,version_buf);

    gr_flip();
    pthread_mutex_unlock(&gUpdateMutex);

}


void ui_set_cleanboot_succ()
{
    pthread_mutex_lock(&gUpdateMutex);
    // update_burning_background_locked();
    gPagesIdentical = 0;
   
    int iconWidth;
    int iconHeight;
    int iconX;
    int iconY;
   
    static gr_surface gCleanbootIcon= NULL;
    gCleanbootIcon = gBurningMiscIcon[BURNING_MISC_BACKGROUND_CLEANBOOT];
    iconWidth = gr_get_width(gCleanbootIcon);
    iconHeight = gr_get_height(gCleanbootIcon);
    iconX = 0;
    iconY = 0;
    gr_blit(gCleanbootIcon, 0, 0, iconWidth, iconHeight, iconX, iconY);
   
    gCleanbootIcon = gBurningMiscIcon[BURNING_MISC_UPDATING_CLEANBOOT];
    iconWidth = gr_get_width(gCleanbootIcon);
    iconHeight = gr_get_height(gCleanbootIcon);
    iconX = 8;
    iconY = 79;
    gr_blit(gCleanbootIcon, 0, 0, iconWidth, iconHeight, iconX, iconY);

	gCleanbootIcon = gBurningSuccTypeIcon[BURNING_SUCC_TYPE_CLEANBOOT];
	iconWidth = gr_get_width(gCleanbootIcon);
	iconHeight = gr_get_height(gCleanbootIcon);
	iconX = 25;
	iconY = 134;
	gr_blit(gCleanbootIcon, 0, 0, iconWidth, iconHeight, iconX, iconY);
	
    gr_color(80, 80, 80, 128);
	gr_text(150,470,version_buf);

    gr_flip();
    pthread_mutex_unlock(&gUpdateMutex);

}

volatile int system_cleanboot_end = 0;
int cleanboot_dsp_status = 0;
int cleanboot_detail_seekbar_status = 0;
//int cleanboot_dsp_status = 0;

void ui_set_clean_status_detail(void)
{
	pthread_mutex_lock(&gUpdateMutex);
	int iconWidth;
    int iconHeight;
    int iconX;
    int iconY; 
    gPagesIdentical = 0;
    static gr_surface gCleanbootIcon= NULL;
	
	if(cleanboot_detail_seekbar_status == 0) {
		gCleanbootIcon = gBurningSeekbarIcon[BURNING_SEEKBAR_0];
        iconWidth = gr_get_width(gCleanbootIcon);
        iconHeight = gr_get_height(gCleanbootIcon);
        iconX = 25;
        iconY = 328;
        gr_blit(gCleanbootIcon, 0, 0, iconWidth, iconHeight, iconX, iconY);
		cleanboot_detail_seekbar_status = 1;
		pthread_mutex_unlock(&gUpdateMutex);
		return;
	}
	
	gCleanbootIcon = gBurningSeekbarIcon[BURNING_SEEKBAR_2P];
	iconWidth = gr_get_width(gCleanbootIcon);
    iconHeight = gr_get_height(gCleanbootIcon);
    iconX = 25 + cleanboot_dsp_status * iconWidth;
    iconY = 328;
    cleanboot_dsp_status++;
	gr_blit(gCleanbootIcon, 0, 0, iconWidth, iconHeight, iconX, iconY);  
    gr_flip();
	pthread_mutex_unlock(&gUpdateMutex);
}

static void *progress_system_cleanboot(void *cookie)
{
	printf("progress_system_cleanboot\n\r");
	int i = 0;
	while(1) {
		for( i = 0; i <= 50; i++) {
			pthread_mutex_lock(&gSystemUpdateMutex);
			ui_set_clean_status_detail();
			pthread_mutex_unlock(&gSystemUpdateMutex);
    	    usleep(800000);
    	}
		break;
	}
	system_cleanboot_end = 1;
	printf("system_update_end == %d\n\r",system_cleanboot_end);
    return NULL;
}


void ui_set_cleanboot_system(void)
{
    pthread_t t;
    pthread_create(&t, NULL, progress_system_cleanboot, NULL);
}

void ui_show_indeterminate_progress()
{
/*
    pthread_mutex_lock(&gUpdateMutex);
    if (gProgressBarType != PROGRESSBAR_TYPE_INDETERMINATE) {
        gProgressBarType = PROGRESSBAR_TYPE_INDETERMINATE;
        update_progress_locked();
    }
    pthread_mutex_unlock(&gUpdateMutex);
*/
}

void ui_show_progress(float portion, int seconds)
{
/*
    pthread_mutex_lock(&gUpdateMutex);
    gProgressBarType = PROGRESSBAR_TYPE_NORMAL;
    gProgressScopeStart += gProgressScopeSize;
    gProgressScopeSize = portion;
    gProgressScopeTime = time(NULL);
    gProgressScopeDuration = seconds;
    gProgress = 0;
    update_progress_locked();
    pthread_mutex_unlock(&gUpdateMutex);
*/
}

void ui_set_progress(float fraction)
{
/*
    pthread_mutex_lock(&gUpdateMutex);
    if (fraction < 0.0) fraction = 0.0;
    if (fraction > 1.0) fraction = 1.0;
    if (gProgressBarType == PROGRESSBAR_TYPE_NORMAL && fraction > gProgress) {
        // Skip updates that aren't visibly different.
        int width = gr_get_width(gProgressBarIndeterminate[0]);
        float scale = width * gProgressScopeSize;
        if ((int) (gProgress * scale) != (int) (fraction * scale)) {
            gProgress = fraction;
            update_progress_locked();
        }
    }
    pthread_mutex_unlock(&gUpdateMutex);
*/
}

void ui_reset_progress()
{
/*
    pthread_mutex_lock(&gUpdateMutex);
    gProgressBarType = PROGRESSBAR_TYPE_NONE;
    gProgressScopeStart = gProgressScopeSize = 0;
    gProgressScopeTime = gProgressScopeDuration = 0;
    gProgress = 0;
    update_screen_locked();
    pthread_mutex_unlock(&gUpdateMutex);
*/
}

void ui_print(const char *fmt, ...)
{
#if 0
    char buf[256];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, 256, fmt, ap);
    va_end(ap);

    fputs(buf, stdout);

    // This can get called before ui_init(), so be careful.
    pthread_mutex_lock(&gUpdateMutex);
    if (text_rows > 0 && text_cols > 0) {
        char *ptr;
        for (ptr = buf; *ptr != '\0'; ++ptr) {
            if (*ptr == '\n' || text_col >= text_cols) {
                text[text_row][text_col] = '\0';
                text_col = 0;
                text_row = (text_row + 1) % text_rows;
                if (text_row == text_top) text_top = (text_top + 1) % text_rows;
            }
            if (*ptr != '\n') text[text_row][text_col++] = *ptr;
        }
        text[text_row][text_col] = '\0';
        update_screen_locked();
    }
    pthread_mutex_unlock(&gUpdateMutex);
#else
    va_list ap;
    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);
#endif
}
static int param_init(void)
{
    	text_rows = 26;
	text_cols = 80;
	menu_top  = 0;
	menu_items= 0;
	menu_sel  = 0;
	text_row  = 0;  
	text_col  = 0;
	text_top  = 1;
	return 0;
}

void ui_start_menu(char** headers, char** items, int initial_selection) {
    int i;
    pthread_mutex_lock(&gUpdateMutex);
    if (text_rows > 0 && text_cols > 0) {
        for (i = 0; i < text_rows; ++i) {
            if (headers[i] == NULL) break;
            strncpy(menu[i], headers[i], text_cols-1);
            menu[i][text_cols-1] = '\0';
        }
        menu_top = i;
        for (; i < text_rows; ++i) {
            if (items[i-menu_top] == NULL) break;
            strncpy(menu[i], items[i-menu_top], text_cols-1);
            menu[i][text_cols-1] = '\0';
        }
        menu_items = i - menu_top;
        show_menu = 1;
        menu_sel = initial_selection;
        update_screen_locked();
    }
    pthread_mutex_unlock(&gUpdateMutex);
}

int ui_menu_select(int sel) {
    int old_sel;
    pthread_mutex_lock(&gUpdateMutex);
    if (show_menu > 0) {
        old_sel = menu_sel;
        menu_sel = sel;
        if (menu_sel < 0) menu_sel = 0;
        if (menu_sel >= menu_items) menu_sel = menu_items-1;
        sel = menu_sel;
        if (menu_sel != old_sel) update_screen_locked();
    }
    pthread_mutex_unlock(&gUpdateMutex);
    return sel;
}

void ui_end_menu() {
    int i;
    pthread_mutex_lock(&gUpdateMutex);
    if (show_menu > 0 && text_rows > 0 && text_cols > 0) {
        show_menu = 0;
        update_screen_locked();
    }
    pthread_mutex_unlock(&gUpdateMutex);
}

int ui_text_visible()
{
    pthread_mutex_lock(&gUpdateMutex);
    int visible = show_text;
    pthread_mutex_unlock(&gUpdateMutex);
    return visible;
}

void ui_show_text(int visible)
{
    pthread_mutex_lock(&gUpdateMutex);
    show_text = visible;
    update_screen_locked();
    pthread_mutex_unlock(&gUpdateMutex);
}

int ui_wait_key_timeout(int *key, int sec_out)
{ 
    struct timespec time_out;
    time_out.tv_sec = time(NULL) + sec_out;
    time_out.tv_nsec = 0;
    int ret;

    pthread_mutex_lock(&key_queue_mutex);
    ret =  pthread_cond_timedwait(&key_queue_cond, &key_queue_mutex, &time_out);
    if (ret == ETIMEDOUT) {
	pthread_mutex_unlock(&key_queue_mutex);
	return 0;
    }
    *key = key_queue[0];
    memcpy(&key_queue[0], &key_queue[1], sizeof(int) * --key_queue_len);
    pthread_mutex_unlock(&key_queue_mutex);
    return 1;
}

int ui_wait_key()
{
    pthread_mutex_lock(&key_queue_mutex);
    while (key_queue_len == 0) {
        pthread_cond_wait(&key_queue_cond, &key_queue_mutex);
    }

    int key = key_queue[0];
    memcpy(&key_queue[0], &key_queue[1], sizeof(int) * --key_queue_len);
    pthread_mutex_unlock(&key_queue_mutex);
    return key;
}

int ui_key_pressed(int key)
{
    // This is a volatile static array, don't bother locking
    return key_pressed[key];
}

void ui_clear_key_queue() {
    pthread_mutex_lock(&key_queue_mutex);
    key_queue_len = 0;
    pthread_mutex_unlock(&key_queue_mutex);
}
