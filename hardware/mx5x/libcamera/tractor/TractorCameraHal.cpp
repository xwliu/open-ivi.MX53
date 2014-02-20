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
 * Copyright 2011-2012 TokenWireless Comm Co,Ltd  Inc. All Rights Reserved.
 */


#include <cutils/properties.h>
#include "TractorCameraHal.h"
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include <linux/videodev2.h>
#include <hardware_legacy/power.h>
#include <linux/mxc_v4l2.h>
#include <time.h>

extern "C" {
#include "mxc_ipu_hl_lib.h" 
} 


namespace android {

    CameraHal::CameraHal()
        : mParameters(),
        mCallbackCookie(NULL),
        mNotifyCb(NULL),
        mDataCb(NULL),
        mDataCbTimestamp(NULL),
        mCaptureFrameThread(NULL),
        mPreviewShowFrameThread(NULL),
        mLock(),
        supportedPictureSizes(NULL),
        supportedPreviewSizes(NULL),
        supportedFPS(NULL),
        supprotedThumbnailSizes(NULL),
        mOverlay(NULL),
        mMsgEnabled(0),
        mPreviewHeap(0),
        mVideoBufNume(VIDEO_OUTPUT_BUFFER_NUM),
        mPreviewRunning(0),
        mPreviewFormat(V4L2_PIX_FMT_RGB24), 
        mPreviewFrameSize(0),
        mTakePicFlag(false),
        mCaptureFrameSize(0),
        mCaptureBufNum(0),
        mRecordRunning(0),
        nCameraBuffersQueued(0),
        mPreviewHeapBufNum(PREVIEW_HEAP_BUF_NUM),
        mCameraReady(false),
        mCaptureDeviceOpen(false),
        mPowerLock(false),
        mPreviewRotate(CAMERA_PREVIEW_BACK_REF)
    {
        CAMERA_HAL_LOG_FUNC;
        preInit();
    }

    CameraHal :: ~CameraHal()
    {
        CAMERA_HAL_LOG_FUNC;
        CameraMiscDeInit();
        CloseCaptureDevice();
        FreeInterBuf();
        postDestroy();
    }

    void CameraHal :: release()
    {
        CAMERA_HAL_LOG_FUNC;
        Mutex::Autolock lock(mLock);

        mCameraReady = false;
        CameraHALStopPreview();
		CloseCaptureDevice();
        UnLockWakeLock();
        return;
    }

    void CameraHal :: preInit()
    {
        CAMERA_HAL_LOG_FUNC;

    }
    void CameraHal :: postDestroy()
    {
        CAMERA_HAL_LOG_FUNC;
    }

    CAMERA_HAL_ERR_RET CameraHal :: setCaptureDevice(sp<CaptureDeviceInterface> capturedevice)
    {
        CAMERA_HAL_LOG_FUNC;
        CAMERA_HAL_ERR_RET ret = CAMERA_HAL_ERR_NONE;
        if (mCameraReady == false)
            mCaptureDevice = capturedevice;
        else
            ret = CAMERA_HAL_ERR_BAD_ALREADY_RUN;
        return ret;
    }

    CAMERA_HAL_ERR_RET CameraHal::Init()
    {
        CAMERA_HAL_LOG_FUNC;
        CAMERA_HAL_ERR_RET ret = CAMERA_HAL_ERR_NONE;
        mCameraReady == true;

        if ((ret = AolLocForInterBuf())<0)
            return ret;
        if ((ret = InitCameraHalParam()) < 0)
            return ret;
        if ((ret = CameraMiscInit()) < 0)
            return ret;

        return ret;
    }
    void  CameraHal::setPreviewRotate(CAMERA_PREVIEW_ROTATE previewRotate)
    {
        CAMERA_HAL_LOG_FUNC;
        return ;
    }

    CAMERA_HAL_ERR_RET  CameraHal :: AolLocForInterBuf()
    {
        CAMERA_HAL_LOG_FUNC;
        CAMERA_HAL_ERR_RET ret = CAMERA_HAL_ERR_NONE;

        supportedPictureSizes = (char *)malloc(CAMER_PARAM_BUFFER_SIZE);
        supportedPreviewSizes = (char *)malloc(CAMER_PARAM_BUFFER_SIZE);
        supportedFPS          = (char *)malloc(CAMER_PARAM_BUFFER_SIZE);
        supprotedThumbnailSizes = (char *)malloc(CAMER_PARAM_BUFFER_SIZE);

        if (supportedPictureSizes == NULL ||
                supportedPreviewSizes == NULL ||
                supportedFPS          == NULL ||
                supprotedThumbnailSizes == NULL)
            ret = CAMERA_HAL_ERR_ALLOC_BUF;

        return ret;
    }
    void  CameraHal :: FreeInterBuf()
    {
        CAMERA_HAL_LOG_FUNC;
        if (supportedPictureSizes)
            free(supportedPictureSizes);
        if (supportedPreviewSizes)
            free(supportedPreviewSizes);
        if (supportedFPS)
            free(supportedFPS);
        if (supprotedThumbnailSizes)
            free(supprotedThumbnailSizes);
    }

    CAMERA_HAL_ERR_RET CameraHal :: InitCameraHalParam()
    {	
        CAMERA_HAL_LOG_FUNC;
        CAMERA_HAL_ERR_RET ret = CAMERA_HAL_ERR_NONE;

        if ((ret = GetCameraBaseParam(&mParameters)) < 0)
            return ret;

        return ret;
    }

    CAMERA_HAL_ERR_RET CameraHal::CameraMiscInit()
    {
        CAMERA_HAL_LOG_FUNC;
        CAMERA_HAL_ERR_RET ret = CAMERA_HAL_ERR_NONE;
        pthread_mutex_init(&mOverlayMutex, NULL);
        return ret;
    }
    CAMERA_HAL_ERR_RET CameraHal::CameraMiscDeInit()
    {
        CAMERA_HAL_LOG_FUNC;
        CAMERA_HAL_ERR_RET ret = CAMERA_HAL_ERR_NONE;
        pthread_mutex_destroy(&mOverlayMutex);
        return ret;
    }

    CAMERA_HAL_ERR_RET CameraHal :: GetCameraBaseParam(CameraParameters *pParam)
    {
        CAMERA_HAL_LOG_FUNC;
        char TmpStr[20];
        unsigned int CapPreviewFmt[MAX_QUERY_FMT_TIMES];
        struct capture_config_t CaptureSizeFps;
        int  previewCnt= 0, pictureCnt = 0, i;

        pParam->setPreviewFormat(CameraParameters::PIXEL_FORMAT_RGB888);
        pParam->set(CameraParameters::KEY_VIDEO_FRAME_FORMAT, CameraParameters::PIXEL_FORMAT_RGB888);
        pParam->set(CameraParameters::KEY_SUPPORTED_PREVIEW_FORMATS, CameraParameters::PIXEL_FORMAT_RGB888);

        //the Camera Open here will not be close immediately, for later preview.
        if (OpenCaptureDevice() < 0)
            return CAMERA_HAL_ERR_OPEN_CAPTURE_DEVICE;

        memset(mCaptureSupportedFormat, 0, sizeof(unsigned int)*MAX_QUERY_FMT_TIMES);

        for(i =0; i< MAX_QUERY_FMT_TIMES; i ++){
            if (mCaptureDevice->EnumDevParam(OUTPU_FMT,&(mCaptureSupportedFormat[i])) < 0)
                break;
        }
        if (i == 0)
            return CAMERA_HAL_ERR_GET_PARAM;

        if (NegotiateCaptureFmt(false) < 0)
            return CAMERA_HAL_ERR_GET_PARAM;

        CaptureSizeFps.fmt = mPreviewCapturedFormat;

        CAMERA_HAL_LOG_INFO("mPreviewCapturedFormat is %x", mPreviewCapturedFormat);

        for(;;){
            if (mCaptureDevice->EnumDevParam(FRAME_SIZE_FPS,&CaptureSizeFps) <0){
                CAMERA_HAL_LOG_RUNTIME("get the frame size and time interval error");
                break;
            }
            sprintf(TmpStr, "%dx%d", CaptureSizeFps.width,CaptureSizeFps.height);
            CAMERA_HAL_LOG_INFO("the size is %s , the framerate is %d ", TmpStr, (CaptureSizeFps.tv.denominator/CaptureSizeFps.tv.numerator));
            if (previewCnt == 0)
                strncpy((char*) supportedPictureSizes, TmpStr, CAMER_PARAM_BUFFER_SIZE);
            else{
                strncat(supportedPictureSizes,  PARAMS_DELIMITER, CAMER_PARAM_BUFFER_SIZE);
                strncat(supportedPictureSizes, TmpStr, CAMER_PARAM_BUFFER_SIZE);
            }
            pictureCnt ++;

            if (CaptureSizeFps.tv.denominator/CaptureSizeFps.tv.numerator > 25){
                if (previewCnt == 0)
                    strncpy((char*) supportedPreviewSizes, TmpStr, CAMER_PARAM_BUFFER_SIZE);
                else{
                    strncat(supportedPreviewSizes,  PARAMS_DELIMITER, CAMER_PARAM_BUFFER_SIZE);
                    strncat(supportedPreviewSizes, TmpStr, CAMER_PARAM_BUFFER_SIZE);
                }
                previewCnt ++;
            }
        }

        /*hard code here*/
        strcpy(supportedFPS, "30");
        CAMERA_HAL_LOG_INFO("##The supportedPictureSizes is %s##", supportedPictureSizes);
        CAMERA_HAL_LOG_INFO("##the supportedPreviewSizes is %s##", supportedPreviewSizes);
        CAMERA_HAL_LOG_INFO("##the supportedFPS is %s##", supportedFPS);

        pParam->set(CameraParameters::KEY_SUPPORTED_PICTURE_SIZES, supportedPictureSizes);
        pParam->set(CameraParameters::KEY_SUPPORTED_PREVIEW_SIZES, supportedPreviewSizes);
        pParam->set(CameraParameters::KEY_SUPPORTED_PREVIEW_FRAME_RATES, supportedFPS);
        pParam->set(CameraParameters::KEY_SUPPORTED_PREVIEW_FPS_RANGE, "(1000,15000),(5000,30000)");
        pParam->set(CameraParameters::KEY_PREVIEW_FPS_RANGE, "5000,30000");
	 int result = 0;
	 mCaptureDevice->DevGetInputValid(&result);
	 pParam->set(CameraParameters::TW8832_INPUT_VALID, ((result & 0x80) >> 7)); 
	 pParam->set(CameraParameters::TW8832_INPUT_SOURCE, (result & 0x7)); 
        pParam->setPreviewSize(800, 480);
        pParam->setPictureSize(800, 480);
        pParam->setPreviewFrameRate(30);

        return CAMERA_HAL_ERR_NONE;

    }

    status_t CameraHal :: OpenCaptureDevice()
    {
        CAMERA_HAL_LOG_FUNC;
        status_t ret = NO_ERROR;
        if (mCaptureDeviceOpen){
            CAMERA_HAL_LOG_INFO("The capture device already open");
            return NO_ERROR;
        }
        else if (mCaptureDevice != NULL){
            if ( mCaptureDevice->DevOpen()<0 )
                return INVALID_OPERATION;
            mCaptureDeviceOpen = true;
        }else{
            CAMERA_HAL_ERR("no capture device assigned");
            return INVALID_OPERATION;
        }
        return ret;
    }
    void CameraHal ::CloseCaptureDevice()
    {
        CAMERA_HAL_LOG_FUNC;
        if (mCaptureDeviceOpen && mCaptureDevice != NULL){
            mCaptureDevice->DevClose();
            mCaptureDeviceOpen = false;
        }
    }

    sp<IMemoryHeap> CameraHal::getPreviewHeap() const
    {
        CAMERA_HAL_LOG_FUNC;

        return mPreviewHeap;
    }

    sp<IMemoryHeap> CameraHal::getRawHeap() const
    {
        return NULL;
    }

    status_t CameraHal::dump(int fd, const Vector<String16>& args) const
    {
        return NO_ERROR;
    }

    status_t CameraHal::sendCommand(int32_t command, int32_t arg1,
            int32_t arg2)
    {
        return BAD_VALUE;
    }

    void CameraHal::setCallbacks(notify_callback notify_cb,
            data_callback data_cb,
            data_callback_timestamp data_cb_timestamp,
            void* user)
    {
        Mutex::Autolock lock(mLock);
        mNotifyCb = notify_cb;
        mDataCb = data_cb;
        mDataCbTimestamp = data_cb_timestamp;
        mCallbackCookie = user;
    }

    void CameraHal::enableMsgType(int32_t msgType)
    {
        Mutex::Autolock lock(mLock);
        CAMERA_HAL_LOG_INFO("###the mesg enabled is %x###", msgType);
        mMsgEnabled |= msgType;
    }

    void CameraHal::disableMsgType(int32_t msgType)
    {
        Mutex::Autolock lock(mLock);
        CAMERA_HAL_LOG_INFO("###the mesg disabled is 0x%x###", msgType);
        mMsgEnabled &= ~msgType;
    }
    bool CameraHal::msgTypeEnabled(int32_t msgType)
    {
        Mutex::Autolock lock(mLock);
        CAMERA_HAL_LOG_INFO("###the mesg check is %x###", msgType);
        return (mMsgEnabled & msgType);
    }

    CameraParameters CameraHal::getParameters() const
    {
        CAMERA_HAL_LOG_FUNC;
        int result = 0;
        CameraParameters * pParam = (CameraParameters *)&mParameters;
        Mutex::Autolock lock(mLock);
	 mCaptureDevice->DevGetInputValid(&result);
	 pParam->set(CameraParameters::TW8832_INPUT_VALID, ((result & 0x80) >> 7)); 
	 pParam->set(CameraParameters::TW8832_INPUT_SOURCE, (result & 0x7)); 
		
        return mParameters;
    }

    status_t  CameraHal:: setParameters(const CameraParameters& params)
    {
        CAMERA_HAL_LOG_FUNC;
        int w, h;
        int framerate;
        int max_zoom,zoom, max_fps, min_fps;
        char tmp[128];
        Mutex::Autolock lock(mLock);

        mCaptureDeviceCfg.input_source = (tw8832_source)params.getInt(CameraParameters::TW8832_INPUT_SOURCE);
        CAMERA_HAL_ERR("@###tw8832 format is %d###@", mCaptureDeviceCfg.input_source);
        mParameters = params;
        mCaptureDevice->DevSetSource(&mCaptureDeviceCfg);
        return NO_ERROR;
    }

    status_t CameraHal::setOverlay(const sp<Overlay> &overlay)
    {
        CAMERA_HAL_LOG_FUNC;

        Mutex::Autolock lock(mLock);
        if (overlay == NULL){
            CAMERA_HAL_LOG_INFO("Trying to set overlay, but overlay is null!");
        }
        else{
            CAMERA_HAL_LOG_INFO("Get the overlay to display");
            overlay->setParameter(OVERLAY_MODE, OVERLAY_PUSH_MODE);
        }
        pthread_mutex_lock(&mOverlayMutex);
        mOverlay = overlay;
        pthread_mutex_unlock(&mOverlayMutex);
        return NO_ERROR;
    }

    status_t CameraHal::startPreview()
    {
        CAMERA_HAL_LOG_FUNC;
        status_t ret = NO_ERROR;

        Mutex::Autolock lock(mLock);
        if (mPreviewRunning) {
            return NO_ERROR;
        }
        if ((ret == CameraHALStartPreview())<0)
            return ret;

        LockWakeLock();
        return ret;
    }

    void CameraHal::stopPreview()
    {
        CAMERA_HAL_LOG_FUNC;
        struct timeval af_time, be_time;
        CAMERA_HAL_LOG_INFO("+CameraHal::stopPreview");
        Mutex::Autolock lock(mLock);
        /* Cannot stop preview in recording */
        //   if(mMsgEnabled & CAMERA_MSG_VIDEO_FRAME)
        //       return;
        CameraHALStopPreview();
        UnLockWakeLock();
        CAMERA_HAL_LOG_INFO("-CameraHal::stopPreview");
    }

    bool CameraHal::previewEnabled()
    {
        CAMERA_HAL_LOG_FUNC;
        return mPreviewRunning;
    }

    status_t CameraHal::startRecording()
    {
        CAMERA_HAL_LOG_FUNC;
        return NO_ERROR;
    }

    void CameraHal::stopRecording()
    {
        CAMERA_HAL_LOG_FUNC;
        mRecordRunning = false;
    }

    void CameraHal::releaseRecordingFrame(const sp<IMemory>& mem)
    {   }

    bool CameraHal::recordingEnabled()
    {
        CAMERA_HAL_LOG_FUNC;
        return (mPreviewRunning && mRecordRunning);
    }

    status_t CameraHal::autoFocus()
    {
        CAMERA_HAL_LOG_FUNC;

        Mutex::Autolock lock(mLock);
        return UNKNOWN_ERROR;
    }

    status_t CameraHal::cancelAutoFocus()
    {
        CAMERA_HAL_LOG_FUNC;
        return UNKNOWN_ERROR;
    }

    status_t CameraHal::takePicture()
    {
        CAMERA_HAL_LOG_FUNC;
        Mutex::Autolock lock(mLock);
        return UNKNOWN_ERROR;
    }

    status_t CameraHal::cancelPicture()
    {
        CAMERA_HAL_LOG_FUNC;
        return NO_ERROR;
    }

    int CameraHal :: cameraHALTakePicture()
    {
        CAMERA_HAL_LOG_FUNC;
        int ret = NO_ERROR;
        return ret;

    }

    int CameraHal :: NegotiateCaptureFmt(bool TakePicFlag)
    {
        CAMERA_HAL_LOG_FUNC;
        int ret = NO_ERROR, i = 0, j = 0;
	 if(TakePicFlag){
	 	return BAD_VALUE;
	 }

        CAMERA_HAL_LOG_INFO("mPreviewFormat :%c%c%c%c\n",
        mPreviewFormat & 0xFF, (mPreviewFormat >> 8) & 0xFF,
        (mPreviewFormat >> 16) & 0xFF, (mPreviewFormat >> 24) & 0xFF);

        for(i =0; i< MAX_QUERY_FMT_TIMES; i ++){
        	CAMERA_HAL_LOG_RUNTIME("mCaptureSupportedFormat[%d] is %x", i, mCaptureSupportedFormat[i]);
              if (mCaptureSupportedFormat[i] == mPreviewFormat){
              	CAMERA_HAL_LOG_RUNTIME("get the correct format [%d] is %x", i, mCaptureSupportedFormat[i]);
			mPreviewCapturedFormat = mPreviewFormat;
			break;
                }
	}
       CAMERA_HAL_LOG_INFO("mPreviewCapturedFormat :%c%c%c%c\n",
       mPreviewCapturedFormat & 0xFF, (mPreviewCapturedFormat >> 8) & 0xFF,
       (mPreviewCapturedFormat >> 16) & 0xFF, (mPreviewCapturedFormat >> 24) & 0xFF);

       if ((i == MAX_QUERY_FMT_TIMES)){
	       CAMERA_HAL_ERR("Negotiate for the preview format error");
       	return BAD_VALUE;
       }
       return ret;
    }

    status_t CameraHal::CameraHALStartPreview()
    {
        CAMERA_HAL_LOG_FUNC;
        status_t ret = NO_ERROR;
        int  max_fps, min_fps;
        mParameters.getPreviewSize((int *)&(mCaptureDeviceCfg.width),(int *)&(mCaptureDeviceCfg.height));
        mCaptureDeviceCfg.fmt = mPreviewCapturedFormat;
        mCaptureDeviceCfg.rotate = (SENSOR_PREVIEW_ROTATE)mPreviewRotate;
        mCaptureDeviceCfg.tv.numerator = 1;
        mCaptureDevice->GetDevName(mCameraSensorName);
        //according to google's doc getPreviewFrameRate & getPreviewFpsRange should support both.
        // so here just a walkaround, if the app set the frameRate, will follow this frame rate.
        if (mParameters.getPreviewFrameRate() >= 15)
            mCaptureDeviceCfg.tv.denominator = mParameters.getPreviewFrameRate();
        else{
            mParameters.getPreviewFpsRange(&min_fps, &max_fps);
            CAMERA_HAL_LOG_INFO("###start the capture the fps is %d###", max_fps);
            mCaptureDeviceCfg.tv.denominator = max_fps/1000;
        }
        mCaptureBufNum = PREVIEW_CAPTURE_BUFFER_NUM;
        mTakePicFlag = false;

        if ((ret = PrepareCaptureDevices()) < 0){
            CAMERA_HAL_ERR("PrepareCaptureDevices error ");
            return ret;
        }
        if ((ret = PreparePreviwBuf()) < 0){
            CAMERA_HAL_ERR("PreparePreviwBuf error");
            return ret;
        }

        if ((ret = PreparePreviwMisc()) < 0){
            CAMERA_HAL_ERR("PreparePreviwMisc error");
            return ret;
        }

        if ((ret = CameraHALPreviewStart()) < 0){
            CAMERA_HAL_ERR("CameraHALPreviewStart error");
            return ret;
        }
        return ret;
    }
    void CameraHal::CameraHALStopPreview()
    {
        CAMERA_HAL_LOG_FUNC;
        if (mPreviewRunning != 0)	{
            CameraHALStopThreads();
            CAMERA_HAL_LOG_INFO("CameraHal call Stop Misc");            
            CameraHALStopMisc();
            CAMERA_HAL_LOG_INFO("camera hal stop preview done");
        }else{
            CAMERA_HAL_LOG_INFO("Camera hal already stop preview");
        }
        return ;
    }

    void CameraHal :: CameraHALStopThreads()
    {
        CAMERA_HAL_LOG_FUNC;
        mPreviewRunning = 0;
        CAMERA_HAL_LOG_INFO("+CameraHal::CameraHALStopThreads Wait Capture Thread");
        
        if (mCaptureFrameThread!= 0){
            mCaptureFrameThread->requestExitAndWait();
            mCaptureFrameThread.clear();
        }
        CAMERA_HAL_LOG_INFO("+CameraHal::CameraHALStopThreads Wait PreviewShow Thread");        
        if (mPreviewShowFrameThread!= 0){
            mPreviewShowFrameThread->requestExitAndWait();
            mPreviewShowFrameThread.clear();
        }
        return ;
    }

    void CameraHal :: CameraHALStopMisc()
    {
        CAMERA_HAL_LOG_FUNC;
        sem_destroy(&avab_dequeue_frame);
        sem_destroy(&avab_show_frame);
        mCaptureDevice->DevStop();
        mCaptureDevice->DevDeAllocate();
        //CloseCaptureDevice();
    }
    status_t CameraHal :: PrepareCaptureDevices()
    {
        CAMERA_HAL_LOG_FUNC;
        status_t ret = NO_ERROR;
        int i =0;
        unsigned int CaptureBufNum = mCaptureBufNum;
        struct capture_config_t *pCapcfg;
        if ((ret = OpenCaptureDevice())<0)
            return ret;
		//add this step is used to seperate setparameters and startpreview
		//as setparameters is only to set source 
		//and startpreview is to set all other parameters
		mCaptureDeviceCfg.input_source |= 0x80;
        if (mCaptureDevice->DevSetConfig(&mCaptureDeviceCfg) < 0) {//set the config and get the captured framesize
            CAMERA_HAL_ERR("Dev config failed");
            return BAD_VALUE;
        }
        mCaptureFrameSize = mCaptureDeviceCfg.framesize;

        if (mCaptureDevice->DevAllocateBuf(mCaptureBuffers,&CaptureBufNum)< 0){
            CAMERA_HAL_ERR("capture device allocat buf error");
            return BAD_VALUE;
        }
        if(mCaptureBufNum != CaptureBufNum){
            CAMERA_HAL_LOG_INFO("The driver can only supply %d bufs, but required %d bufs", CaptureBufNum, mCaptureBufNum);
        }

        mCaptureBufNum = CaptureBufNum;

        if (mCaptureDevice->DevPrepare()< 0){
            CAMERA_HAL_ERR("capture device prepare error");
            return BAD_VALUE;
        }
        nCameraBuffersQueued = mCaptureBufNum;

        return ret;
    }

    status_t CameraHal::PreparePreviwBuf()
    {
        CAMERA_HAL_LOG_FUNC;
        status_t ret = NO_ERROR;
        unsigned int i =0;
        mPreviewFrameSize = mCaptureDeviceCfg.width*mCaptureDeviceCfg.height * 3;
        mPreviewHeap.clear();
        for (i = 0; i< mPreviewHeapBufNum; i++)
            mPreviewBuffers[i].clear();
        mPreviewHeap = new MemoryHeapBase(mPreviewFrameSize * mPreviewHeapBufNum);
        if (mPreviewHeap == NULL)
            return NO_MEMORY;
        for (i = 0; i < mPreviewHeapBufNum; i++)
            mPreviewBuffers[i] = new MemoryBase(mPreviewHeap, mPreviewFrameSize* i, mPreviewFrameSize);
        return ret;
    }

    status_t CameraHal ::PreparePreviwMisc()
    {
        CAMERA_HAL_LOG_FUNC;
        status_t ret = NO_ERROR;
        dequeue_head = 0;
        preview_heap_buf_head = 0;
        display_head = 0;
        error_status = 0;
        is_first_buffer = 1;
        last_display_index = 0;

        sem_init(&avab_dequeue_frame, 0, mCaptureBufNum);
        sem_init(&avab_show_frame, 0, 0);
        return ret;
    }

    status_t CameraHal ::CameraHALPreviewStart()
    {
        CAMERA_HAL_LOG_FUNC;
        status_t ret = NO_ERROR;
        if (mCaptureDevice->DevStart()<0)
            return INVALID_OPERATION;
	 
        mCaptureFrameThread = new CaptureFrameThread(this);
        mPreviewShowFrameThread = new PreviewShowFrameThread(this);
	 
        if (mCaptureFrameThread == NULL ||
                mPreviewShowFrameThread == NULL){
            return UNKNOWN_ERROR;
        }
	 
        mPreviewRunning = true;
        return ret;
    }


    int CameraHal ::captureframeThread()
    {
        CAMERA_HAL_LOG_FUNC;

        unsigned int DeqBufIdx = 0;
        struct timespec ts;

        do {
            clock_gettime(CLOCK_REALTIME, &ts);
            ts.tv_nsec +=100000; // 100ms
        } while (mPreviewRunning && !error_status &&(sem_timedwait(&avab_dequeue_frame, &ts) != 0) );

        if(!mPreviewRunning || error_status)
            return UNKNOWN_ERROR;

        mCaptureDevice->DevDequeue(&DeqBufIdx);

        nCameraBuffersQueued--;

        buffer_index_maps[dequeue_head]=DeqBufIdx;
        dequeue_head ++;
        dequeue_head %= mCaptureBufNum;

        sem_post(&avab_show_frame);
        return NO_ERROR;
    }

    int CameraHal::color_space_888_to_565(DMA_BUFFER InBuf)
    {
	  CAMERA_HAL_LOG_FUNC;
	  ipu_lib_input_param_t sIPUInputParam;   
	  ipu_lib_output_param_t sIPUOutputParam; 
	  ipu_lib_handle_t            sIPUHandle;
	  int iIPURet = 0;
	  memset(&sIPUInputParam,0,sizeof(sIPUInputParam));
	  memset(&sIPUOutputParam,0,sizeof(sIPUOutputParam));
	  memset(&sIPUHandle,0,sizeof(sIPUHandle));
         //Setting input format
	  sIPUInputParam.width = 800;
	  sIPUInputParam.height = 480;
	  sIPUInputParam.input_crop_win.pos.x = 0;
	  sIPUInputParam.input_crop_win.pos.y = 0;  
	  sIPUInputParam.input_crop_win.win_w = 800;
	  sIPUInputParam.input_crop_win.win_h = 480;
	  sIPUInputParam.fmt = v4l2_fourcc('R', 'G', 'B', '3');
	  sIPUInputParam.user_def_paddr[0] = InBuf.phy_offset;

	  //Setting output format
	  //Should align with v4l
	  sIPUOutputParam.fmt = v4l2_fourcc('R', 'G', 'B', 'P');
	  sIPUOutputParam.width = 800;
	  sIPUOutputParam.height = 480;   
	  sIPUOutputParam.show_to_fb = 1;
	  sIPUOutputParam.fb_disp.fb_num = 2;
	  //Output param should be same as input, since no resize,crop
	  sIPUOutputParam.output_win.pos.x = 0;
	  sIPUOutputParam.output_win.pos.y = 0;
	  sIPUOutputParam.output_win.win_w = 800;
	  sIPUOutputParam.output_win.win_h = 480;

	  //saveBMP888((uint8_t*)(InBuf.virt_start));

	  CAMERA_HAL_LOG_INFO("color_space_888_to_565++");
	  iIPURet =  mxc_ipu_lib_task_init(&sIPUInputParam, NULL, &sIPUOutputParam, OP_NORMAL_MODE|TASK_ENC_MODE, &sIPUHandle);
         if (iIPURet < 0) {
		CAMERA_HAL_LOG_INFO("Error!mxc_ipu_lib_task_init failed mIPURet %d!",iIPURet);
		return -1;
	  }  
	  CAMERA_HAL_LOG_INFO("mxc_ipu_lib_task_init success");
         iIPURet = mxc_ipu_lib_task_buf_update(&sIPUHandle,  InBuf.phy_offset, NULL,NULL,NULL,NULL);
         if (iIPURet < 0) {
              CAMERA_HAL_LOG_INFO("Error!mxc_ipu_lib_task_buf_update failed mIPURet %d!",iIPURet);
              mxc_ipu_lib_task_uninit(&sIPUHandle);
              return -1;
         }
         CAMERA_HAL_LOG_INFO("mxc_ipu_lib_task_buf_update success");

	  //saveBMP565((uint16_t*)(sIPUHandle.outbuf_start));
		 
         mxc_ipu_lib_task_uninit(&sIPUHandle);
	  CAMERA_HAL_LOG_INFO("color_space_888_to_565--");
         return 0;
    }

    int CameraHal ::previewshowFrameThread()
    {
        CAMERA_HAL_LOG_FUNC;
        struct timespec ts;
        int display_index = 0;
        DMA_BUFFER InBuf;
        int queue_back_index = 0;
	  
        do {
            clock_gettime(CLOCK_REALTIME, &ts);
            ts.tv_nsec +=100000; // 100ms
        } while (!error_status && mPreviewRunning &&(sem_timedwait(&avab_show_frame, &ts) != 0) );

        if ((mPreviewRunning == 0) || error_status)
            return UNKNOWN_ERROR;

        display_index = buffer_index_maps[display_head];
        InBuf = mCaptureBuffers[display_index];
        display_head ++;
        display_head %= mCaptureBufNum;

	 //color_space_888_to_565(InBuf);

        if((mMsgEnabled & CAMERA_MSG_PREVIEW_FRAME) && (mOverlay == 0) ){			
            memcpy((uint8_t*)(mPreviewBuffers[preview_heap_buf_head]->pointer()), (uint8_t*)(InBuf.virt_start), mCaptureDeviceCfg.width * mCaptureDeviceCfg.height *  2);
            mDataCb(CAMERA_MSG_PREVIEW_FRAME, mPreviewBuffers[preview_heap_buf_head], mCallbackCookie);
            preview_heap_buf_head ++;
            preview_heap_buf_head %= mPreviewHeapBufNum;
        }

        pthread_mutex_lock(&mOverlayMutex);

        if (mOverlay != 0) {
	     //saveBMP565((uint16_t*)(InBuf.virt_start));
              //saveBMP888(InBuf.virt_start);
             if (mOverlay->queueBuffer((overlay_buffer_t)InBuf.phy_offset) < 0){
                CAMERA_HAL_ERR("queueBuffer failed. May be bcos stream was not turned on yet.");
            }
            if (is_first_buffer) {
                is_first_buffer = 0;
                last_display_index = display_index;
                pthread_mutex_unlock(&mOverlayMutex);
                goto show_out;
            }
        }

        if (mOverlay != 0){
            queue_back_index = last_display_index;
        }else{
            queue_back_index = display_index;
        }
        pthread_mutex_unlock(&mOverlayMutex);

        //queue the v4l2 buf back
        if(mCaptureDevice->DevQueue(queue_back_index) <0){
            CAMERA_HAL_ERR("The Capture device queue buf error !!!!");
            return INVALID_OPERATION;
        }
        last_display_index = display_index;
        nCameraBuffersQueued++;
        sem_post(&avab_dequeue_frame);

show_out:
        return NO_ERROR;
    }

    	void CameraHal::saveBMP565(uint16_t* pinBuff)
	{
	       int ii,jj;
		static uint32_t i = 0;
		unsigned long ul24BitBufSize = 800 * 480 * 3;
		uint8_t* p24BitBuf = (uint8_t*)malloc(ul24BitBufSize);;
	       char fileNamePath[30];
	       char temp[10];
		const uint8_t head[] = {0x42,0x4d,0x36,0x94,0x11,0x00,0x00,0x00,0x00,0x00,0x36,0x00,0x00,0x00,0x28,0x00,
							0x00,0x00,0x20,0x03,0x00,0x00,0xe0,0x01,0x00,0x00,0x01,0x00,0x18,0x00,0x00,0x00,
							0x00,0x00,0x00,0x94,0x11,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
							0x00,0x00,0x00,0x00,0x00,0x00};

		static long last_save = 0;
		long now = time(NULL);
                if( (last_save == 0) || (now - last_save) > 15){	// if we are first in and the diff is bigger than 30, we try to save one bmp
                    LOGW("saveBMP565 Try Save Now");
                    last_save = now;
                }else{
                    //LOGW("saveBMP565 Next Save Time Left: %d", (now - last_save));
                    if(p24BitBuf)
			free(p24BitBuf);	
                    return;
                }
	        i++;
#if 0
		if( i <= 5){
		}
		else if(/*(i < 300) ||*/ (i > 600)){
			return;
		}
#endif
		if( 1/*((i % 20) == 0) || (i <= 5)*/){
			LOGW("saveBMP565 Call with %d", i);			
			LOGW("saveBMP565 Call with 0x%x", p24BitBuf);
			if(p24BitBuf == NULL){
				LOGW("!!!MALLOC FAILED !!! [0x%x] : %s", errno, strerror(errno));
				return;
			}
			uint32_t numwritten  = 0;
		        memset(p24BitBuf, 0xFF, ul24BitBufSize);
                        //Transform the format from frame-buffer into the BMP bit buffer.
		        for (ii=0; ii<(int)480; ii++){
		            for (jj=0; jj<(int)800; jj++){
	              	           uint16_t w16BitRGB;
	                    	   w16BitRGB=*(pinBuff + (ii*800)+ jj);
	                           //Normal Picture 
	                           unsigned long ul24BitPos=((ii*800)+jj)*3;
	                           //Keep in BGR order
	                           *(p24BitBuf+ul24BitPos+2)=(uint8_t)((w16BitRGB&0xf800)>>8);
	                           *(p24BitBuf+ul24BitPos+1)=(uint8_t)((w16BitRGB&0x07e0)>>3);
	                           *(p24BitBuf+ul24BitPos)=(uint8_t)((w16BitRGB&0x001f)<<3);
	                    }
	                 }	
			 LOGW("saveBMP565 Memory Saved...");
	                 memset(temp, 0, 10);
	                 memset(fileNamePath,0, 30);
	                 strcpy(fileNamePath,"/data/");
                         sprintf(temp,"%d_565", i);
	                 strcat(fileNamePath,temp);
	                 strcat(fileNamePath,(".bmp"));
                         LOGI("open file: %s", fileNamePath);
			 FILE *pFile = fopen(fileNamePath, "wb"); 
			 if(pFile == 0){
			     LOGW("WRITE TMP FILE ERROR!\n");	
			     free(p24BitBuf);
			     return;
			 }
	                 numwritten = fwrite( head, sizeof(uint8_t), sizeof(head)/sizeof(head[0]), pFile );
	        	 if (numwritten != sizeof(head)/sizeof(head[0])) {
			     LOGW("WRITE head FILE ERROR!\n");
	                 }    
			   
			 // Write DIB bits
	                 numwritten = fwrite(p24BitBuf, sizeof(uint8_t), ul24BitBufSize, pFile );
	        	 if(numwritten != ul24BitBufSize) {
                             LOGW("WRITE bmp data FILE ERROR!\n");
	                 }    
			   fclose(pFile);			
		}
		free(p24BitBuf);	
		return;
	}

	void CameraHal::saveBMP888(uint8_t* pinBuff)
	{
	       int ii,jj;
		static uint32_t i = 0;
		unsigned long ul24BitBufSize = 800 * 480 * 3;
		uint8_t* p24BitBuf = (uint8_t*)malloc(ul24BitBufSize);;
		//uint8_t* p24BitBuf = (uint8_t*)(mCaptureBuffers_overlay[0].virt_start);
		
	       char fileNamePath[30];
	       char temp[10];
		const uint8_t head[] = {0x42,0x4d,0x36,0x94,0x11,0x00,0x00,0x00,0x00,0x00,0x36,0x00,0x00,0x00,0x28,0x00,
							0x00,0x00,0x20,0x03,0x00,0x00,0xe0,0x01,0x00,0x00,0x01,0x00,0x18,0x00,0x00,0x00,
							0x00,0x00,0x00,0x94,0x11,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
							0x00,0x00,0x00,0x00,0x00,0x00};
	       i++;
		if( i <= 5){
		}
		else if((i < 300) || (i > 400)){
			return;
		}
		if( ((i % 20) == 0) || (i <= 5)){
			LOGW("saveBMP888 Call with %d", i);
			uint32_t numwritten  = 0;
		       memset(p24BitBuf, 0xFF, ul24BitBufSize);
	       	 //Transform the format from frame-buffer into the BMP bit buffer.
		        for (ii=0; ii<(int)480; ii++) 
	       	 {
		            for (jj=0; jj<(int)800; jj++) 
	       	     {
          	      uint8_t u8ColorValueR = *(pinBuff + (((ii*800)+jj)*3));
          	      uint8_t u8ColorValueG = *(pinBuff + (((ii*800)+jj)*3) + 1);
          	      uint8_t u8ColorValueB = *(pinBuff + (((ii*800)+jj)*3) + 2);						  						  
                       //Normal Picture 
                       unsigned long ul24BitPos=((ii*800)+jj)*3;
                       //Keep in RGB order
                       *(p24BitBuf+ul24BitPos+2)=(uint8_t)(u8ColorValueR);
                       *(p24BitBuf+ul24BitPos+1)=(uint8_t)(u8ColorValueG);
                       *(p24BitBuf+ul24BitPos)=(uint8_t)(u8ColorValueB);
                }
             }
	           memset(temp, 0, 10);
	           memset(fileNamePath,0, 30);
	           strcpy(fileNamePath,"/data/");
			   sprintf(temp,"%d_888", (i <= 5 ? i : (i / 10)));
	           strcat(fileNamePath,temp);
	           strcat(fileNamePath,(".bmp"));
			   LOGI("open file: %s", fileNamePath);
			   FILE *pFile = fopen(fileNamePath, "wb"); 
			   if(pFile == 0){
				   free(p24BitBuf);
				   return;
			   }
	            numwritten = fwrite( head, sizeof(uint8_t), sizeof(head)/sizeof(head[0]), pFile );
	        	if (numwritten != sizeof(head)/sizeof(head[0])) {
				LOGW("WRITE head FILE ERROR!\n");
	                 }    
			   
			   // Write DIB bits
	            numwritten = fwrite( p24BitBuf, sizeof(uint8_t), ul24BitBufSize, pFile );
	        	   if (numwritten != ul24BitBufSize) {
				   LOGW("WRITE bmp data FILE ERROR!\n");
	                 }    
			   fclose(pFile);			
		}
		free(p24BitBuf);	
		return;
	}


    status_t CameraHal :: AllocateRecordVideoBuf()
    {
        status_t ret = NO_ERROR;
        return ret;
    }


    void CameraHal :: LockWakeLock()
    {
        if (!mPowerLock) {
            acquire_wake_lock (PARTIAL_WAKE_LOCK, V4LSTREAM_WAKE_LOCK);
            mPowerLock = true;
        }
    }
    void CameraHal :: UnLockWakeLock()
    {
        if (mPowerLock) {
            release_wake_lock (V4LSTREAM_WAKE_LOCK);
            mPowerLock = false;
        }
    }

// Tractor only have one tw8832 as camera and in this camera we support three mode: mpeg/cmmb/bcam
static CameraInfo sCameraInfo = {CAMERA_FACING_BACK, 0};
static char Camera_name[MAX_SENSOR_NAME] = "tw8832\0";
    int HAL_getNumberOfCameras()
    {
        return 1;
    }

    void HAL_getCameraInfo(int cameraId, struct CameraInfo* cameraInfo)
    {
        memcpy(cameraInfo, &sCameraInfo, sizeof(CameraInfo));					
    }

    sp<CameraHardwareInterface> HAL_openCameraHardware(int cameraId)
    {
        sp<CaptureDeviceInterface> pCaptureDevice = NULL;

        if (HAL_getNumberOfCameras() ==0 ){
            CAMERA_HAL_ERR("There is no configure for Cameras");
            return NULL;
        }

        pCaptureDevice = createCaptureDevice(Camera_name);

        CameraHal *pCameraHal = new CameraHal();
        if (pCameraHal->setCaptureDevice(pCaptureDevice) < 0)
            return NULL;

        if (pCameraHal->Init() < 0)
            return NULL;

        sp<CameraHardwareInterface> hardware(pCameraHal);
        CAMERA_HAL_LOG_INFO("created the tokenwireless Camera hal");

        return hardware;
    }

};



