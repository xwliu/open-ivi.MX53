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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <linux/fb.h>
#include <sys/mman.h>
#include <linux/mxcfb.h>

#include <hardware/hardware.h>
#include <hardware/overlay.h>

#include <fcntl.h>
#include <errno.h>

#include <cutils/log.h>
#include <cutils/ashmem.h>
#include <cutils/atomic.h>
#include <cutils/properties.h>
#include <time.h>
#include <pthread.h>
#include <semaphore.h>

#include <utils/List.h>
#include <ui/PixelFormat.h>
//#include <utils/threads.h>

#include "overlay_utils.h"
#include "fsl_overlay.h"
#include "overlay_pmem.h"

static int bits_per_pixel(int32_t format);

using namespace android;

int fill_frame_back(char * frame,int frame_size, int xres, int yres, unsigned int pixelformat);

//class OverlayThread;

/*****************************************************************************/

struct overlay_data_context_t {
    struct overlay_data_device_t device;
    /* our private state goes below here */
    int control_shared_fd;//all overlay instances share the same control 
    int control_shared_size;
    int data_shared_fd;
    int data_shared_size;
    int width;
    int height;
    int32_t format;
    int num_buffer;//Number of buffers for overlay
    int queue_threshold;
    int buf_size;
    int buf_queued;
    OverlayAllocator *allocator;
    OVERLAY_BUFFER *overlay_buffer;
    overlay_control_shared_t  *control_shared;
    overlay_data_shared_t  *data_shared;
};

static int overlay_device_open(const struct hw_module_t* module, const char* name,
        struct hw_device_t** device);

static struct hw_module_methods_t overlay_module_methods = {
    open: overlay_device_open
};

struct overlay_module_t HAL_MODULE_INFO_SYM = {
    common: {
        tag: HARDWARE_MODULE_TAG,
        version_major: 1,
        version_minor: 0,
        id: OVERLAY_HARDWARE_MODULE_ID,
        name: "FSL i.MX Overlay module",
        author: "The Android Open Source Project",
        methods: &overlay_module_methods,
    }
};

static int  create_data_shared_data(overlay_data_shared_t **shared);
static void destroy_data_shared_data(int shared_fd, overlay_data_shared_t *shared, bool closefd);
static int  open_data_shared_data(overlay_data_context_t *ctx);
static void close_data_shared_data(overlay_data_context_t *ctx);

static int  create_control_shared_data(overlay_control_shared_t **shared);
static void destroy_control_shared_data(int shared_fd, overlay_control_shared_t *shared, bool closefd);
static int  open_control_shared_data(overlay_data_context_t *ctx);
static void close_control_shared_data(overlay_data_context_t *ctx);

/*****************************************************************************/

//#include "overlay_thread.h"

overlay_object::overlay_object(uint32_t w, uint32_t h, int32_t format,int control_fd, int control_size) {
    OVERLAY_LOG_FUNC;
    this->overlay_t::getHandleRef = getHandleRef;
    this->overlay_t::format = format;
    this->overlay_t::h = h;
    this->overlay_t::h_stride = h;
    this->overlay_t::w = w;
    this->overlay_t::w_stride = w; 
    
    mHandle.version = sizeof(native_handle);
    mHandle.width = w;
    mHandle.height = h;
    mHandle.format = format;
    mHandle.num_buffer = DEFAULT_OVERLAY_BUFFER_NUM;
    mHandle.numFds = 2;
    mHandle.numInts = 7; // extra ints we have in  our handle
    mHandle.buf_size = mHandle.width*mHandle.height*bits_per_pixel(format)/8;
    
    rotation = 0;
    outX = 0;
    outY = 0;
    outW = 0;
    outH = 0;
    zorder = 0;//Should be set from Layerbase's drawing state
    visible = 1;

    out_changed = false;

    //???creat share file for this obj
    mHandle.data_shared_fd = create_data_shared_data(&mDataShared);
    mHandle.data_shared_size = mDataShared->size;
    //The control fd is opened in overlay hal init.
    mHandle.control_shared_fd = control_fd;
    mHandle.control_shared_size = control_size;

    //init the crop setting
    mDataShared->crop_x = 0;
    mDataShared->crop_y = 0;
    mDataShared->crop_w = mHandle.width&0xFFFFFFF8;
    mDataShared->crop_h = mHandle.height&0xFFFFFFF8;
    mDataShared->num_buffer = mHandle.num_buffer;
    mDataShared->for_playback = false;
    OVERLAY_LOG_INFO("overlay_object num_buffer %d width %d,height %d,format %d,buf_size %d",
     mHandle.num_buffer,mHandle.width,mHandle.height,
     mHandle.format,mHandle.buf_size);

}

overlay_object::~overlay_object(){
    OVERLAY_LOG_INFO("~overlay_object()");
    //???delete this share file;
    
    destroy_data_shared_data(mHandle.data_shared_fd,mDataShared,true);

}

#include "overlay_thread.h"

/*
*   Fill the rgb alpha buffer with alpha_val
*/
static int fill_alpha_buffer(void *alpha_buf, int buf_w,
                             WIN_REGION *fill_region,char alpha_val);


unsigned long fmt_to_bpp(unsigned long pixelformat)
{
   unsigned long bpp;

   switch (pixelformat)
   {
      case OUT_PIX_FMT_RGB565:
      /*interleaved 422*/
      case OUT_PIX_FMT_YUYV:
      case OUT_PIX_FMT_UYVY:
      /*non-interleaved 422*/
      case OUT_PIX_FMT_YUV422P:
      case OUT_PIX_FMT_YVU422P:
           bpp = 16;
           break;

      case OUT_PIX_FMT_BGR24:
      case OUT_PIX_FMT_RGB24:
      case OUT_PIX_FMT_YUV444:
           bpp = 24;
           break;

      case OUT_PIX_FMT_BGR32:
      case OUT_PIX_FMT_BGRA32:
      case OUT_PIX_FMT_RGB32:
      case OUT_PIX_FMT_RGBA32:
      case OUT_PIX_FMT_ABGR32:
           bpp = 32;
           break;

      /*non-interleaved 420*/
      case OUT_PIX_FMT_YUV420P:
      case OUT_PIX_FMT_YVU420P:
      case OUT_PIX_FMT_YUV420P2:
      case OUT_PIX_FMT_NV12:
           bpp = 12;
           break;

      default:
          bpp = 8;
          break;
  }
  return bpp;
}


static int bpp_to_pixfmt(struct fb_var_screeninfo fb_var)
{
	int pixfmt = 0;

	if (fb_var.nonstd)
		return fb_var.nonstd;

	switch (fb_var.bits_per_pixel) {
	case 24:
		pixfmt = IPU_PIX_FMT_BGR24;
		break;
	case 32:
		pixfmt = IPU_PIX_FMT_BGR32;
		break;
	case 16:
		pixfmt = IPU_PIX_FMT_RGB565;
		break;
	}
	return pixfmt;
}

static int create_control_shared_data(overlay_control_shared_t **shared)
{
    OVERLAY_LOG_FUNC;
    int fd;
    // assuming sizeof(overlay_data_shared_t) < a single page
    int size = (sizeof(overlay_control_shared_t) + getpagesize()-1) & ~(getpagesize()-1);
    overlay_control_shared_t *p;

    if ((fd = ashmem_create_region("overlay_control", size)) < 0) {
        LOGE("Error!Failed to Create Overlay Shared control!\n");
        return fd;
    }

    p = (overlay_control_shared_t*)mmap(NULL, size, PROT_READ | PROT_WRITE,
                                MAP_SHARED, fd, 0);
    if (p == MAP_FAILED) {
        LOGE("Error!Failed to Map Overlay Shared control!\n");
        close(fd);
        return -1;
    }

    memset(p, 0, size);
    p->marker = SHARED_CONTROL_MARKER;
    p->size   = size;
    p->refCnt = 1;

    //Create the sem for control
    if(sem_init(&p->overlay_sem, 1, 0) != 0){
        OVERLAY_LOG_ERR("Error!init overlay_sem failed");
        munmap(p, size);
        close(fd);
        return -1;
    }

    *shared = p;
    return fd;
}

static void destroy_control_shared_data( int shared_fd, overlay_control_shared_t *shared, bool closefd )
{
    OVERLAY_LOG_FUNC;
    if (shared == NULL)
        return;

    // Last side deallocated releases the mutex, otherwise the remaining
    // side will deadlock trying to use an already released mutex
    if (android_atomic_dec(&shared->refCnt) == 1) {
        if (sem_destroy(&shared->overlay_sem)) {
            OVERLAY_LOG_ERR("Error!Failed to Close Overlay control Semaphore!\n");
        }
        shared->marker = 0;
    }

    if (munmap(shared, shared->size)) {
        OVERLAY_LOG_ERR("Error!Failed to Unmap Overlay Shared control!\n");
    }

    if (closefd && close(shared_fd)) {
        OVERLAY_LOG_ERR("Error!Failed to Close Overlay Shared control!\n");
    }
}

static int open_control_shared_data( overlay_data_context_t *ctx )
{
    OVERLAY_LOG_FUNC;
    int rc   = -1;
    int mode = PROT_READ | PROT_WRITE;
    int fd   = ctx->control_shared_fd;
    int size = ctx->control_shared_size;

    if (ctx->control_shared != NULL) {
        // Already open, return success
        OVERLAY_LOG_ERR("Error!Overlay Shared Data Already Open\n");
        return 0;
    }
    ctx->control_shared = (overlay_control_shared_t*)mmap(0, size, mode, MAP_SHARED, fd, 0);

    if (ctx->control_shared == MAP_FAILED) {
        OVERLAY_LOG_ERR("Error!Failed to Map Overlay Shared control!\n");
    } else if ( ctx->control_shared->marker != SHARED_CONTROL_MARKER ) {
        OVERLAY_LOG_ERR("Error!Invalid Overlay Shared control Marker!\n");
        munmap( ctx->control_shared, size);
    } else if ( (int)ctx->control_shared->size != size ) {
        OVERLAY_LOG_ERR("Error!Invalid Overlay Shared control Size!\n");
        munmap(ctx->control_shared, size);
    } else {
        android_atomic_inc(&ctx->control_shared->refCnt);
        rc = 0;
    }

    return rc;
}

static void close_control_shared_data(overlay_data_context_t *ctx)
{
    OVERLAY_LOG_FUNC;
    destroy_control_shared_data(ctx->control_shared_fd, ctx->control_shared, false);
    ctx->control_shared = NULL;
}

static int create_data_shared_data(overlay_data_shared_t **shared)
{
    OVERLAY_LOG_FUNC;
    int fd;
    // assuming sizeof(overlay_data_shared_t) < a single page
    int size = (sizeof(overlay_data_shared_t) + getpagesize()-1) & ~(getpagesize()-1);
    overlay_data_shared_t *p;

    if ((fd = ashmem_create_region("overlay_data", size)) < 0) {
        OVERLAY_LOG_ERR("Error!Failed to Create Overlay Shared Data!\n");
        return fd;
    }

    p = (overlay_data_shared_t*)mmap(NULL, size, PROT_READ | PROT_WRITE,
                                MAP_SHARED, fd, 0);
    if (p == MAP_FAILED) {
        OVERLAY_LOG_ERR("Error!Failed to Map Overlay Shared Data!\n");
        close(fd);
        return -1;
    }

    memset(p, 0, size);
    p->marker = SHARED_DATA_MARKER;
    p->size   = size;
    p->refCnt = 1;

    pthread_mutexattr_t mutex_attr;
    pthread_mutexattr_init(&mutex_attr);
    pthread_mutexattr_setpshared(&mutex_attr, PTHREAD_PROCESS_SHARED);
    if (pthread_mutex_init(&p->obj_lock, &mutex_attr) != 0) {
        OVERLAY_LOG_ERR("Error!Failed to Open Overlay Lock!\n");
        munmap(p, size);
        close(fd);
        return -1;
    }

    pthread_condattr_t cond_attr;
    pthread_condattr_init(&cond_attr);
    pthread_condattr_setpshared(&cond_attr, PTHREAD_PROCESS_SHARED);
    if (pthread_cond_init(&p->free_cond, &cond_attr) != 0) {
        OVERLAY_LOG_ERR("Error!Failed to Open Overlay Lock!\n");
        pthread_mutex_destroy(&p->obj_lock);
        munmap(p, size);
        close(fd);
        return -1;
    }

    *shared = p;
    return fd;
}

static void destroy_data_shared_data( int shared_fd, overlay_data_shared_t *shared, bool closefd )
{
    OVERLAY_LOG_INFO("destroy_data_shared_data shared %p closefd %d",shared,closefd);
    if (shared == NULL)
        return;

    // Last side deallocated releases the mutex, otherwise the remaining
    // side will deadlock trying to use an already released mutex
    if (android_atomic_dec(&shared->refCnt) == 1) {
        if (pthread_mutex_destroy(&shared->obj_lock)) {
            OVERLAY_LOG_ERR("Error!Failed to Close Overlay Semaphore!\n");
        }
        //???delete this cond;
        if (pthread_cond_destroy(&shared->free_cond)) {
            OVERLAY_LOG_ERR("Error!Failed to Close Overlay Semaphore!\n");
        }

        shared->marker = 0;
    }

    if (munmap(shared, shared->size)) {
        OVERLAY_LOG_ERR("Error!Failed to Unmap Overlay Shared Data!\n");
    }

    if (closefd && close(shared_fd)) {
        OVERLAY_LOG_ERR("Error!Failed to Close Overlay Shared Data!\n");
    }
}

static int open_data_shared_data( overlay_data_context_t *ctx )
{
    OVERLAY_LOG_FUNC;
    int rc   = -1;
    int mode = PROT_READ | PROT_WRITE;
    int fd   = ctx->data_shared_fd;
    int size = ctx->data_shared_size;

    if (ctx->data_shared != NULL) {
        // Already open, return success
        OVERLAY_LOG_ERR("Error!Overlay Shared Data Already Open\n");
        return 0;
    }
    ctx->data_shared = (overlay_data_shared_t*)mmap(0, size, mode, MAP_SHARED, fd, 0);

    if (ctx->data_shared == MAP_FAILED) {
        OVERLAY_LOG_ERR("Error!Failed to Map Overlay Shared Data!\n");
    } else if ( ctx->data_shared->marker != SHARED_DATA_MARKER ) {
        OVERLAY_LOG_ERR("Error!Invalid Overlay Shared Marker!\n");
        munmap( ctx->data_shared, size);
    } else if ( (int)ctx->data_shared->size != size ) {
        OVERLAY_LOG_ERR("Error!Invalid Overlay Shared Size!\n");
        munmap(ctx->data_shared, size);
    } else {
        android_atomic_inc(&ctx->data_shared->refCnt);
        rc = 0;
    }

    return rc;
}

static void close_data_shared_data(overlay_data_context_t *ctx)
{
    OVERLAY_LOG_FUNC;
    destroy_data_shared_data(ctx->data_shared_fd, ctx->data_shared, false);
    ctx->data_shared = NULL;
}

static int bits_per_pixel(int32_t format)
{
    int bits = 0;
    switch (format) {
        case HAL_PIXEL_FORMAT_RGB_888:
            bits = 24;
            break;
        case HAL_PIXEL_FORMAT_YCbCr_422_SP:
        case HAL_PIXEL_FORMAT_YCbCr_422_I:
        case HAL_PIXEL_FORMAT_RGB_565:
            bits = 16;
            break;
        case HAL_PIXEL_FORMAT_RGBA_8888:
            bits = 32;
            break;
        case HAL_PIXEL_FORMAT_YCbCr_420_SP:
        case HAL_PIXEL_FORMAT_YCbCr_420_I:
            bits = 12;
            break;
        default:
            bits = 0;
            break;
    }
    return bits;
}

static int fill_alpha_buffer(void *alpha_buf, int buf_w, 
                             WIN_REGION *fill_region,char alpha_val)
{
    //Check parameter
    if((!alpha_buf)||(buf_w <0)||(!fill_region)) {
        OVERLAY_LOG_ERR("Error!Invalid parameters in fill_alpha_buffer");
        return -1;
    }
    OVERLAY_LOG_INFO("fill_alpha_buffer: buf_w %d,top %d, bottom %d, left %d,right %d",buf_w,
         fill_region->top,fill_region->bottom,fill_region->left,fill_region->right);

    char *pPointAlphaValue;
	int x, y;

	for (y = fill_region->top; y < fill_region->bottom; y++) {
        pPointAlphaValue = (char *)alpha_buf +buf_w * y + fill_region->left;
        memset(pPointAlphaValue,alpha_val,fill_region->right-fill_region->left);
	}

    return 0;
}

//pixelformat format for v4l2 setting
int fill_frame_back(char * frame,int frame_size, int xres,
                           int yres, unsigned int pixelformat)
{
    int ret = 0;
    char * base;
    int j, screen_size;
    short * tmp;
    short color;
    if((xres<=0)||(yres<=0)||(!frame)) {
        OVERLAY_LOG_ERR("Error!Not valid parameters in fill_frame_back");
        return -1;
    }
    switch(pixelformat) {
        case V4L2_PIX_FMT_RGB565:
        case V4L2_PIX_FMT_RGB24:
        case V4L2_PIX_FMT_BGR24:
        case V4L2_PIX_FMT_RGB32:
        case V4L2_PIX_FMT_BGR32:
            memset(frame, 0, frame_size);
            break;
        case V4L2_PIX_FMT_YUYV:
        case V4L2_PIX_FMT_UYVY:
            tmp = (short *) frame;
            if(pixelformat == V4L2_PIX_FMT_YUYV)
               color = 0x8000;
            else
               color = 0x80;
            for(int i = 0; i < frame_size/2;i++, tmp++)
                *tmp = color;
            break;
        case V4L2_PIX_FMT_YUV422P:
            base = (char *)frame;
            screen_size = xres * yres;
            memset(base, 0, frame_size);
            base += screen_size;
            for (int i = 0; i < screen_size; i++, base++)
                *base = 0x80;
            break;
        case V4L2_PIX_FMT_YUV420:
        case V4L2_PIX_FMT_YVU420:
        case V4L2_PIX_FMT_NV12:
            base = (char *)frame;
            screen_size = xres * yres;
            memset(base, 0, frame_size);
            base += screen_size;
            for (int i = 0; i < screen_size/2; i++, base++)
                 *base = 0x80;
            break;
        defaule:
            OVERLAY_LOG_ERR("Error!Not supported pixel format");
            ret = -1;
            break;
    }
    return ret;
}

/*****************************************************************************/
#define SINGLE_DISPLAY_CAPABILITY  (1920 * 1080 * 60)
#define DUAL_DISPLAY_CAPABILITY    (1920 * 1080 * 30)

typedef struct
{
	char* start;
	char* end;
	int width;
	int height;
	int freq;
}
disp_mode;

static int str2int(char *p)
{
	int val = 0;
	if(!p) return -1;

	while(p[0] >= '0' && p[0] <= '9')
	{
		val = val * 10 + p[0] - '0';
		p++;
	}

	return val;
}

typedef enum {
    CHECK_NEXT_STATE,
    FIND_WIDTH_STATE,
    FIND_JOINT_STATE,
    FIND_HEIGHT_STATE,
    PREFIX_FREQ_STATE,
    FREQUENCY_STATE,
    FIND_NEWLINE_STATE
}
read_state;

static disp_mode disp_mode_list[128];
static int disp_mode_compare( const void *arg1, const void *arg2)
{
	disp_mode *dm1 = (disp_mode *)arg1;
	disp_mode *dm2 = (disp_mode *)arg2;

	if(dm1->width * dm1->height > dm2->width * dm2->height) return -1;
	if(dm1->width * dm1->height == dm2->width * dm2->height)
	{
		return dm1->freq > dm2->freq ? -1 : 1;
	}

	return 1;
}
static char* find_available_mode(const char *mode_list, int dual_disp)
{
	int disp_threshold = 0;
	int i,disp_mode_count = 0;
	read_state state = CHECK_NEXT_STATE;
	char *p = (char *)mode_list;

    if(!p) return NULL;

	while(p[0])
	{
		switch(state)
		{
		case CHECK_NEXT_STATE:
			if(!strncmp(p, "D:", 2)
				|| !strncmp(p, "S:", 2)
				|| !strncmp(p, "U:", 2)
				|| !strncmp(p, "V:", 2))
			{
				disp_mode_list[disp_mode_count].start = p;
				state = FIND_WIDTH_STATE;
				p+=2;
			}
			else p++;
			break;
		case FIND_WIDTH_STATE:
			if(p[0]>='0' && p[0]<='9')
			{
				disp_mode_list[disp_mode_count].width = str2int(p);
				state = FIND_JOINT_STATE;
			}
			p++;
			break;
		case FIND_JOINT_STATE:
			if(p[0] == 'x' || p[0] == 'X')
			{
				state = FIND_HEIGHT_STATE;
			}
			p++;
			break;
		case FIND_HEIGHT_STATE:
			if(p[0]>='0' && p[0]<='9')
			{
				disp_mode_list[disp_mode_count].height = str2int(p);
				state = PREFIX_FREQ_STATE;
			}
			p++;
			break;
		case PREFIX_FREQ_STATE:
			if(!strncmp(p, "p-", 2) || !strncmp(p, "i-", 2))
			{
				state = FREQUENCY_STATE;
				p+=2;
			}
			else p++;
			break;
		case  FREQUENCY_STATE:
			if(p[0]>='0' && p[0]<='9')
			{
				disp_mode_list[disp_mode_count].freq = str2int(p);
				state = FIND_NEWLINE_STATE;
			}
			p++;
			break;
		case FIND_NEWLINE_STATE:
			if(p[0] == '\n')
			{
				disp_mode_list[disp_mode_count++].end = p+1;
				state = CHECK_NEXT_STATE;
                if(disp_mode_count >= sizeof(disp_mode_list)/sizeof(disp_mode_list[0])) goto check_mode_end;
			}
			p++;
			break;
		default:
			p++;
			break;
		}
	}

check_mode_end:

	qsort(&disp_mode_list[0], disp_mode_count, sizeof(disp_mode), disp_mode_compare);

	disp_threshold = dual_disp ? DUAL_DISPLAY_CAPABILITY : SINGLE_DISPLAY_CAPABILITY;

	for(i=0; i<disp_mode_count; i++)
	{
		if(disp_mode_list[i].width * disp_mode_list[i].height * disp_mode_list[i].freq <= disp_threshold)
			break;
	}

	if(disp_mode_list[i].end) disp_mode_list[i].end[0] = 0;

    return disp_mode_list[i].start;
}

//Auto set the TV out mode based on the modes
int autoSetTVMode(int fb, int dual_disp)
{
    int size=0;
    int fp_cmd=0;
    int fp_mode=0;
    int fp_modes=0;
    char fb_mode[256];
    char fb_modes[1024];
    char cmd_line[1024];
    char temp_name[256];
    char *disp_mode=NULL;

    char value[PROPERTY_VALUE_MAX];

    LOGW("autoSetTVMode()*****");

    property_get("rw.AUTO_CONFIG_DISPLAY", value, "0");
    if (strcmp(value, "1") != 0)  return 0;

    fp_cmd = open("/proc/cmdline",O_RDONLY, 0);
    if(fp_cmd < 0) {
        LOGI("Error! Cannot open /proc/cmdline");
        goto set_graphics_fb_mode_error;
    }

    memset(cmd_line, 0, sizeof(cmd_line));
    size = read(fp_cmd, cmd_line, sizeof(cmd_line));
    if(size <= 0)
    {
        LOGI("Error! Cannot read /proc/cmdline");
        goto set_graphics_fb_mode_error;
    }

    close(fp_cmd); fp_cmd = 0;


    sprintf(temp_name, "/sys/class/graphics/fb%d/modes", fb);
    fp_modes = open(temp_name,O_RDONLY, 0);
    if(fp_modes < 0) {
        LOGI("Error! Cannot open %s", temp_name);
        goto set_graphics_fb_mode_error;
    }

    memset(fb_modes, 0, sizeof(fb_modes));
    size = read(fp_modes, fb_modes, sizeof(fb_modes));
    if(size <= 0)
    {
        LOGI("Error! Cannot read %s", temp_name);
        goto set_graphics_fb_mode_error;
    }

    close(fp_modes); fp_modes = 0;

    if(size == sizeof(fb_modes)) fb_modes[size -1] = 0;

    disp_mode = find_available_mode(fb_modes, dual_disp);
    if(!disp_mode)
    {
        LOGI("Error! Cannot find available mode for fb%d", fb);
        goto set_graphics_fb_mode_error;
    }

    LOGI("find fb%d available mode %s", fb,disp_mode);

    sprintf(temp_name, "/sys/class/graphics/fb%d/mode", fb);
    fp_mode = open(temp_name,O_RDWR, 0);
    if(fp_mode < 0) {
        LOGI("Error! Cannot open %s", temp_name);
        goto set_graphics_fb_mode_error;
    }

    memset(fb_mode, 0, sizeof(fb_mode));
    size = read(fp_mode, fb_mode, sizeof(fb_mode));
    if(size <= 0)
    {
        LOGI("Error! Cannot read %s", temp_name);
        goto set_graphics_fb_mode_error;
    }

    if(strncmp(fb_mode, disp_mode, strlen(fb_mode)))
    {
        write(fp_mode, disp_mode, strlen(disp_mode)+1);
    }

    close(fp_mode); fp_mode = 0;

    return 0;

set_graphics_fb_mode_error:

    if(fp_modes > 0) close(fp_modes);
    if(fp_mode > 0) close(fp_mode);
    if(fp_cmd > 0) close(fp_cmd);

    return -1;

}

int overlay_init_fbdev(struct overlay_control_context_t *dev, const char* dev_name)
{
    OVERLAY_LOG_FUNC;
    //Check fb0 dev
    dev->fb_dev  = open(dev_name, O_RDWR | O_NONBLOCK, 0);
    if(dev->fb_dev < 0) {
        OVERLAY_LOG_ERR("Error!Open fb device %s failed",dev_name);
        return -1;
    }
    
    int ret = 0;
    struct fb_var_screeninfo fb_var;

    if ( ioctl(dev->fb_dev, FBIOGET_VSCREENINFO, &fb_var) < 0) {
        OVERLAY_LOG_ERR("Error!VSCREENINFO getting failed for dev %s",dev_name);
        return -1;
    }
    OVERLAY_LOG_INFO("overlay_init_fbdev %s fb_var: bits_per_pixel %d,xres %d,yres %d,xres_virtual %d,yres_virtual %d\n",
            dev_name,fb_var.bits_per_pixel,
            fb_var.xres,fb_var.yres,
            fb_var.xres_virtual,fb_var.yres_virtual);
    dev->xres = fb_var.xres;
    dev->yres = fb_var.yres;

    if(strcmp(dev_name, DEFAULT_FB_DEV_NAME)==0){
        OVERLAY_LOG_INFO("save default fb resolution: %d,%d", dev->xres, dev->yres);
        dev->default_fb_resolution.nLeft = 0;
        dev->default_fb_resolution.nTop = 0;
        dev->default_fb_resolution.nWidth = dev->xres;
        dev->default_fb_resolution.nHeight = dev->yres;
    }
    else if(strcmp(dev_name, FB1_DEV_NAME)==0){
        OVERLAY_LOG_INFO("save fb1 resolution: %d,%d", dev->xres, dev->yres);
        dev->fb1_resolution.nLeft = 0;
        dev->fb1_resolution.nTop = 0;
        dev->fb1_resolution.nWidth = dev->xres;
        dev->fb1_resolution.nHeight = dev->yres;
    }
        
    return 0;
}

int overlay_deinit_fbdev(struct overlay_control_context_t *dev)
{
    OVERLAY_LOG_FUNC;
    if(!dev) {
        return -1;
    }
    if(dev->fb_dev) {
        close(dev->fb_dev);
        dev->fb_dev = 0;
    }
    return 0;
}

//Init the FB for output overlay
int overlay_init_output_fb(struct overlay_control_context_t *dev, int fb_num)
{
    struct fb_fix_screeninfo finfo;
    struct fb_var_screeninfo info;
    char *out_buf_addr[MAX_V4L_BUFFERS];
    char output_fb_name[32] = "/dev/graphics/fb";
    int fbSize;
    void *vaddr;
    int blank;
    int ret = 0;
	int try_times = 3;
    OVERLAY_LOG_FUNC;
    if(!dev) {
        return -1;
    }

    memset(output_fb_name, 0, 32);
    sprintf(output_fb_name, "/dev/graphics/fb%d", fb_num);
    LOGI("output fb name %s", output_fb_name);
    dev->output_fd = open(output_fb_name, O_RDWR | O_NONBLOCK, 0);
    //Get FB information
    if(dev->output_fd < 0) {
        OVERLAY_LOG_ERR("Error!Open output fb device %s failed", output_fb_name);
        return -1;
    }
    
    if ( ioctl(dev->output_fd, FBIOGET_VSCREENINFO, &info) < 0) {
        OVERLAY_LOG_ERR("Error!VSCREENINFO getting failed for dev %s",output_fb_name);
        return -1;
    }

    OVERLAY_LOG_INFO("overlay_init_output_fb %s fb_var: bits_per_pixel %d,xres %d,yres %d,xres_virtual %d,yres_virtual %d\n",
            output_fb_name, info.bits_per_pixel,
            info.xres, info.yres,
            info.xres_virtual, info.yres_virtual);

    //Set FB information
    if (ioctl(dev->output_fd, FBIOGET_FSCREENINFO, &finfo) == -1)
       goto output_init_error;
                
    info.xoffset = 0;
    info.yoffset = 0;
    info.bits_per_pixel = fmt_to_bpp(dev->outpixelformat);
    info.nonstd = dev->outpixelformat; 
    info.activate = FB_ACTIVATE_NOW;
    info.xres = dev->xres;
    info.yres = dev->yres;
#if 1	
    info.yres_virtual = ALIGN_PIXEL_128(info.yres) * MAX_V4L_BUFFERS;
    info.xres_virtual = ALIGN_PIXEL(info.xres);
#else 
	info.xres_virtual = info.xres;
#endif
    OVERLAY_LOG_INFO("overlay_init_output_fb %s fb_var: bits_per_pixel %d,xres %d,yres %d,xres_virtual %d,yres_virtual %d\n",
            output_fb_name, info.bits_per_pixel,
            info.xres, info.yres,
            info.xres_virtual, info.yres_virtual);
	OVERLAY_LOG_INFO("overlay_init_output_fb: yoffset:%d", info.yoffset);
	OVERLAY_LOG_INFO("overlay_init_output_fb: outpixelformat:%d", dev->outpixelformat);
	
linelength_err:                    
    if (ioctl(dev->output_fd, FBIOPUT_VSCREENINFO, &info) == -1) {
        OVERLAY_LOG_ERR("Error!output display FBIOPUT_VSCREENINFO");
        goto output_init_error;
    }
                    
    if (ioctl(dev->output_fd, FBIOGET_VSCREENINFO, &info) == -1){
        OVERLAY_LOG_ERR("Error!output display FBIOGET_VSCREENINFO");
        goto output_init_error;
    }
                    
    if (ioctl(dev->output_fd, FBIOGET_FSCREENINFO, &finfo) == -1){
        OVERLAY_LOG_ERR("Error!output display FBIOGET_FSCREENINFO");
    }
                    
    if(finfo.smem_len <= 0)
        goto output_init_error;

	LOGI("line_length:%d, yres_virtual:%d",
         finfo.line_length, info.yres_virtual);
	if(finfo.line_length != info.xres_virtual * info.bits_per_pixel / 8) {
		LOGI("linelength error");
		while(try_times -- ) {
			usleep(300000);
			goto linelength_err;
		}
	}
    //Map FB three buffers
    fbSize = roundUpToPageSize(finfo.line_length * info.yres_virtual);  
                   
    vaddr = mmap(0, fbSize, PROT_READ|PROT_WRITE, MAP_SHARED, dev->output_fd, 0);
    if (vaddr == MAP_FAILED) {
        OVERLAY_LOG_ERR("Error!mapping the framebuffer (%s)", strerror(errno));
        goto output_init_error;
    }

    dev->output_base = vaddr;
    dev->output_phys = finfo.smem_start;
    LOGI("virtual: 0x%x, phy 0x%x", vaddr, finfo.smem_start);
    //Fill FB buffers to black
    //memset(vaddr, 0, fbSize);
    fill_frame_back((char *)vaddr, fbSize, ALIGN_PIXEL(info.xres), ALIGN_PIXEL_128(info.yres), dev->outpixelformat);
    blank = FB_BLANK_UNBLANK;
    if(ioctl(dev->output_fd, FBIOBLANK, blank) < 0) {
       OVERLAY_LOG_ERR("Error!output init UNBLANK failed at first");
       //In case fb's late_resume not be called at this when just wakeup
       usleep(300000);
       if(ioctl(dev->output_fd, FBIOBLANK, blank) < 0) {
            OVERLAY_LOG_ERR("Error!output init UNBLANK failed at second");
            goto output_init_error;
       }
    }

    dev->output_frame_size = fbSize/MAX_V4L_BUFFERS;
    dev->output_next_buf = ((info.yoffset * MAX_V4L_BUFFERS / info.yres_virtual) + 1) % MAX_V4L_BUFFERS;

	LOGI("init output_next_buf %d", dev->output_next_buf);
    return 0;

output_init_error:
    if(dev->output_fd ){
        close(dev->output_fd);
        dev->output_fd = 0;
    }
    return -1;
}

//Fectch an output buffer for overlay mixing
int output_device_fetch(struct overlay_control_context_t *dev, void **pOutVir, unsigned int *pOutPhy)
{
    OVERLAY_LOG_FUNC;

    if((!dev)||(dev->output_fd <= 0)) {
        OVERLAY_LOG_ERR("Error!Cannot fetch output buffer!");
        return -1;
    }

    *pOutVir = dev->output_base + dev->output_frame_size*dev->output_next_buf;
    *pOutPhy = dev->output_phys + dev->output_frame_size*dev->output_next_buf;

    return 0;
}

//Post an output buffer after overlay mixing
int output_device_post(struct overlay_control_context_t *dev)
{
    OVERLAY_LOG_FUNC;

    if((!dev)||(dev->output_fd <= 0)) {
        OVERLAY_LOG_ERR("Error!Cannot post output buffer!");
        return -1;
    }

    struct fb_var_screeninfo info;
    if(ioctl(dev->output_fd, FBIOGET_VSCREENINFO, &info) < 0) {
        OVERLAY_LOG_ERR("Error! output_device::post VSCREENINFO getting failed!");
        return -1;
    }

    struct fb_fix_screeninfo finfo;
    if(ioctl(dev->output_fd, FBIOGET_FSCREENINFO, &finfo) < 0) {
        OVERLAY_LOG_ERR("Error! output_device::post FSCREENINFO getting failed!");
        return -1;
    }

    info.yoffset = dev->output_next_buf*ALIGN_PIXEL_128(dev->yres);
    dev->output_next_buf = (dev->output_next_buf + 1) % MAX_V4L_BUFFERS; 
    info.activate = FB_ACTIVATE_VBL;
    ioctl(dev->output_fd, FBIOPAN_DISPLAY, &info);

    return 0;
}

//Deinit the output FB device
int overlay_deinit_output_fb(struct overlay_control_context_t *dev)
{
    OVERLAY_LOG_FUNC;

    if(!dev) {
        return -1;
    }

    int blank = 1;

    if(ioctl(dev->output_fd, FBIOBLANK, blank) < 0) {
        return -1;
    }

    //Unmap the output FB buffers
    munmap(dev->output_base, dev->output_frame_size * MAX_V4L_BUFFERS);
    //Close output FB fd
    close(dev->output_fd);

    dev->output_base = 0;
    dev->output_phys = 0;
    dev->output_frame_size = 0;
    dev->output_next_buf = 0;
    dev->output_fd = 0;

    return 0;
} 

int overlay_init_v4l(struct overlay_control_context_t *dev, int layer)
{
        OVERLAY_LOG_FUNC;
        //Open v4l2 device
        dev->v4l_id = open(V4L_DEV_NAME, O_RDWR, 0);
        if(dev->v4l_id < 0) {
            OVERLAY_LOG_ERR("Error!Open v4l device %s failed",V4L_DEV_NAME);
            return -1;
        }

        if ( ioctl(dev->v4l_id,VIDIOC_S_OUTPUT, &layer) < 0) {
    	    OVERLAY_LOG_ERR("Error!VIDIOC_S_OUTPUT getting failed for dev %s",V4L_DEV_NAME);
            return -1;
        }

        struct v4l2_cropcap cropcap;
        memset(&cropcap, 0, sizeof(cropcap));
        cropcap.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
        if ( ioctl(dev->v4l_id, VIDIOC_CROPCAP, &cropcap) < 0) {
    	    OVERLAY_LOG_ERR("Error!VIDIOC_CROPCAP getting failed for dev %s",V4L_DEV_NAME);
            return -1;
        }
        dev->video_frames = 0;

        struct v4l2_crop crop;
        /* set the image rectangle of the display by 
        setting the appropriate parameters */
        crop.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
        crop.c.width = dev->xres;
        crop.c.height = dev->yres;
        crop.c.top = 0;
        crop.c.left = 0;
        if ( ioctl(dev->v4l_id, VIDIOC_S_CROP, &crop) < 0) {
    	    OVERLAY_LOG_ERR("Error!VIDIOC_CROPCAP getting failed for dev %s",V4L_DEV_NAME);
            return -1;
        }

        //Set V4L format
        struct v4l2_format fmt;
        //struct v4l2_mxc_offset off;
        memset(&fmt, 0, sizeof(fmt));
        OVERLAY_LOG_INFO("init v4l parameters: w %d, h %d", dev->xres, dev->yres);
        fmt.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
        fmt.fmt.pix.width = dev->xres;
        fmt.fmt.pix.height = dev->yres;
        fmt.fmt.pix.pixelformat = dev->outpixelformat;//in_fmt;
        fmt.fmt.pix.bytesperline = dev->xres;
        fmt.fmt.pix.priv = 0;
        fmt.fmt.pix.sizeimage = 0;//dev->xres * dev->yres * 3 / 2

        if ( ioctl(dev->v4l_id, VIDIOC_S_FMT, &fmt) < 0) {
            OVERLAY_LOG_ERR("Error!VIDIOC_S_FMT setting failed for dev %s",V4L_DEV_NAME);
            return -1;
        }

        if ( ioctl(dev->v4l_id, VIDIOC_G_FMT, &fmt) < 0) {
            OVERLAY_LOG_ERR("Error!VIDIOC_G_FMT setting failed for dev %s",V4L_DEV_NAME);
            return -1;
        }
        OVERLAY_LOG_INFO("V4L setting: format 0x%x",fmt.fmt.pix.pixelformat);
        struct v4l2_requestbuffers buf_req;
        dev->v4l_bufcount = DEFAULT_V4L_BUFFERS;
        buf_req.count = dev->v4l_bufcount;
        buf_req.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
        buf_req.memory = V4L2_MEMORY_MMAP;
        if ( ioctl(dev->v4l_id, VIDIOC_REQBUFS, &buf_req) < 0) {
            OVERLAY_LOG_ERR("Error!VIDIOC_REQBUFS setting failed for dev %s",V4L_DEV_NAME);
            return -1;
        }


        //for each buffer,get the vir/phy address
        struct v4l2_buffer *v4lbuf = dev->v4l_buffers;
        char * vir_addr;
        OVERLAY_LOG_RUNTIME("dev->v4l_buffers 0x%x v4l2_buffer size %d",dev->v4l_buffers,sizeof(struct v4l2_buffer));
        for(int buf_index=0;buf_index < dev->v4l_bufcount;buf_index++) {
            v4lbuf->index = buf_index;
            v4lbuf->type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
            v4lbuf->memory = V4L2_MEMORY_MMAP;
            if( ioctl(dev->v4l_id, VIDIOC_QUERYBUF, v4lbuf) < 0) {
                OVERLAY_LOG_ERR("Error!VIDIOC_QUERYBUF getting failed for dev %s",V4L_DEV_NAME);
                return -1;
            }

            vir_addr = (char *)mmap(NULL,v4lbuf->length,
                            PROT_READ | PROT_WRITE, MAP_SHARED,
                            dev->v4l_id, v4lbuf->m.offset);
            if ((int)vir_addr == -1) {
                OVERLAY_LOG_INFO("mmap V4L buffer %d failed for dev %s",buf_index,V4L_DEV_NAME);
                return -1;
            }

            dev->v4lbuf_addr[buf_index] = vir_addr;
            //v4l already init this buffer to black
            //fill the v4l to black;
            fill_frame_back(vir_addr,v4lbuf->length,dev->xres,dev->yres,dev->outpixelformat);
            //LOGI("******0x%x 0x%x 0x%x 0x%x",*(vir_addr),*(vir_addr+1),*(vir_addr+2),*(vir_addr+3));
            OVERLAY_LOG_INFO("v4l buf[%d] 0x%p: vir 0x%p,phy 0x%x, size %d",
                             buf_index,v4lbuf,vir_addr,v4lbuf->m.offset,v4lbuf->length);
            v4lbuf++;
        }

        return 0;
}

//Fetch a v4l2 output buffer for ipu mixer
int v4l2_device_fetch(struct overlay_control_context_t *dev, struct v4l2_buffer *pV4LBuf)
{
    OVERLAY_LOG_FUNC;
    if(!pV4LBuf){
        OVERLAY_LOG_ERR("Error!v4l2_device_fetch() parameters error!");    
        return -1;
    }

    if(dev->video_frames < dev->v4l_bufcount) {
        //pV4LBuf = &m_dev->v4l_buffers[m_dev->video_frames];
        memcpy(pV4LBuf, &dev->v4l_buffers[dev->video_frames], sizeof(struct v4l2_buffer));
    }
    else{
        pV4LBuf->type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
        pV4LBuf->memory = V4L2_MEMORY_MMAP;
        if(ioctl(dev->v4l_id, VIDIOC_DQBUF, pV4LBuf) < 0){
            OVERLAY_LOG_ERR("Error!Cannot DQBUF a buffer from v4l");
    
            //stream off it,so to make it recover
            int type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
            ioctl(dev->v4l_id, VIDIOC_STREAMOFF, &type);
            dev->stream_on = false;
            dev->video_frames = 0;
            return -2;
        }
    }
    return 0;
}

//Post a v4l2 output buffer for display
int v4l2_device_post(struct overlay_control_context_t *dev, struct v4l2_buffer *pV4LBuf)
{
    OVERLAY_LOG_FUNC;
    gettimeofday(&pV4LBuf->timestamp, 0);
    if(ioctl(dev->v4l_id, VIDIOC_QBUF, pV4LBuf) < 0){
        OVERLAY_LOG_ERR("Error!Cannot QBUF a buffer from v4l");
        return -1;
    }
    OVERLAY_LOG_RUNTIME("QBUF from v4l at frame %d:index %d, phy 0x%x at sec %d usec %d",
                    dev->video_frames,pV4LBuf->index,pV4LBuf->m.offset,
                    pV4LBuf->timestamp.tv_sec,pV4LBuf->timestamp.tv_usec); 
    
    
    //Only stream on after two frames queued 
    dev->video_frames++;
    if((dev->video_frames>=2)&&(!dev->stream_on)) {
        int type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
        ioctl(dev->v4l_id, VIDIOC_STREAMON, &type);
        dev->stream_on = true;
        OVERLAY_LOG_INFO("V4L STREAMON NOW");
    }
    
    return 0;
}

int overlay_deinit_v4l(struct overlay_control_context_t *dev)
{
    OVERLAY_LOG_FUNC;

    if(!dev) {
        return -1;
    }
    for(int buf_index=0;buf_index < dev->v4l_bufcount;buf_index++) {
       if(dev->v4lbuf_addr[buf_index])
          munmap((void *)dev->v4lbuf_addr[buf_index], dev->v4l_buffers[buf_index].length);
       memset(&dev->v4l_buffers[buf_index],0,sizeof(v4l2_buffer));
       dev->v4lbuf_addr[buf_index] = NULL;
    }
    dev->v4l_bufcount = 0;

    if(dev->v4l_id) {
       close(dev->v4l_id);
       dev->v4l_id = 0;
    }
    dev->video_frames = 0;
    return 0;
}

int overlay_init_output(struct overlay_control_context_t *dev, int fb_num)
{
    #ifdef OVERLAY_V4L_OUT
    int layer;
    if(fb_num == OVERLAY_FB_NORMAL)
        layer = DEFAULT_V4L_LAYER;
    else
        layer = TV_V4L_LAYER;
    return overlay_init_v4l(dev, layer);
    #else
    return overlay_init_output_fb(dev, fb_num);
    #endif
}

int overlay_deinit_output(struct overlay_control_context_t *dev)
{
    #ifdef OVERLAY_V4L_OUT
    return overlay_deinit_v4l(dev);
    #else
    return overlay_deinit_output_fb(dev);
    #endif
}

int overlay_check_parameters(struct handle_t *overlay_handle)
{
    OVERLAY_LOG_FUNC;
    //Make sure the parameters are within our support, as alinement or 
    //something else
    return 0;
}
// ****************************************************************************
// Control module
// ****************************************************************************

static int overlay_get(struct overlay_control_device_t *dev, int name) {
    OVERLAY_LOG_FUNC;
    int result = -1;
    switch (name) {
        case OVERLAY_MINIFICATION_LIMIT:
            result = 0; // 0 = no limit
            break;
        case OVERLAY_MAGNIFICATION_LIMIT:
            result = 0; // 0 = no limit
            break;
        case OVERLAY_SCALING_FRAC_BITS:
            result = 0; // 0 = infinite
            break;
        case OVERLAY_ROTATION_STEP_DEG:
            result = 90; // 90 rotation steps (for instance)
            break;
        case OVERLAY_HORIZONTAL_ALIGNMENT:
            result = 1; // 1-pixel alignment
            break;
        case OVERLAY_VERTICAL_ALIGNMENT:
            result = 1; // 1-pixel alignment
            break;
        case OVERLAY_WIDTH_ALIGNMENT:
            result = 1; // 1-pixel alignment
            break;
        case OVERLAY_HEIGHT_ALIGNMENT:
            result = 1; // 1-pixel alignment
            break;
    }
    return result;
}

/*******************************************************************************/
/**********************       aibing debug *****************************************/

#define ENABLE_VIDEO_TVOUT_AUTO	1

#if ENABLE_VIDEO_TVOUT_AUTO
bool enabled_sec_disp;//whether software sec disp is ready for use
bool sec_disp_connected;//whether physical connected on sec disp
bool to_disable_overlay;//whether to disable overlay when video playback
bool to_tvout_display;//whether video ready to use fb1 for video on sec display
bool video_playing;
static void setOverlayOutput()
{
	char value[PROPERTY_VALUE_MAX] = {0};

   //Make the overlay to use FB1 for output
   
   if(sec_disp_connected&&to_disable_overlay){
		property_get("rw.DUAL_VIDEO_SIN_UI", value, "0");
		if (!strcmp(value, "1")) {
			property_set("sys.VIDEO_TVOUT_DISPLAY", "0");
			to_tvout_display = 0;
		} else {
			if(to_tvout_display){
				property_set("sys.VIDEO_TVOUT_DISPLAY", "1");
				to_tvout_display = 1;
				LOGI("enable VIDEO  TVOUT MODE");
			}
	     }
   }
   else{
        if(to_tvout_display){
            property_set("sys.VIDEO_TVOUT_DISPLAY", "0");
            to_tvout_display = 0;
            LOGI("disable VIDEO  TVOUT MODE");
        }
   }

   if(sec_disp_connected){
       //Set the prop rw.SECOND_DISPLAY_ENABLED to 1
       if(!enabled_sec_disp){
            property_set("sys.SECOND_DISPLAY_ENABLED", "1");
            enabled_sec_disp = 1;
            LOGI("Enable sec display mode");
       }
   }
   else{
       if(enabled_sec_disp){
            property_set("sys.SECOND_DISPLAY_ENABLED", "0");
            enabled_sec_disp = 0;
            LOGI("Disable sec display mode");
       }
   }
   LOGI("enabled_sec_disp:%d to_tvout_display:%d", enabled_sec_disp, to_tvout_display);
   return;
}

static int isStartSecFramebuffer(){
   int isStart = 0;
   int secDispConnected = 0;
   int videoPlaying = 0;
   int disableOverlay = 0;

   //Check the prop rw.SECOND_DISPLAY_CONNECTED
   char value[PROPERTY_VALUE_MAX] = {0};

	property_get("rw.secfb.disable-overlay", value, "0");
	if (strcmp(value, "1") == 0) {
		 to_disable_overlay = 1;
	}
	else{
		 to_disable_overlay = 0;
	}
	
	if ( video_playing == 1) {
		 property_set("rw.SECOND_DISPLAY_CONNECTED", "1");
		 sec_disp_connected = 1;
	}
	else{
		 property_set("rw.SECOND_DISPLAY_CONNECTED", "0");
		 sec_disp_connected = 0;
	}
	LOGI("sec_disp_connected:%d	to_disable_overlay:%d", sec_disp_connected, to_disable_overlay);
   return isStart;
}

int close_tve_output()
{
	char overlayStr[32];
	int blank;
	int fb2_fp;
	int sec_fp;

	int fp_property = open("/sys/class/graphics/fb1/fsl_disp_property",O_RDWR, 0); 
	if(fp_property >= 0) {
		struct fb_var_screeninfo fb0_var;
	
		blank = 1;
	
		fb2_fp = open("/dev/graphics/fb2",O_RDWR, 0);
		if (fb2_fp < 0){
			LOGE("Error!Cannot open the /dev/graphics/fb2");
		}
		else{
			if(ioctl(fb2_fp, FBIOBLANK, blank) < 0) {
				LOGI("Error!BLANK FB2 failed!\n");
			}
			close(fb2_fp);
		}
	
		sec_fp = open("/dev/graphics/fb1",O_RDWR, 0);
		if (sec_fp < 0){
			LOGE("Error!Cannot open the /dev/graphics/fb1 for second display");
			return 0;
		}
	
		if(ioctl(sec_fp, FBIOBLANK, blank) < 0) {
			LOGI("Error!BLANK FB1 failed!\n");
		}
	
		memset(overlayStr, 0 ,32);
		strcpy(overlayStr, "1-layer-fb\n");
		LOGI("WRITE 1-layer-fb to fb1/fsl_disp_property");
		write(fp_property, overlayStr, strlen(overlayStr)+1);
		close(fp_property);
	
		blank = FB_BLANK_POWERDOWN;
		if(ioctl(sec_fp, FBIOBLANK, blank) < 0) {
			LOGI("Error!BLANK FB1 failed!\n");
		}
	}
	close(sec_fp);
	sec_fp = 0;
	return 0;
}
#endif
/**********************       aibing debug *****************************************/
/*******************************************************************************/

static overlay_t* overlay_createOverlay(struct overlay_control_device_t *dev, uint32_t w, uint32_t h, int32_t format) 
{
    OVERLAY_LOG_FUNC;
    overlay_control_context_t *ctx = (overlay_control_context_t *)dev;
    overlay_object* overlay = NULL;
    OVERLAY_LOG_INFO("overlay_createOverlay w %d,h %d,format %d, pid %d,,gettid() %d",w,h,format,getpid(),gettid());

    property_set("sys.VIDEO_PLAYING", "1");
#if ENABLE_VIDEO_TVOUT_AUTO
	video_playing = 1;
	isStartSecFramebuffer();
	setOverlayOutput();
#endif
    if((!ctx)||(!ctx->overlay_running)){
        OVERLAY_LOG_ERR("Error!overlay_control_device_t not in good state");
        return NULL;
    }
    /* check the input params, reject if not supported or invalid */
    switch (format) {
        case HAL_PIXEL_FORMAT_RGB_888:
             break;
        case HAL_PIXEL_FORMAT_RGBA_8888:
        case HAL_PIXEL_FORMAT_YCbCr_422_SP:
            OVERLAY_LOG_ERR("Error!Not a valid format for overlay");
            return NULL;
        case HAL_PIXEL_FORMAT_RGB_565:
            break;
        case HAL_PIXEL_FORMAT_YCbCr_420_SP:
            break;
        case HAL_PIXEL_FORMAT_YCbCr_420_I:
            break;
        case HAL_PIXEL_FORMAT_YCbCr_422_I:
            break;
        default:
            OVERLAY_LOG_ERR("Error!Not a valid format for overlay");
            return NULL;
    }
    
    /* Create overlay object. Talk to the h/w here and adjust to what it can
     * do. the overlay_t returned can  be a C++ object, subclassing overlay_t
     * if needed.
     * 
     * we probably want to keep a list of the overlay_t created so they can
     * all be cleaned up in overlay_close(). 
     */

    overlay = new overlay_object( w, h, format,ctx->control_shared_fd,ctx->control_shared_size);
    if(overlay) {
        int instance = 0;

        pthread_mutex_lock(&ctx->control_lock);
        //Only init v4l or output fb as needed
        #ifdef OVERLAY_V4L_OUT
        if(ctx->overlay_number == 0 && (ctx->v4l_id == 0)) {
        #else
        if(ctx->overlay_number == 0 && (ctx->output_fd == 0)) {
        #endif
             char *fb_name = DEFAULT_FB_DEV_NAME;
             //Reinit the fb dev since it may be destoryed in display mode change
             if(ctx->display_mode == DISPLAY_MODE_TV){
                fb_name = FB1_DEV_NAME;
             }

             overlay_deinit_fbdev(ctx);
             if(overlay_init_fbdev(ctx, fb_name)<0){
                OVERLAY_LOG_ERR("Error!init fbdev %s failed", fb_name);
                return false;
             }

             if(overlay_init_output(ctx, (ctx->display_mode == DISPLAY_MODE_TV) ? OVERLAY_FB_TVOUT : OVERLAY_FB_NORMAL)<0){
                 OVERLAY_LOG_ERR("Error!init output failed");
                 pthread_mutex_unlock(&ctx->control_lock);
                 delete overlay;
                 return NULL;
             }
        }

        while(instance < MAX_OVERLAY_INSTANCES) {
            if(!ctx->overlay_instance_valid[instance]) {
                ctx->overlay_instance_valid[instance] = 1;
                ctx->overlay_intances[instance] = overlay;
                ctx->overlay_number++;
                OVERLAY_LOG_INFO("Create overlay instance 0x%p id %d total %d",
                     overlay,instance,ctx->overlay_number);
                overlay->mDataShared->instance_id = instance;
                break;
            }
            instance++;
        }
        pthread_mutex_unlock(&ctx->control_lock);

        if(instance >= MAX_OVERLAY_INSTANCES) {
            OVERLAY_LOG_ERR("Error!Cannot have more overlay instance in system");
            delete overlay;
            overlay = NULL;
        }
    }
    else{
        OVERLAY_LOG_ERR("Error!overlay_object creation failed w:%d,h%d,format:%d",w,h,format);
    }

    return (overlay_t *)overlay;
}

static void overlay_destroyOverlay(struct overlay_control_device_t *dev,
         overlay_t* overlay) 
{
    struct timespec timeout;
    struct timeval tv;
    OVERLAY_LOG_INFO("overlay_destroyOverlay()");
    overlay_control_context_t *ctx = (overlay_control_context_t *)dev;
    int instance = 0;
    overlay_object *obj = static_cast<overlay_object *>(overlay);

    pthread_mutex_lock(&ctx->control_lock);

    for(instance = 0;instance < MAX_OVERLAY_INSTANCES;instance++) {
        if((ctx->overlay_instance_valid[instance])&&(ctx->overlay_intances[instance] == obj)) {
            break;
        }
    }
    
    if(instance < MAX_OVERLAY_INSTANCES) {
        OVERLAY_LOG_INFO("****Destory the overlay instance id %d",instance);
        //Set a flag to indicate the overlay_obj is invalid.
        //Flush the buffer in queue
        overlay_data_shared_t *data_shared = ctx->overlay_intances[instance]->mDataShared;
        if(data_shared != NULL) {
            //Unlock the control lock incase the dead lock between overlay thread
            pthread_mutex_unlock(&ctx->control_lock);
            pthread_mutex_lock(&data_shared->obj_lock);
            data_shared->in_destroy = true;
            while(data_shared->queued_count > 0) {
                OVERLAY_LOG_WARN("Warning!destroyOverlay Still %d buffer in queue",
                                data_shared->queued_count);
                //Wait a buffer be mixered
                data_shared->wait_buf_flag = 1;
                //post sempore to notify mixer thread, give mixer thread a chance to free a buffer
                if(ctx->control_shared) {
                    OVERLAY_LOG_INFO("call sem post in destroy");
                    sem_post(&ctx->control_shared->overlay_sem);
                }
                gettimeofday(&tv, (struct timezone *) NULL);
                timeout.tv_sec = tv.tv_sec;
                timeout.tv_nsec = (tv.tv_usec + 200000) * 1000L;//200ms
                //Overlay data close or Overlay destroy may both block in this, so only one can get the condition
                //Make a time out here.
                pthread_cond_timedwait(&data_shared->free_cond, &data_shared->obj_lock,&timeout);
                if(data_shared->wait_buf_flag != 0) {
                    OVERLAY_LOG_ERR("Error!cannot make a buffer flushed for destory overlay");
               }
            }

            if(data_shared->buf_mixing) {
                int wait_count = 0;
                LOGW("Current this overlay is in buf_mixing! Have to wait it done");
                pthread_mutex_unlock(&data_shared->obj_lock);
                //Make a sleep for 10ms
                do{
                    usleep(2000);
                    wait_count++;
                    if(wait_count > 5) {
                        OVERLAY_LOG_ERR("Error!Still cannot wait the buf mix done!");
                        break;
                    }
                }while (data_shared->buf_mixing);
            }
            else{
                pthread_mutex_unlock(&data_shared->obj_lock);
            }

            pthread_mutex_lock(&ctx->control_lock);
        }

        ctx->overlay_number--;
        ctx->overlay_instance_valid[instance] = 0;
        ctx->overlay_intances[instance] = NULL;

    }

    #ifdef OVERLAY_V4L_OUT
    if((ctx->overlay_number == 0)&& ctx->stream_on) {
        int type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
        ioctl(ctx->v4l_id, VIDIOC_STREAMOFF, &type);
        OVERLAY_LOG_INFO("V4L STREAMON OFF");
        ctx->video_frames = 0;
        ctx->stream_on = false;
        //refill the back color
        for(int i = 0;i < ctx->v4l_bufcount;i++) {
            fill_frame_back(ctx->v4lbuf_addr[i],ctx->v4l_buffers[i].length,
                            ctx->xres,ctx->yres,ctx->outpixelformat);
        }
    }
    #endif

    if(ctx->overlay_number == 0) {
        overlay_deinit_output(ctx);
        overlay_deinit_fbdev(ctx);

        #ifdef OVERLAY_V4L_OUT
        if(ctx->display_mode == DISPLAY_MODE_TV){
            //A workaround when previouse v4l is based on
            //TV_V4L_LAYER
            if(overlay_init_v4l(ctx, DEFAULT_V4L_LAYER)<0){
                OVERLAY_LOG_ERR("Error!init v4l failed");
                //return false;
            }
        }
        overlay_deinit_v4l(ctx);
        #endif
        ctx->display_mode = DISPLAY_MODE_NORMAL;
    }

    if (ctx->video_play_mode == DUAL_VIDEO_SIN_UI && ctx->sec_disp_enable) {
        OVERLAY_LOG_WARN("uninit the sec_video_mIPUHandle");
        mxc_ipu_lib_task_uninit(&(ctx->sec_video_IPUHandle));
        memset(&(ctx->sec_video_IPUHandle), 0, sizeof(ipu_lib_handle_t));
		ctx->sec_disp_enable = false;
        ctx->display_mode = DISPLAY_MODE_NORMAL;

        int fb_dev;
        fb_dev  = open(FB1_DEV_NAME, O_RDWR | O_NONBLOCK, 0);
        if(fb_dev < 0) {
            OVERLAY_LOG_ERR("Error!Open fb device %s failed",FB1_DEV_NAME);
        }

        struct fb_fix_screeninfo fb_fix;
        if ( ioctl(fb_dev, FBIOGET_FSCREENINFO, &fb_fix) < 0) {
            OVERLAY_LOG_ERR("Error!FSCREENINFO getting failed for dev %s",FB1_DEV_NAME);
            close(fb_dev);
        }

        struct fb_var_screeninfo fb_var;
        if ( ioctl(fb_dev, FBIOGET_VSCREENINFO, &fb_var) < 0) {
            OVERLAY_LOG_ERR("Error!VSCREENINFO getting failed for dev %s",FB1_DEV_NAME);
            close(fb_dev);
        }

        void* vaddr = mmap(0, fb_fix.smem_len, PROT_READ|PROT_WRITE, MAP_SHARED, fb_dev, 0);
        if (vaddr == MAP_FAILED) {
            OVERLAY_LOG_ERR("Error mapping the fb1");
        }

        int i = 0;
        for (i = 0; i < 3; i++)
            fill_frame_back((char*)vaddr+i*fb_var.xres*fb_var.yres*fb_var.bits_per_pixel/8,
	                    fb_var.xres*fb_var.yres*fb_var.bits_per_pixel/8, fb_var.xres, fb_var.yres, bpp_to_pixfmt(fb_var));

        munmap(vaddr, fb_fix.smem_len);
        close(fb_dev);
    }

    /* free resources associated with this overlay_t */
    delete obj;

    pthread_mutex_unlock(&ctx->control_lock);
#if ENABLE_VIDEO_TVOUT_AUTO
	close_tve_output();
#endif	
    property_set("sys.VIDEO_PLAYING", "0");
#if ENABLE_VIDEO_TVOUT_AUTO
	video_playing = 0;
	isStartSecFramebuffer();
	setOverlayOutput();
#endif	

}

static int overlay_setPosition(struct overlay_control_device_t *dev,
         overlay_t* overlay, 
         int x, int y, uint32_t w, uint32_t h) {
    OVERLAY_LOG_FUNC;
    /* set this overlay's position (talk to the h/w) */
    overlay_object *obj = static_cast<overlay_object *>(overlay);
    overlay_control_context_t *ctx = (overlay_control_context_t *)dev;

    if (x <0 || y <0 )
    {
        OVERLAY_LOG_ERR ("!!!!Overlay pos set: x %d,y %d,w %d,h %d",x,y,w,h);
        return 0;
    }

    OVERLAY_LOG_INFO("Overlay pos set: x %d,y %d,w %d,h %d",x,y,w,h);

    //fetch the overlay obj lock
    pthread_mutex_lock(&obj->mDataShared->obj_lock);
    if((x!= obj->outX)||
       (y!= obj->outY)||
       (w!= (uint32_t)obj->outW)||
       (h!= (uint32_t)obj->outH)) {
        //Set out changed flag,so mixer thread will know
        //output area changed
        obj->out_changed = true;

        OVERLAY_LOG_INFO("Overlay pos set: x %d,y %d,w %d,h %d",x,y,w,h);

        obj->outX = x;
        obj->outY = y;
        obj->outW = w;
        obj->outH = h;

        //sem_post(&ctx->control_shared->overlay_sem);
    }

    //release the overlay obj lock
    pthread_mutex_unlock(&obj->mDataShared->obj_lock);

    return 0;
}

static int overlay_getPosition(struct overlay_control_device_t *dev,
         overlay_t* overlay, 
         int* x, int* y, uint32_t* w, uint32_t* h) {
    OVERLAY_LOG_FUNC;
    /* get this overlay's position */
    overlay_object *obj = static_cast<overlay_object *>(overlay);

    //fetch the overlay obj lock
    pthread_mutex_lock(&obj->mDataShared->obj_lock);
    *x = obj->outX;
    *y = obj->outY;
    *w = obj->outW;
    *y = obj->outH;
    //release the overlay obj lock
    pthread_mutex_unlock(&obj->mDataShared->obj_lock);

    return -EINVAL;
}

static int overlay_setParameter(struct overlay_control_device_t *dev,
         overlay_t* overlay, int param, int value) {
    OVERLAY_LOG_FUNC;
    overlay_object *obj = static_cast<overlay_object *>(overlay);
    int result = 0;
    /* set this overlay's parameter (talk to the h/w) */
    OVERLAY_LOG_INFO("overlay_setParameter param %d value %d",param,value);
    switch (param) {
        case OVERLAY_ROTATION_DEG:
            /* if only 90 rotations are supported, the call fails
             * for other values */
            OVERLAY_LOG_INFO("overlay_setParameter OVERLAY_ROTATION_DEG %d",value);
            //fetch the overlay obj lock
            pthread_mutex_lock(&obj->mDataShared->obj_lock);
            obj->rotation = value;
            //release the overlay obj lock
            pthread_mutex_unlock(&obj->mDataShared->obj_lock);

            break;
        case OVERLAY_DITHER: 
            break;
        case OVERLAY_TRANSFORM: 
            // see OVERLAY_TRANSFORM_*
            OVERLAY_LOG_INFO("overlay_setParameter OVERLAY_TRANSFORM %d",value);
            //fetch the overlay obj lock
            pthread_mutex_lock(&obj->mDataShared->obj_lock);
            obj->rotation = value;
            //release the overlay obj lock
            pthread_mutex_unlock(&obj->mDataShared->obj_lock);
            break;
        case OVERLAY_ZORDER: 
            // see OVERLAY_ZORDER*
            OVERLAY_LOG_INFO("overlay_setParameter OVERLAY_ZORDER %d",value);
            //fetch the overlay obj lock
            pthread_mutex_lock(&obj->mDataShared->obj_lock);
            obj->zorder = value;
            //release the overlay obj lock
            pthread_mutex_unlock(&obj->mDataShared->obj_lock);
            break;
         case OVERLAY_VISIBLE: 
            // see OVERLAY_VISIBLE*
            OVERLAY_LOG_INFO("overlay_setParameter OVERLAY_VISIBLE %d",value);
            //fetch the overlay obj lock
            pthread_mutex_lock(&obj->mDataShared->obj_lock);
            obj->visible = value;
            //release the overlay obj lock
            pthread_mutex_unlock(&obj->mDataShared->obj_lock);
            break;
        default:
            result = -EINVAL;
            break;
    }
    return result;
}

static int overlay_stage(struct overlay_control_device_t *dev,
                          overlay_t* overlay) {
    OVERLAY_LOG_FUNC;

    return 0;
}

static int overlay_commit(struct overlay_control_device_t *dev,
                          overlay_t* overlay) {
    OVERLAY_LOG_FUNC;
    return 0;
}


static int overlay_control_close(struct hw_device_t *dev) 
{
    OVERLAY_LOG_FUNC;
    struct overlay_control_context_t* ctx = (struct overlay_control_context_t*)dev;

    if (ctx) {
        /* free all resources associated with this device here
         * in particular the overlay_handle_t, outstanding overlay_t, etc...
         */

        ctx->overlay_running = false;
        sem_post(&ctx->control_shared->overlay_sem);//post the sem to unblock overlay thread

        if (ctx->overlay_thread != 0) {
            ctx->overlay_thread->requestExitAndWait();
        }
        
        //Destory all overlay instance here??, 
        //should let it done by user
        //v4l deinit should be called in overlay destory, but here is save also
        overlay_deinit_v4l(ctx);
        overlay_deinit_fbdev(ctx);

        //???delete this share file;
        destroy_control_shared_data(ctx->control_shared_fd,ctx->control_shared,true);

        pthread_mutex_destroy(&ctx->control_lock);

        free(ctx);
    }
    return 0;
}
 
// ****************************************************************************
// Data module
// ****************************************************************************

int overlay_data_initialize(struct overlay_data_device_t *dev,
        overlay_handle_t handle)
{
    OVERLAY_LOG_FUNC;
    struct handle_t *overlay_handle = (struct handle_t *)handle;
    struct overlay_data_context_t* ctx = (struct overlay_data_context_t*)dev;
    OVERLAY_LOG_INFO("overlay_data_initialize overlay_initialize overlay_handle 0x%p overlay_data_context_t 0x%p pid %d tid %d",
         overlay_handle,ctx,getpid(),gettid());
    /* 
     * overlay_handle_t should contain all the information to "inflate" this
     * overlay. Typically it'll have a file descriptor, informations about
     * how many buffers are there, etc...
     * It is also the place to mmap all buffers associated with this overlay
     * (see getBufferAddress).
     * 
     * NOTE: this function doesn't take ownership of overlay_handle_t
     * 
     */
    if(overlay_check_parameters(overlay_handle) < 0)
    {
        OVERLAY_LOG_ERR("Error!Invalid parameters in this overlay handle");
        return -EINVAL;
    }

    OVERLAY_LOG_INFO("num_buffer %d width %d,height %d,format %d, buf_size %d",
         overlay_handle->num_buffer,overlay_handle->width,overlay_handle->height,
         overlay_handle->format,overlay_handle->buf_size);

    ctx->control_shared_fd = overlay_handle->control_shared_fd;
    ctx->control_shared_size = overlay_handle->control_shared_size;
    ctx->data_shared_fd = overlay_handle->data_shared_fd;
    ctx->data_shared_size = overlay_handle->data_shared_size;
    ctx->width = overlay_handle->width;
    ctx->height = overlay_handle->height;
    ctx->format = overlay_handle->format;
    ctx->num_buffer = overlay_handle->num_buffer;//Number of buffers for overlay
    ctx->buf_size = overlay_handle->buf_size;
    //Open Control Share file
    if(open_control_shared_data(ctx) == -1)
    {
        OVERLAY_LOG_ERR("Error!Cannot open overlay control share file");
        return -1;
    }
    //Open Data Share file
    if(open_data_shared_data(ctx) == -1)
    {
        OVERLAY_LOG_ERR("Error!Cannot open overlay data share file");
        close_control_shared_data(ctx);
        return -1;
    }
    if(ctx->data_shared->num_buffer != overlay_handle->num_buffer) {
        OVERLAY_LOG_ERR("Warning!num_buffer %d in overlay_handle is not the same as %d in data_shared",
                        overlay_handle->num_buffer,ctx->data_shared->num_buffer);
    }

    int bufcount = overlay_handle->num_buffer;
    int bufsize = overlay_handle->buf_size;
    ctx->allocator = new PmemAllocator(bufcount,bufsize);
    if(!ctx->allocator) {
        OVERLAY_LOG_ERR("Error!Cannot create PmemAllocator");
        close_control_shared_data(ctx);
        close_data_shared_data(ctx);
        return -1;
    }

    //overlay_buffer should be reallocated if num_buffer changed
    ctx->overlay_buffer = (OVERLAY_BUFFER *)malloc(sizeof(OVERLAY_BUFFER)*bufcount);
    if(!ctx->overlay_buffer) {
       OVERLAY_LOG_ERR("Error!Cannot allocate overlay buffer hdr");
       close_control_shared_data(ctx);
       close_data_shared_data(ctx);
       delete ctx->allocator;
       return -1;
    }
    else{
       memset(ctx->overlay_buffer,0,sizeof(OVERLAY_BUFFER)*bufcount);
       OVERLAY_LOG_INFO("overlay_buffer 0x%p",ctx->overlay_buffer);
    }


    pthread_mutex_lock(&ctx->data_shared->obj_lock);

    ctx->data_shared->free_count = 0;
    ctx->data_shared->free_head = 0;
    ctx->data_shared->free_tail = 0;
    memset(ctx->data_shared->free_bufs, 0, sizeof(int)*MAX_OVERLAY_BUFFER_NUM);
    ctx->data_shared->queued_count = 0;
    ctx->data_shared->queued_head = 0;
    ctx->data_shared->queued_tail = 0;
    memset(ctx->data_shared->queued_bufs, 0, sizeof(int)*MAX_OVERLAY_BUFFER_NUM);

    //Allocate the buffer
    //Insert all allocated buffer into buffer freequeue
    OVERLAY_BUFFER *pOverlayBuf = ctx->overlay_buffer;
    for(int index =0;index < bufcount; index++) {
        int ret = -1;
        ret = ctx->allocator->allocate(pOverlayBuf, bufsize);
        if(ret<0) {
            OVERLAY_LOG_ERR("Error!Cannot allocate overlay buffer");
            pthread_mutex_unlock(&ctx->data_shared->obj_lock);
            close_control_shared_data(ctx);
            close_data_shared_data(ctx);
            delete ctx->allocator;
            return -1;

        }

        OVERLAY_LOG_INFO("buffer %d: vir 0x%p, phy 0x%x",index,pOverlayBuf->vir_addr,pOverlayBuf->phy_addr);
        ctx->data_shared->free_bufs[ctx->data_shared->free_tail] = pOverlayBuf->phy_addr;
        ctx->data_shared->free_count++;
        ctx->data_shared->free_tail++;
        pOverlayBuf++;
    }
    ctx->queue_threshold = OVERLAY_QUEUE_THRESHOLD;
    pthread_mutex_unlock(&ctx->data_shared->obj_lock);
    OVERLAY_LOG_INFO("Overlay init success for Id %d",ctx->data_shared->instance_id);
    
    return 0;
}

int overlay_data_dequeueBuffer(struct overlay_data_device_t *dev,
			  overlay_buffer_t* buf) 
{
    OVERLAY_LOG_FUNC;
    int ret = 0;
    struct overlay_data_context_t* ctx = (struct overlay_data_context_t*)dev;
    if(!dev||!buf||!ctx->data_shared) {
       OVERLAY_LOG_ERR("Error!Invalid parameters for dequeuBuffer");
       *buf = NULL;
       return -EINVAL;
    }

    overlay_data_shared_t  *data_shared = ctx->data_shared;
    if(data_shared->overlay_mode == OVERLAY_PUSH_MODE){
        OVERLAY_LOG_ERR("Error!Push mode not support dequeueBuffer API");
        *buf = NULL;
        return -EINVAL;
    }

    /* blocks until a buffer is available and return an opaque structure
     * representing this buffer.
     */
    //Maybe we should limited the loop count
    //to avoid block calling thread too much
    //in case error condition
    do{
        pthread_mutex_lock(&data_shared->obj_lock);

        //check whether is in destroying
        if(data_shared->in_destroy){
           OVERLAY_LOG_ERR("Error!Cannot dequeueBuffer when it is under destroying");
           *buf = NULL;
           pthread_mutex_unlock(&data_shared->obj_lock);
           return -EINVAL;
        }

        //Check the free buffer queue
        if(data_shared->free_count == 0)
        {
            OVERLAY_LOG_RUNTIME("Id %d: No buffer for dequeueBuffer,have to wait",data_shared->instance_id);
            //pthread_cond_wait will unlock mutex firstly, before calling thread
            //will be blocked 
            //pthread_cond_timedwait() may be more suitable here
            data_shared->wait_buf_flag = 1;
            //if buffer waiting flag be setted
            //wait buffer free semphore here
            //the semphore be post in mixer thread, if a buffer has 
            //been mixed, and the waint flag be setted
            //and waiting flag should be reset in mixer thread
            
            //post sempore to notify mixer thread, give mixer thread a chance to free a buffer
            if(ctx->control_shared) {
                sem_post(&ctx->control_shared->overlay_sem);
            }
            pthread_cond_wait(&data_shared->free_cond, &data_shared->obj_lock);
            if(data_shared->wait_buf_flag != 0) {
                OVERLAY_LOG_ERR("Error!Instance %d:Cannot wait a free buffer",data_shared->instance_id);
                ret = -EINVAL;
            }
        }

        if(data_shared->free_count > 0) 
        {
            //fetch the buffer handle
            //return the buffer handle
            unsigned int phy_addr = data_shared->free_bufs[data_shared->free_head];
            OVERLAY_LOG_RUNTIME("Id %d:Have free buffer %d,head %d, tail %d,phy 0x%x",
                                data_shared->instance_id,
                                data_shared->free_count,
                                data_shared->free_head,
                                data_shared->free_tail,
                                phy_addr);
            data_shared->free_bufs[data_shared->free_head] = 0;
            data_shared->free_head++;
            data_shared->free_head = data_shared->free_head%MAX_OVERLAY_BUFFER_NUM;
            data_shared->free_count--;

            OVERLAY_BUFFER *overlay_buf = NULL;
            for(int index = 0;index< ctx->num_buffer;index ++) {
                if(ctx->overlay_buffer[index].phy_addr == phy_addr) {
                    overlay_buf = &ctx->overlay_buffer[index];
                    OVERLAY_LOG_RUNTIME("Instance %d:Dequeued a overlay buffer %d: phy 0x%x, vir 0x%x",
                         data_shared->instance_id,
                         index,overlay_buf->phy_addr,overlay_buf->vir_addr);
                }
            }

            if(!overlay_buf||!overlay_buf->vir_addr) {
                OVERLAY_LOG_ERR("Error!Instance %d:Not a valid buffer",data_shared->instance_id);
                pthread_mutex_unlock(&data_shared->obj_lock);
                return -1;
            }
            
            *buf = (overlay_buffer_t)overlay_buf;

            pthread_mutex_unlock(&data_shared->obj_lock);
            return 0;
        }

        //If not free buffer in queue, try again
        pthread_mutex_unlock(&data_shared->obj_lock);
 
    }while (ret <0);
 
    return ret;
}

int overlay_data_queueBuffer(struct overlay_data_device_t *dev,
        overlay_buffer_t buffer)
{
    OVERLAY_LOG_FUNC;;
    struct overlay_data_context_t* ctx = (struct overlay_data_context_t*)dev;
    //Push mode, should be a mothod for user to flush all buffer in overlay hal
    if(!dev||!ctx->data_shared) {
       OVERLAY_LOG_ERR("Error!Invalid parameters for dequeuBuffer");
       return -EINVAL;
    }

    overlay_data_shared_t  *data_shared = ctx->data_shared;
    OVERLAY_BUFFER *overlay_buf = NULL;
    //Check whether it is a valid buffer
    //Further check maybe needed:
    //Cannot be queued twice(not a duplicated node in display buffer queue)
    if(data_shared->overlay_mode != OVERLAY_PUSH_MODE)
    {
        if(buffer == 0) {
            OVERLAY_LOG_ERR("Error!Invalid parameters for dequeuBuffer");
            return -EINVAL;
        }

        overlay_buf = (OVERLAY_BUFFER *)buffer;
        if((overlay_buf < ctx->overlay_buffer)||
           (overlay_buf >= (ctx->overlay_buffer+ctx->num_buffer))) {
           OVERLAY_LOG_ERR("Error!Id %d:Not a valid overlay buffer for queueBuffer",
                           data_shared->instance_id);
           return -EINVAL;
        }
    }


    pthread_mutex_lock(&data_shared->obj_lock);
    //check whether is in destroying
    if(data_shared->in_destroy){
       OVERLAY_LOG_ERR("Error!Cannot queueBuffer when it is under destroying");
       pthread_mutex_unlock(&data_shared->obj_lock);
       return -EINVAL;
    }

    if(data_shared->overlay_mode == OVERLAY_NORAML_MODE){
        if (data_shared->queued_count >= ctx->queue_threshold ||
                ((data_shared->queued_count == (ctx->queue_threshold-1))&&(data_shared->buf_mixing == true))) {
                //Wait a buffer be mixered
                data_shared->wait_buf_flag = 1;
                //post sempore to notify mixer thread, give mixer thread a chance to free a buffer
                if(ctx->control_shared) {
                    sem_post(&ctx->control_shared->overlay_sem);
                }
                pthread_cond_wait(&data_shared->free_cond, &data_shared->obj_lock);
                if(data_shared->wait_buf_flag != 0) {
                    OVERLAY_LOG_ERR("Error!Id %d:Queued overlay buffer is out of number buffers supported,so dropped",
                            data_shared->instance_id);
                    pthread_mutex_unlock(&data_shared->obj_lock);
                    return -EINVAL;

                }
                OVERLAY_LOG_RUNTIME("Id %d:Wait a free slot for queue buffer",data_shared->instance_id);
        }
    }
    else{
        if(data_shared->buf_mixing == true
            || (data_shared->buf_displayed[data_shared->queued_head] == 0 && data_shared->queued_count >= ctx->queue_threshold)){
            //Wait a buffer be mixered
            OVERLAY_LOG_RUNTIME("buf_mixing, wait...");
            data_shared->wait_buf_flag = 1;

            pthread_cond_wait(&data_shared->free_cond, &data_shared->obj_lock);
            if(data_shared->wait_buf_flag != 0) {
                OVERLAY_LOG_ERR("Error!Id %d:Queued overlay buffer is out of number buffers supported,so dropped",
                        data_shared->instance_id);
                pthread_mutex_unlock(&data_shared->obj_lock);
                return -EINVAL;

            }
        }
    }

    unsigned int phy_addr;
    if(data_shared->overlay_mode != OVERLAY_PUSH_MODE){
        phy_addr = overlay_buf->phy_addr;
    }
    else{
        phy_addr = (unsigned int)buffer;
        //Push mode, should be a mothod for user to flush all buffer in overlay hal
        if(buffer == 0) {
            OVERLAY_LOG_WARN("Flush overlay hal by push null buffer!, current queued_count %d", data_shared->queued_count);
            if(data_shared->queued_count > 0){
                data_shared->queued_bufs[data_shared->queued_head] = 0;
                data_shared->buf_displayed[data_shared->queued_head] = 0;
                data_shared->queued_head ++;
                data_shared->queued_head = data_shared->queued_head%MAX_OVERLAY_BUFFER_NUM;
                data_shared->queued_count --;
            }
            pthread_mutex_unlock(&data_shared->obj_lock);
            return 0;
        }
    }


    OVERLAY_LOG_RUNTIME("Id %d:Queue buffer 0x%x at %d,queued count %d",
                        data_shared->instance_id,
                        phy_addr,data_shared->queued_tail,data_shared->queued_count);

    //Check whether it is duplicated in the queue
    for(int i= 0;i < data_shared->queued_count;i++) {
        if(phy_addr == data_shared->queued_bufs[(data_shared->queued_head+i)%MAX_OVERLAY_BUFFER_NUM]) {
            OVERLAY_LOG_WARN("Warning!This buffer is duplicated at queue,will be ignored");
            pthread_mutex_unlock(&data_shared->obj_lock);
            return -EINVAL;
        }
    }

    data_shared->queued_bufs[data_shared->queued_tail] = phy_addr;
    data_shared->buf_displayed[data_shared->queued_tail] = 0;
    data_shared->queued_tail++;
    data_shared->queued_tail = data_shared->queued_tail%MAX_OVERLAY_BUFFER_NUM;
    data_shared->queued_count++;
    ctx->buf_queued++;
    
    if(data_shared->overlay_mode == OVERLAY_PUSH_MODE && data_shared->queued_count > ctx->queue_threshold){
        data_shared->queued_bufs[data_shared->queued_head] = 0;
        data_shared->buf_displayed[data_shared->queued_head] = 0;
        data_shared->queued_head ++;
        data_shared->queued_head = data_shared->queued_head%MAX_OVERLAY_BUFFER_NUM;
        data_shared->queued_count --;
    }
    
    pthread_mutex_unlock(&data_shared->obj_lock);

    //post sempore to notify mixer thread, new buffer is coming for mixing
    if(ctx->control_shared) {
        sem_post(&ctx->control_shared->overlay_sem);
    }

    return 0;
}

void *overlay_data_getBufferAddress(struct overlay_data_device_t *dev,
        overlay_buffer_t buffer)
{
    OVERLAY_LOG_FUNC;
    struct overlay_data_context_t* ctx = (struct overlay_data_context_t*)dev;
    if(!dev) {
       OVERLAY_LOG_ERR("Error!Invalid parameters for getBufferAddress");
       return NULL;
    }

    overlay_data_shared_t  *data_shared = ctx->data_shared;
    if(data_shared->overlay_mode == OVERLAY_PUSH_MODE){
        OVERLAY_LOG_ERR("Error!Push mode not support getBufferAddress API");
        return NULL;
    }

    /* this may fail (NULL) if this feature is not supported. In that case,
     * presumably, there is some other HAL module that can fill the buffer,
     * using a DSP for instance */

    //Check it is a valid buffer
    //return virtual addree of this buffer
    OVERLAY_BUFFER *overlay_buf = (OVERLAY_BUFFER *)buffer;
    //Check whether it is a valid buffer
    //Further check maybe needed:
    //Cannot be queued twice(not a duplicated node in display buffer queue)
    if((overlay_buf < ctx->overlay_buffer)||
       (overlay_buf >= (ctx->overlay_buffer+ctx->num_buffer))) {
       OVERLAY_LOG_ERR("Error!Id %d:Not a valid overlay buffer for queueBuffer",
                       ctx->data_shared->instance_id);
       return NULL;
    }

    return overlay_buf->vir_addr;


}

unsigned int overlay_data_getBufferAddressPhy(struct overlay_data_device_t *dev,
        overlay_buffer_t buffer)
{
    OVERLAY_LOG_FUNC;
    struct overlay_data_context_t* ctx = (struct overlay_data_context_t*)dev;
    if(!dev) {
       OVERLAY_LOG_ERR("Error!Invalid parameters for getBufferAddressPhy");
       return NULL;
    }

    overlay_data_shared_t  *data_shared = ctx->data_shared;
    if(data_shared->overlay_mode == OVERLAY_PUSH_MODE){
        OVERLAY_LOG_ERR("Error!Push mode not support getBufferAddress API");
        return 0;
    }

    /* this may fail (NULL) if this feature is not supported. In that case,
     * presumably, there is some other HAL module that can fill the buffer,
     * using a DSP for instance */

    //Check it is a valid buffer
    //return virtual addree of this buffer
    OVERLAY_BUFFER *overlay_buf = (OVERLAY_BUFFER *)buffer;
    //Check whether it is a valid buffer
    //Further check maybe needed:
    //Cannot be queued twice(not a duplicated node in display buffer queue)
    if((overlay_buf < ctx->overlay_buffer)||
       (overlay_buf >= (ctx->overlay_buffer+ctx->num_buffer))) {
       OVERLAY_LOG_ERR("Error!Id %d:Not a valid overlay buffer for queueBuffer",
                       ctx->data_shared->instance_id);
       return 0;
    }

    return overlay_buf->phy_addr;

}

int overlay_data_resizeInput(struct overlay_data_device_t *dev,
            uint32_t w, uint32_t h)
{
    OVERLAY_LOG_FUNC;
    struct overlay_data_context_t* ctx = (struct overlay_data_context_t*)dev;
    if((!dev)||(w=0)||(h=0)||
       (w> MAX_OVERLAY_INPUT_W)||
       (h> MAX_OVERLAY_INPUT_H)) {
       OVERLAY_LOG_ERR("Error!Invalid parameters for overlay_data_resizeInput");
       return -EINVAL;
    }
    OVERLAY_LOG_ERR("Error!overlay_data_resizeInput not supported");
    return -1;
#if 0
    overlay_data_shared_t  *data_shared = ctx->data_shared;
    pthread_mutex_lock(&data_shared->obj_lock);
    //Deallocate the overlay buffer
    //reallocate the overlay buffer
    //reinit the free/queue list
    pthread_mutex_unlock(&data_shared->obj_lock);
    return 0;
#endif
}

int overlay_data_setCrop(struct overlay_data_device_t *dev,
            uint32_t x, uint32_t y, uint32_t w, uint32_t h)
{
    OVERLAY_LOG_FUNC;
    struct overlay_data_context_t* ctx = (struct overlay_data_context_t*)dev;
    if((!dev)||
       ((x+w)> (uint32_t)ctx->width)||
       ((y+h)> (uint32_t)ctx->height)) {
       OVERLAY_LOG_ERR("Error!Invalid parameters for overlay_data_setCrop");
       return -EINVAL;
    }
    overlay_data_shared_t  *data_shared = ctx->data_shared;
    OVERLAY_LOG_INFO("Id %d:overlay_data_setCrop %d %d %d %d",
                     data_shared->instance_id,x,y,w,h);
    pthread_mutex_lock(&data_shared->obj_lock);
    //Set the  crop setting for the overlay instance
    data_shared->crop_x = x&0xFFFFFFF8;
    data_shared->crop_y = y&0xFFFFFFF8;
    data_shared->crop_w = w&0xFFFFFFF8;
    data_shared->crop_h = h&0xFFFFFFF8;
    pthread_mutex_unlock(&data_shared->obj_lock);
    return 0;
}

int overlay_data_getCrop(struct overlay_data_device_t *dev,
       uint32_t* x, uint32_t* y, uint32_t* w, uint32_t* h)
{
    OVERLAY_LOG_FUNC;
    struct overlay_data_context_t* ctx = (struct overlay_data_context_t*)dev;
    if((!dev)||(!x)||(!y)||(!w)||(!h)) {
       OVERLAY_LOG_ERR("Error!Invalid parameters for overlay_data_getCrop");
       return -EINVAL;
    }
    overlay_data_shared_t  *data_shared = ctx->data_shared;
    pthread_mutex_lock(&data_shared->obj_lock);
    //Get the  crop setting for the overlay instance
    *x = data_shared->crop_x;
    *y = data_shared->crop_y;
    *w = data_shared->crop_w;
    *h = data_shared->crop_h;
    pthread_mutex_unlock(&data_shared->obj_lock);
    return 0;
}

int overlay_data_setParameter(struct overlay_data_device_t *dev,
            int param, int value)
{
    OVERLAY_LOG_FUNC;
    struct overlay_data_context_t* ctx = (struct overlay_data_context_t*)dev;
    if((!dev)) {
       OVERLAY_LOG_ERR("Error!Invalid parameters for overlay_data_setParameter");
       return -EINVAL;
    }
    overlay_data_shared_t  *data_shared = ctx->data_shared;
    switch(param) {
        case OVERLAY_MODE:
            {
                OVERLAY_LOG_INFO("Id %d:overlay_data_setParameter push mode %d",
                                 data_shared->instance_id,value);
                pthread_mutex_lock(&data_shared->obj_lock);

                if(data_shared->overlay_mode == value) {
                    OVERLAY_LOG_INFO("No mode change needed");
                    pthread_mutex_unlock(&data_shared->obj_lock);
                    return 0;
                }
                //We only support the change from normal mode to push mode
                //Default setting of mode should be normal mode
                if(value != OVERLAY_PUSH_MODE) {
                    OVERLAY_LOG_ERR("Error!Mode change not supported");
                    pthread_mutex_unlock(&data_shared->obj_lock);
                    return 0;
                }               
                if(data_shared->free_count != data_shared->num_buffer) {
                    //The buffer may be used by mixer
                    //should hold until all buffer are in free queue
                    OVERLAY_LOG_ERR("Error!Id %d:There are still %d buffers been used",
                                    data_shared->instance_id,
                                    data_shared->num_buffer-data_shared->free_count);
                    pthread_mutex_unlock(&data_shared->obj_lock);
                    return -EINVAL;
                }
    
                //DeAllocate all buffers
                for(int i = 0; i< ctx->num_buffer;i++) {
                    ctx->allocator->deAllocate(&ctx->overlay_buffer[i]);
                }
    
                //DeAllocate all buffer hdr
                if(ctx->overlay_buffer) {
                    free(ctx->overlay_buffer);
                    ctx->overlay_buffer = NULL;
                }
    
                //Delete allocator
                if(ctx->allocator) {
                    delete ctx->allocator;
                    ctx->allocator = NULL;
                }
    
                data_shared->free_count = 0;
                data_shared->free_head = 0;
                data_shared->free_tail = 0;
                memset(data_shared->free_bufs, 0, sizeof(int)*MAX_OVERLAY_BUFFER_NUM);
                data_shared->queued_count = 0;
                data_shared->queued_head = 0;
                data_shared->queued_tail = 0;
                memset(data_shared->queued_bufs, 0, sizeof(int)*MAX_OVERLAY_BUFFER_NUM);
                memset(data_shared->buf_displayed, 0, sizeof(int)*MAX_OVERLAY_BUFFER_NUM);
                data_shared->overlay_mode = OVERLAY_PUSH_MODE;
                ctx->num_buffer = 0;
                data_shared->num_buffer = 0;
                //Set the threashold to 1 for push mode, to avoid the conflict buffer sharing
                //between vpu and ipu
                ctx->queue_threshold = 1;
                pthread_mutex_unlock(&data_shared->obj_lock);
            }
            break;
        case OVERLAY_BUFNUM:
            {
                OVERLAY_LOG_INFO("Id %d:overlay_data_setParameter buf num %d",
                                 data_shared->instance_id,value);
                if(MAX_OVERLAY_BUFFER_NUM < value) {
                    OVERLAY_LOG_ERR("Error!Id %d:Invalid vaule %d for OVERLAY_BUFNUM setting",
                                    data_shared->instance_id,value);
                    return -EINVAL;
                }
                pthread_mutex_lock(&data_shared->obj_lock);
                if(data_shared->free_count != data_shared->num_buffer) {
                    //The buffer may be used by mixer
                    //should hold until all buffer are in free queue
                    OVERLAY_LOG_ERR("Error!Id %d:There are still %d buffers been used",
                                    data_shared->instance_id,
                                    data_shared->num_buffer-data_shared->free_count);
                    pthread_mutex_unlock(&data_shared->obj_lock);
                    return -EINVAL;
                }
    
                //DeAllocate all buffers
                for(int i = 0; i< ctx->num_buffer;i++) {
                    ctx->allocator->deAllocate(&ctx->overlay_buffer[i]);
                }
    
                //DeAllocate all buffer hdr
                if(ctx->overlay_buffer) {
                    free(ctx->overlay_buffer);
                    ctx->overlay_buffer = NULL;
                }
    
                //Delete allocator
                if(ctx->allocator) {
                    delete ctx->allocator;
                    ctx->allocator = NULL;
                }
    
                data_shared->free_count = 0;
                data_shared->free_head = 0;
                data_shared->free_tail = 0;
                memset(data_shared->free_bufs, 0, sizeof(int)*MAX_OVERLAY_BUFFER_NUM);
                data_shared->queued_count = 0;
                data_shared->queued_head = 0;
                data_shared->queued_tail = 0;
                memset(data_shared->queued_bufs, 0, sizeof(int)*MAX_OVERLAY_BUFFER_NUM);
                ctx->num_buffer = value;
                data_shared->num_buffer = value;
    
                int bufcount = ctx->num_buffer;
                int bufsize = ctx->buf_size;
                ctx->allocator = new PmemAllocator(bufcount,bufsize);
                if(!ctx->allocator) {
                    OVERLAY_LOG_ERR("Error!Cannot create PmemAllocator");
                    return -1;
                }
            
                //overlay_buffer should be reallocated if num_buffer changed
                ctx->overlay_buffer = (OVERLAY_BUFFER *)malloc(sizeof(OVERLAY_BUFFER)*bufcount);
                if(!ctx->overlay_buffer) {
                   OVERLAY_LOG_ERR("Error!Cannot allocate overlay buffer hdr");
                   delete ctx->allocator;
                   ctx->allocator = NULL;
                   pthread_mutex_unlock(&data_shared->obj_lock);
                   return -1;
                }
                else{
                   memset(ctx->overlay_buffer,0,sizeof(OVERLAY_BUFFER)*bufcount);
                   OVERLAY_LOG_INFO("overlay_buffer 0x%p",ctx->overlay_buffer);
                }
                //Allocate the buffer
                //Insert all allocated buffer into buffer freequeue
                OVERLAY_BUFFER *pOverlayBuf = ctx->overlay_buffer;
                for(int index =0;index < bufcount; index++) {
                    int ret = -1;
                    ret = ctx->allocator->allocate(pOverlayBuf, bufsize);
                    if(ret<0) {
                        OVERLAY_LOG_ERR("Error!Cannot allocate overlay buffer");
                        pthread_mutex_unlock(&ctx->data_shared->obj_lock);
                        delete ctx->allocator;
                        return -1;
            
                    }
            
                    OVERLAY_LOG_INFO("buffer %d: vir 0x%p, phy 0x%x",index,pOverlayBuf->vir_addr,pOverlayBuf->phy_addr);
                    ctx->data_shared->free_bufs[ctx->data_shared->free_tail] = pOverlayBuf->phy_addr;
                    ctx->data_shared->free_count++;
                    ctx->data_shared->free_tail++;
                    pOverlayBuf++;
                }
                pthread_mutex_unlock(&data_shared->obj_lock);
            }

            break;
            case OVERLAY_FOR_PLAYBACK:
                data_shared->for_playback = (value != 0 ) ? true : false;
            break;
        default:
            break;
    }

    return 0;
}

int overlay_data_getBufferCount(struct overlay_data_device_t *dev)
{
    OVERLAY_LOG_FUNC;
    struct overlay_data_context_t* ctx = (struct overlay_data_context_t*)dev;
    if((!dev)) {
       OVERLAY_LOG_ERR("Error!Invalid parameters for overlay_data_getBufferCount");
       return 0;
    }
    return ctx->num_buffer;
}

static int overlay_data_close(struct hw_device_t *dev) 
{
    struct overlay_data_context_t* ctx = (struct overlay_data_context_t*)dev;
    overlay_data_shared_t  *data_shared;
    struct timespec timeout;
    struct timeval  tv;

    OVERLAY_LOG_INFO("overlay_data_close()");
    if (ctx) {
        /* free all resources associated with this device here
         * in particular all pending overlay_buffer_t if needed.
         * 
         * NOTE: overlay_handle_t passed in initialize() is NOT freed and
         * its file descriptors are not closed (this is the responsibility
         * of the caller).
         */
        OVERLAY_LOG_INFO("overlay_data_close ctx %p buf_queued %d",ctx,ctx->buf_queued);
        data_shared = ctx->data_shared;

        if(data_shared != NULL) {
            pthread_mutex_lock(&data_shared->obj_lock);
            data_shared->in_destroy = true;
            while(data_shared->queued_count > 0) {
                OVERLAY_LOG_WARN("Warning!data close Still %d buffer in queue",
                                data_shared->queued_count);
                //Wait a buffer be mixered
                data_shared->wait_buf_flag = 1;
                //post sempore to notify mixer thread, give mixer thread a chance to free a buffer
                if(ctx->control_shared) {
                    sem_post(&ctx->control_shared->overlay_sem);
                }
                gettimeofday(&tv, (struct timezone *) NULL);
                timeout.tv_sec = tv.tv_sec;
                timeout.tv_nsec = (tv.tv_usec + 200000) * 1000L;//200ms
                pthread_cond_timedwait(&data_shared->free_cond, &data_shared->obj_lock,&timeout);
                if(data_shared->wait_buf_flag != 0) {
                    OVERLAY_LOG_ERR("Error!cannot make a buffer flushed");
               }
            }

            if(data_shared->buf_mixing) {
                int wait_count = 0;
                LOGW("Current this overlay is in buf_mixing! Have to wait it done");
                pthread_mutex_unlock(&data_shared->obj_lock);
                //Make a sleep for 10ms
                do{
                    usleep(2000);
                    wait_count++;
                    if(wait_count > 5) {
                        OVERLAY_LOG_ERR("Error!Still cannot wait the buf mix done!");
                        break;
                    }
                }while (data_shared->buf_mixing);
            }
            else{
                pthread_mutex_unlock(&data_shared->obj_lock);
            }
        }

        //Close the share file for data and control
        close_data_shared_data(ctx);
        close_control_shared_data(ctx);

        //DeAllocate all buffers
        for(int i = 0; i< ctx->num_buffer;i++) {
            ctx->allocator->deAllocate(&ctx->overlay_buffer[i]);
        }
        
        //DeAllocate all buffer hdr
        if(ctx->overlay_buffer) {
            free(ctx->overlay_buffer);
        }

        //Delete allocator
        if(ctx->allocator) {
            delete ctx->allocator;
        }
        free(ctx);
        OVERLAY_LOG_INFO("overlay_data_close exit");
    }
    return 0;
}

/*****************************************************************************/

static int overlay_device_open(const struct hw_module_t* module, const char* name,
        struct hw_device_t** device)
{
    OVERLAY_LOG_FUNC;
    int status = -EINVAL;
    OVERLAY_LOG_RUNTIME("overlay_device_open %s pid %d,tid %d",name,getpid(),gettid());
    if (!strcmp(name, OVERLAY_HARDWARE_CONTROL)) {
        
        struct overlay_control_context_t *dev;
        char value[PROPERTY_VALUE_MAX];


        dev = (overlay_control_context_t*)malloc(sizeof(*dev));

        /* initialize our state here */
        memset(dev, 0, sizeof(*dev));

        /* initialize the procs */
        dev->device.common.tag = HARDWARE_DEVICE_TAG;
        dev->device.common.version = 0;
        dev->device.common.module = const_cast<hw_module_t*>(module);
        dev->device.common.close = overlay_control_close;
        
        dev->device.get = overlay_get;
        dev->device.createOverlay = overlay_createOverlay;
        dev->device.destroyOverlay = overlay_destroyOverlay;
        dev->device.setPosition = overlay_setPosition;
        dev->device.getPosition = overlay_getPosition;
        dev->device.setParameter = overlay_setParameter;
        dev->device.stage = overlay_stage;
        dev->device.commit = overlay_commit;

        *device = &dev->device.common;

        dev->outpixelformat = V4L2_PIX_FMT_UYVY;
        if(pthread_mutex_init(&(dev->control_lock), NULL)!=0 ){
            OVERLAY_LOG_ERR("Error!init control_lock failed");
            goto err_control_exit;
        }

        dev->default_fb_resolution.nWidth = dev->default_fb_resolution.nHeight = 0;
        
        //init fb dev as different config
        if(overlay_init_fbdev(dev, DEFAULT_FB_DEV_NAME)<0){
            OVERLAY_LOG_ERR("Error!init fbdev failed");
            goto err_control_exit;
        }

        //Only init v4l as needed
        //if(overlay_init_v4l(dev, DEFAULT_V4L_LAYER)<0){
        //    OVERLAY_LOG_ERR("Error!init v4l failed");
        //    goto err_control_exit;
        //}
        dev->control_shared_fd = create_control_shared_data(&dev->control_shared);
        dev->control_shared_size = dev->control_shared->size;
        if(dev->control_shared_fd < 0 || !dev->control_shared){
            OVERLAY_LOG_ERR("Error!init control share failed");
            goto err_control_exit;
        }

        dev->sec_disp_enable = false;
        dev->display_mode = DISPLAY_MODE_NORMAL;
        property_get("rw.DUAL_VIDEO_SIN_UI", value, "0");
        if (!strcmp(value, "1"))
            dev->video_play_mode = DUAL_VIDEO_SIN_UI;
        else
            dev->video_play_mode = SIN_VIDEO_DUAL_UI;
        dev->overlay_running = true;
        dev->overlay_thread = new OverlayThread(dev);
        dev->overlay_thread->run("OverlayThread", PRIORITY_URGENT_DISPLAY);
        status = 0;

        OVERLAY_LOG_INFO("Overlay HAL control device Created successfully");
        return status;

err_control_exit:
        OVERLAY_LOG_ERR("Error!init overlay_control_context_t failed");
        if(dev) {
            if(dev->overlay_thread) {
                delete dev->overlay_thread;
                dev->overlay_thread = NULL;
            }
            #ifdef OVERLAY_V4L_OUT
            for(int buf_index=0;buf_index < dev->v4l_bufcount;buf_index++) {
               if(dev->v4lbuf_addr[buf_index])
                  munmap((void *)dev->v4lbuf_addr[buf_index], dev->v4l_buffers[buf_index].length);
               memset(&dev->v4l_buffers[buf_index],0,sizeof(v4l2_buffer));
               dev->v4lbuf_addr[buf_index] = NULL;
            }

            dev->v4l_bufcount = 0;

            if(dev->v4l_id) {
                close(dev->v4l_id);
                dev->v4l_id = 0;
            }
            #endif
            if(dev->alpha_buffers[0].vir_addr){
                munmap((void *)dev->alpha_buffers[0].vir_addr, dev->alpha_buffers[0].size);
                memset(&dev->alpha_buffers[0],0,sizeof(OVERLAY_BUFFER));
            }
            if(dev->alpha_buffers[1].vir_addr){
                munmap((void *)dev->alpha_buffers[1].vir_addr, dev->alpha_buffers[1].size);
                memset(&dev->alpha_buffers[1],0,sizeof(OVERLAY_BUFFER));
            }

            if(dev->fb_dev) {
                close(dev->fb_dev);
                dev->fb_dev = 0;
            }

            free(dev);
        }
        status = -1;

    } else if (!strcmp(name, OVERLAY_HARDWARE_DATA)) {
        struct overlay_data_context_t *dev;
        dev = (overlay_data_context_t*)malloc(sizeof(*dev));

        /* initialize our state here */
        memset(dev, 0, sizeof(*dev));

        /* initialize the procs */
        dev->device.common.tag = HARDWARE_DEVICE_TAG;
        dev->device.common.version = 0;
        dev->device.common.module = const_cast<hw_module_t*>(module);
        dev->device.common.close = overlay_data_close;
        
        dev->device.initialize = overlay_data_initialize;
        dev->device.dequeueBuffer = overlay_data_dequeueBuffer;
        dev->device.queueBuffer = overlay_data_queueBuffer;
        dev->device.getBufferAddress = overlay_data_getBufferAddress;
        dev->device.getBufferAddressPhy = overlay_data_getBufferAddressPhy;
        dev->device.resizeInput = overlay_data_resizeInput;
        dev->device.setCrop = overlay_data_setCrop;
        dev->device.getCrop = overlay_data_getCrop;
        dev->device.setParameter = overlay_data_setParameter;
        dev->device.getBufferCount = overlay_data_getBufferCount;

        *device = &dev->device.common;
        status = 0;
    }
    return status;
}
