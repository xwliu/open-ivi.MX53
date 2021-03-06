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

/*
 * Copyright 2009-2011 Freescale Semiconductor, Inc. All Rights Reserved.
 */
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <linux/time.h>
#include <linux/videodev.h>
#include <linux/videodev2.h>
#include <linux/mxc_v4l2.h>
#include <linux/mxcfb.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <utils/threads.h>
#include <dirent.h>

#include "TractorV4l2CsiDevice.h"

namespace android{
    V4l2CsiDevice :: V4l2CsiDevice(){
        mSupportedFmt[0] = v4l2_fourcc('R', 'G', 'B', '3');
    }
    V4l2CsiDevice :: ~V4l2CsiDevice()
    {
    }


    CAPTURE_DEVICE_ERR_RET V4l2CsiDevice :: V4l2Open(){
        CAMERA_HAL_LOG_FUNC;
        int fd = 0, i, j, is_found = 0;
        const char *flags[] = {"uncompressed", "compressed"};

        char	dev_node[CAMAERA_FILENAME_LENGTH];
        DIR *v4l_dir = NULL;
        struct dirent *dir_entry;
        struct v4l2_dbg_chip_ident vid_chip;
        struct v4l2_fmtdesc vid_fmtdesc;
        struct v4l2_frmsizeenum vid_frmsize;
        CAPTURE_DEVICE_ERR_RET ret = CAPTURE_DEVICE_ERR_NONE;

        if(mCameraDevice > 0)
            return CAPTURE_DEVICE_ERR_ALRADY_OPENED;
        else if (mCaptureDeviceName[0] != '#'){
            CAMERA_HAL_LOG_RUNTIME("already get the device name %s", mCaptureDeviceName);
            mCameraDevice = open(mCaptureDeviceName, O_RDWR, O_NONBLOCK);
            if (mCameraDevice < 0)
                return CAPTURE_DEVICE_ERR_OPEN;
        }
        else{
            CAMERA_HAL_LOG_RUNTIME("deviceName is %s", mInitalDeviceName);
            v4l_dir = opendir("/sys/class/video4linux");
            if (v4l_dir){
                while((dir_entry = readdir(v4l_dir))) {
                    memset((void *)dev_node, 0, CAMAERA_FILENAME_LENGTH);
                    if(strncmp(dir_entry->d_name, "video", 5)) 
                        continue;
                    sprintf(dev_node, "/dev/%s", dir_entry->d_name);
                    if ((fd = open(dev_node, O_RDWR, O_NONBLOCK)) < 0)
                        continue;
                    CAMERA_HAL_LOG_RUNTIME("dev_node is %s", dev_node);
                    if(ioctl(fd, VIDIOC_DBG_G_CHIP_IDENT, &vid_chip) < 0 ) {
                        close(fd);
                        continue;
                    } else if (strstr(vid_chip.match.name, mInitalDeviceName) != 0) {
                        is_found = 1;
                        strcpy(mCaptureDeviceName, dev_node);
                        strcpy(mInitalDeviceName, vid_chip.match.name);
                        CAMERA_HAL_LOG_INFO("device name is %s", mCaptureDeviceName);
                        CAMERA_HAL_LOG_INFO("sensor name is %s", mInitalDeviceName);
                        break;
                    } else{
                        close(fd);
                        fd = 0;
                    }
                }
            }
            if (fd > 0)
                mCameraDevice = fd;
            else{
                CAMERA_HAL_ERR("The device name is not correct or the device is error");
                return CAPTURE_DEVICE_ERR_OPEN;
            }
        }
        return ret; 
    }

    CAPTURE_DEVICE_ERR_RET V4l2CsiDevice :: V4l2EnumFmt(void *retParam){
        CAMERA_HAL_LOG_FUNC;
        CAPTURE_DEVICE_ERR_RET ret = CAPTURE_DEVICE_ERR_NONE; 
        unsigned int *pParamVal = (unsigned int *)retParam;

        if (mFmtParamIdx < ENUM_SUPPORTED_FMT){
            CAMERA_HAL_LOG_RUNTIME("vid_fmtdesc.pixelformat is %x", mSupportedFmt[mFmtParamIdx]);
            *pParamVal = mSupportedFmt[mFmtParamIdx];
            mFmtParamIdx ++;
            ret = CAPTURE_DEVICE_ERR_ENUM_CONTINUE;
        }else{
            mFmtParamIdx = 0;
            ret = CAPTURE_DEVICE_ERR_GET_PARAM;
        }
        return ret;
    }

    CAPTURE_DEVICE_ERR_RET V4l2CsiDevice :: V4l2EnumSizeFps(void *retParam){
        CAMERA_HAL_LOG_FUNC;
        CAPTURE_DEVICE_ERR_RET ret = CAPTURE_DEVICE_ERR_NONE; 
        struct v4l2_frmsizeenum vid_frmsize;

        struct capture_config_t *pCapCfg =(struct capture_config_t *) retParam;
        memset(&vid_frmsize, 0, sizeof(struct v4l2_frmsizeenum));
        vid_frmsize.index = mSizeFPSParamIdx;
        CAMERA_HAL_LOG_RUNTIME("the query for size fps fmt is %x",pCapCfg->fmt);
        vid_frmsize.pixel_format = pCapCfg->fmt;
        if (ioctl(mCameraDevice, VIDIOC_ENUM_FRAMESIZES, &vid_frmsize) != 0){
            mSizeFPSParamIdx = 0;
            ret = CAPTURE_DEVICE_ERR_SET_PARAM;
        }else{
            CAMERA_HAL_LOG_RUNTIME("in %s the w %d, h %d", __FUNCTION__,vid_frmsize.discrete.width, vid_frmsize.discrete.height);
            // for tractor, the camera w/h is fixed and fps is also fix at 30fps
            pCapCfg->width  = vid_frmsize.discrete.width;
            pCapCfg->height = vid_frmsize.discrete.height;
            pCapCfg->tv.numerator = 1;
            pCapCfg->tv.denominator = 30;
            mSizeFPSParamIdx ++;
            ret = CAPTURE_DEVICE_ERR_ENUM_CONTINUE;
        }
        return ret;
    }

    CAPTURE_DEVICE_ERR_RET V4l2CsiDevice :: V4l2ConfigInput(struct capture_config_t *pCapcfg)
    {
        CAMERA_HAL_LOG_FUNC;
        int input = 1;
	 CAMERA_HAL_LOG_RUNTIME("tractor V4l2CsiDevice VIDIOC_S_INPUT");
        if (ioctl(mCameraDevice, VIDIOC_S_INPUT, &input) < 0) {
            CAMERA_HAL_ERR("set input failed");
            return CAPTURE_DEVICE_ERR_SYS_CALL;
        }
        return CAPTURE_DEVICE_ERR_NONE;
    }


    CAPTURE_DEVICE_ERR_RET V4l2CsiDevice :: V4l2SetConfig(struct capture_config_t *pCapcfg)
    {

        CAMERA_HAL_LOG_FUNC;
        if (mCameraDevice <= 0 || pCapcfg == NULL){
            return CAPTURE_DEVICE_ERR_BAD_PARAM;
        }

        CAPTURE_DEVICE_ERR_RET ret = CAPTURE_DEVICE_ERR_NONE;
        struct v4l2_format fmt;
        struct v4l2_control ctrl;
        struct v4l2_streamparm parm;

	 if(V4l2SetRot(pCapcfg) < 0)
            return CAPTURE_DEVICE_ERR_SYS_CALL;

	 V4l2SetSource(pCapcfg);

        V4l2ConfigInput(pCapcfg);

        parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        //hard code here to do a walk around.
        if(pCapcfg->tv.denominator != 30 ){
            pCapcfg->tv.numerator = 1;
            pCapcfg->tv.denominator = 30;
        }
        CAMERA_HAL_LOG_RUNTIME("the fps is %d", pCapcfg->tv.denominator);

        parm.parm.capture.timeperframe.numerator = pCapcfg->tv.numerator;
        parm.parm.capture.timeperframe.denominator = pCapcfg->tv.denominator;
        ret = V4l2GetCaptureMode(pCapcfg, &(parm.parm.capture.capturemode));
        if (ret != CAPTURE_DEVICE_ERR_NONE)
            return ret;

        if (ioctl(mCameraDevice, VIDIOC_S_PARM, &parm) < 0) {
            parm.parm.capture.timeperframe.numerator = 1;
            parm.parm.capture.timeperframe.denominator = 30;
            if (ioctl(mCameraDevice, VIDIOC_S_PARM, &parm) < 0){
                CAMERA_HAL_ERR("%s:%d  VIDIOC_S_PARM failed\n", __FUNCTION__,__LINE__);
                CAMERA_HAL_ERR("frame timeval is numerator %d, denominator %d",parm.parm.capture.timeperframe.numerator, 
                        parm.parm.capture.timeperframe.denominator);
                return CAPTURE_DEVICE_ERR_SYS_CALL;
            }
        }


        fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        fmt.fmt.pix.pixelformat = pCapcfg->fmt;

        fmt.fmt.pix.width = pCapcfg->width&0xFFFFFFF8;
        fmt.fmt.pix.height = pCapcfg->height&0xFFFFFFF8;
        fmt.fmt.pix.bytesperline = fmt.fmt.pix.width * 2;
        fmt.fmt.pix.priv = 0;
        fmt.fmt.pix.sizeimage = 0;

        if (ioctl(mCameraDevice, VIDIOC_S_FMT, &fmt) < 0) {
            CAMERA_HAL_ERR("set format failed\n");
            CAMERA_HAL_ERR("pCapcfg->width is %d, pCapcfg->height is %d", pCapcfg->width, pCapcfg->height);
            CAMERA_HAL_ERR(" Set the Format :%c%c%c%c\n",
                    pCapcfg->fmt & 0xFF, (pCapcfg->fmt >> 8) & 0xFF,
                    (pCapcfg->fmt >> 16) & 0xFF, (pCapcfg->fmt >> 24) & 0xFF);
            return CAPTURE_DEVICE_ERR_SYS_CALL;
        }


        fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        if (ioctl(mCameraDevice, VIDIOC_G_FMT, &parm) < 0) {
            CAMERA_HAL_ERR("VIDIOC_S_PARM failed\n");
            return CAPTURE_DEVICE_ERR_SYS_CALL;
        }else{

            CAMERA_HAL_LOG_RUNTIME(" Width = %d\n", fmt.fmt.pix.width);
            CAMERA_HAL_LOG_RUNTIME(" Height = %d \n", fmt.fmt.pix.height);
            CAMERA_HAL_LOG_RUNTIME(" Image size = %d\n", fmt.fmt.pix.sizeimage);
            CAMERA_HAL_LOG_RUNTIME(" pixelformat = %x\n", fmt.fmt.pix.pixelformat);
        }
        pCapcfg->framesize = fmt.fmt.pix.sizeimage;

        return CAPTURE_DEVICE_ERR_NONE;
    }

    CAPTURE_DEVICE_ERR_RET V4l2CsiDevice :: V4l2GetCaptureMode(struct capture_config_t *pCapcfg, unsigned int *pMode){
        CAMERA_HAL_LOG_FUNC;

        if (mCameraDevice <= 0 || pCapcfg == NULL){
            return CAPTURE_DEVICE_ERR_BAD_PARAM;
        }

        CAMERA_HAL_LOG_INFO("the mode is %d", 0);
        *pMode = 0;
        pCapcfg->picture_waite_number = 0;

        return CAPTURE_DEVICE_ERR_NONE;
    }

    CAPTURE_DEVICE_ERR_RET V4l2CsiDevice :: V4l2SetSource(struct capture_config_t *pCapcfg)
    {
        CAMERA_HAL_LOG_FUNC;
        CAPTURE_DEVICE_ERR_RET ret = CAPTURE_DEVICE_ERR_NONE;
        struct v4l2_control ctrl;		
        ctrl.id = V4L2_CID_CAMERA_SOURCE;
        ctrl.value = pCapcfg->input_source;
        CAMERA_HAL_ERR("V4l2SetSource %d\n", ctrl.value);
        if (ioctl(mCameraDevice, VIDIOC_S_CTRL, &ctrl) < 0) {
            CAMERA_HAL_ERR("set ctrl failed\n");
            return CAPTURE_DEVICE_ERR_SYS_CALL;
        }
        return ret;
    }

    CAPTURE_DEVICE_ERR_RET V4l2CsiDevice :: V4l2GetInputValid(struct capture_config_t *pCapcfg, int* result)
    {
        CAMERA_HAL_LOG_FUNC;
        CAPTURE_DEVICE_ERR_RET ret = CAPTURE_DEVICE_ERR_NONE;
        struct v4l2_control ctrl;		
        ctrl.id = V4L2_CID_INPUT_VALID;
        ctrl.value = 0;

        if (ioctl(mCameraDevice, VIDIOC_G_CTRL, &ctrl) < 0) {
            CAMERA_HAL_ERR("set ctrl failed\n");
            return CAPTURE_DEVICE_ERR_SYS_CALL;
        }

        *result = ctrl.value;
        return ret;
    }

};

