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
 */

#ifndef __OVERLAY_UTILS_H__
#define __OVERLAY_UTILS_H__

#include <asm/page.h>

#define LOG_TAG "FslOverlay"

#define MAX_OVERLAY_INSTANCES 2
#define DEFAULT_OVERLAY_BUFFER_NUM 0
#define MAX_OVERLAY_BUFFER_NUM 24

#define DEFAULT_FB_DEV_NAME "/dev/graphics/fb0"
#define FB1_DEV_NAME "/dev/graphics/fb1"
#define V4L_DEV_NAME "/dev/video16"
#define DEFAULT_V4L_LAYER  3
#define TV_V4L_LAYER 4
#define OVERLAY_FB_NORMAL 2
#define OVERLAY_FB_TVOUT 1
#define DEFAULT_ALPHA_BUFFERS 2
#define DEFAULT_V4L_BUFFERS 3
#define MAX_V4L_BUFFERS 3

#define MAX_OVERLAY_INPUT_W 1280
#define MAX_OVERLAY_INPUT_H 720

#define OVERLAY_QUEUE_THRESHOLD 2

#define FULL_TRANSPARANT_VALUE 255

#define SHARED_CONTROL_MARKER             (0x4F564354) //OVCT
#define SHARED_DATA_MARKER             (0x4F564441) //OVDA

 
#define OVERLAY_LOG_INFO(format, ...) LOGI((format), ## __VA_ARGS__)
#define OVERLAY_LOG_ERR(format, ...) LOGE((format), ## __VA_ARGS__)
#define OVERLAY_LOG_WARN(format, ...) LOGW((format), ## __VA_ARGS__)

#ifdef OVERLAY_DEBUG_LOG
#define OVERLAY_LOG_RUNTIME(format, ...) LOGI((format), ## __VA_ARGS__)
#define OVERLAY_LOG_FUNC LOGI("%s: %s",  __FILE__, __FUNCTION__)
#else
#define OVERLAY_LOG_RUNTIME(format, ...) 
#define OVERLAY_LOG_FUNC
#endif


#define fourcc(a, b, c, d)\
         (((__u32)(a)<<0)|((__u32)(b)<<8)|((__u32)(c)<<16)|((__u32)(d)<<24))

#define OUT_PIX_FMT_RGB565  fourcc('R', 'G', 'B', 'P')  /*!< 1 6  RGB-5-6-5   */
#define OUT_PIX_FMT_BGR24   fourcc('B', 'G', 'R', '3')  /*!< 24  BGR-8-8-8    */
#define OUT_PIX_FMT_RGB24   fourcc('R', 'G', 'B', '3')  /*!< 24  RGB-8-8-8    */
#define OUT_PIX_FMT_BGR32   fourcc('B', 'G', 'R', '4')  /*!< 32  BGR-8-8-8-8  */
#define OUT_PIX_FMT_BGRA32  fourcc('B', 'G', 'R', 'A')  /*!< 32  BGR-8-8-8-8  */
#define OUT_PIX_FMT_RGB32   fourcc('R', 'G', 'B', '4')  /*!< 32  RGB-8-8-8-8  */
#define OUT_PIX_FMT_RGBA32  fourcc('R', 'G', 'B', 'A')  /*!< 32  RGB-8-8-8-8  */
#define OUT_PIX_FMT_ABGR32  fourcc('A', 'B', 'G', 'R')  /*!< 32  ABGR-8-8-8-8 */

#define OUT_PIX_FMT_YUYV    fourcc('Y', 'U', 'Y', 'V')  /*!< 16 YUV 4:2:2 */
#define OUT_PIX_FMT_UYVY    fourcc('U', 'Y', 'V', 'Y')  /*!< 16 YUV 4:2:2 */
#define OUT_PIX_FMT_YUV422P fourcc('4', '2', '2', 'P')  /*!< 16 YUV 4:2:2 */
#define OUT_PIX_FMT_YVU422P fourcc('Y', 'V', '1', '6')  /*!< 16 YVU 4:2:2 */
#define OUT_PIX_FMT_YUV444  fourcc('Y', '4', '4', '4')  /*!< 24 YUV 4:4:4 */
#define OUT_PIX_FMT_YUV420P fourcc('I', '4', '2', '0')  /*!< 12 YUV 4:2:0 */
#define OUT_PIX_FMT_YVU420P fourcc('Y', 'V', '1', '2')  /*!< 12 YVU 4:2:0 */
#define OUT_PIX_FMT_YUV420P2 fourcc('Y', 'U', '1', '2') /*!< 12 YUV 4:2:0 */
#define OUT_PIX_FMT_NV12    fourcc('N', 'V', '1', '2') /* 12  Y/CbCr 4:2:0  */
#define OUT_PIX_FMT_YUV420  fourcc('Y', 'U', '1', '2') /* 12  YUV 4:2:0     */
#define OUT_PIX_FMT_YVU420  fourcc('Y', 'V', '1', '2') /* 12  YVU 4:2:0     */

#define  ALIGN_PIXEL(x)  ((x+ 31) & ~31)
#define  ALIGN_PIXEL_128(x)  ((x+ 127) & ~127) 

inline size_t roundUpToPageSize(size_t x) {
        return (x + (PAGE_SIZE-1)) & ~(PAGE_SIZE-1);
}


struct OVERLAY_BUFFER{
    void *vir_addr;
    unsigned int phy_addr;
    unsigned int size;
};

struct WIN_REGION {
    int left;
    int right;
    int top;
    int bottom;
};

typedef struct _RECTTYPE {
    int nLeft; 
    int nTop;
    int nWidth;
    int nHeight;
} RECTTYPE;

class OverlayAllocator{
public:
    virtual ~OverlayAllocator(){}
    virtual int allocate(OVERLAY_BUFFER *overlay_buf, int size){ return -1;}
    virtual int deAllocate(OVERLAY_BUFFER *overlay_buf){  return -1;  }
    virtual int getHeapID(){  return 0;  }
};


#endif
