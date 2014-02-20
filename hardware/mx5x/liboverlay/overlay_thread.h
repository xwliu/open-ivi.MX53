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

/**
 *  Copyright (C) 2011 Freescale Semiconductor,Inc.,
 *
 */

 
#ifndef __OVERLAY_THREAD_H__
#define __OVERLAY_THREAD_H__

//#include <linux/videodev.h>
//#include <cutils/properties.h>
//#include "overlay.h"
#include <utils/threads.h>

extern "C" {
#include "mxc_ipu_hl_lib.h" 
} 

using namespace android;

class OverlayThread: public Thread {
    struct overlay_control_context_t *m_dev;
    unsigned int mLatestOutBuf;//Physical address of output
    ipu_lib_input_param_t mIPUInputParam;   
    ipu_lib_output_param_t mIPUOutputParam; 
    ipu_lib_handle_t            mIPUHandle;
    int mIPURet;
    ipu_lib_input_param_t sec_video_mIPUInputParam;
    ipu_lib_output_param_t sec_video_mIPUOutputParam;
    ipu_lib_handle_t sec_video_mIPUHandle;
    int sec_video_mIPURet;
	unsigned int ClearBufMask;
	unsigned int DisplayChanged;
	unsigned int DisplayBufMask;

    int calcDispWin(RECTTYPE *original_border, RECTTYPE *original_win,
                    RECTTYPE *new_border, RECTTYPE *new_win);
    int calcDispWinWithAR(RECTTYPE *original_win, RECTTYPE *new_border,
                    RECTTYPE *new_win, int orig_rotation);
    bool switchTvOut(struct overlay_control_context_t * dev);
    bool switchSinVideoDualUI(struct overlay_control_context_t * dev);
    bool switchDualVideoSinUI(struct overlay_control_context_t * dev);
    bool switchNormalOverlay(struct overlay_control_context_t * dev);
    bool checkDispMode(struct overlay_control_context_t * dev);
    bool checkSecVideoParam(struct overlay_control_context_t *dev);
    int calcOutputParam(struct overlay_control_context_t * dev, overlay_object *overlayObj,
                        RECTTYPE *rect_out,  int * rotation);

    public:
    OverlayThread(struct overlay_control_context_t *dev)
        : Thread(false),m_dev(dev){
        memset(&mIPUInputParam,0,sizeof(mIPUInputParam));
        memset(&mIPUOutputParam,0,sizeof(mIPUOutputParam));
        memset(&mIPUHandle,0,sizeof(mIPUHandle));
		ClearBufMask = 0;
		DisplayBufMask =0;
		DisplayChanged = 0;
    }

    virtual void onFirstRef() {
        OVERLAY_LOG_FUNC;
        //run("OverlayThread", PRIORITY_URGENT_DISPLAY);
    }
    virtual bool threadLoop() ;

};

#endif
