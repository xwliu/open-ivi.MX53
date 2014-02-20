/* ALSAStreamOps.cpp
 **
 ** Copyright 2008-2009 Wind River Systems
 **
 ** Licensed under the Apache License, Version 2.0 (the "License");
 ** you may not use this file except in compliance with the License.
 ** You may obtain a copy of the License at
 **
 **     http://www.apache.org/licenses/LICENSE-2.0
 **
 ** Unless required by applicable law or agreed to in writing, software
 ** distributed under the License is distributed on an "AS IS" BASIS,
 ** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 ** See the License for the specific language governing permissions and
 ** limitations under the License.
 */

#include <errno.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <dlfcn.h>

#define LOG_TAG "AudioHardwareALSA"
#include <utils/Log.h>
#include <utils/String8.h>

#include <cutils/properties.h>
#include <media/AudioRecord.h>
#include <hardware_legacy/power.h>

#include "AudioHardwareALSA.h"

namespace android
{

// ----------------------------------------------------------------------------

ALSAStreamOps::ALSAStreamOps(AudioHardwareALSA *parent, alsa_handle_t *handle) :
    mParent(parent),
    mHandle(handle),
    mPowerLock(false)
{
	LOGV("ALSAStreamOps, samplerate:%d", handle->sampleRate);
}

ALSAStreamOps::~ALSAStreamOps()
{
    AutoMutex lock(mLock);

    close();
}

// use emulated popcount optimization
// http://www.df.lth.se/~john_e/gems/gem002d.html
static inline uint32_t popCount(uint32_t u)
{
    u = ((u&0x55555555) + ((u>>1)&0x55555555));
    u = ((u&0x33333333) + ((u>>2)&0x33333333));
    u = ((u&0x0f0f0f0f) + ((u>>4)&0x0f0f0f0f));
    u = ((u&0x00ff00ff) + ((u>>8)&0x00ff00ff));
    u = ( u&0x0000ffff) + (u>>16);
    return u;
}

acoustic_device_t *ALSAStreamOps::acoustics()
{
    return mParent->mAcousticDevice;
}

ALSAMixer *ALSAStreamOps::mixer()
{
    return mParent->mMixer;
}

status_t ALSAStreamOps::set(int      *format,
                            uint32_t *channels,
                            uint32_t *rate)
{
	LOGV("$$ALSAStreamOps::set++, format:0x%x, channels:0x%x, rate:0x%x, mHandle->devices:0x%x", *format, *channels, *rate, mHandle->devices);
    if (channels && *channels != 0) {
        if (mHandle->channels != popCount(*channels)) {
			LOGD("$$ALSAStreamOps::set,bad value");
            return BAD_VALUE;
        }
		} else if (channels) {
        *channels = 0;
        if (mHandle->devices & AudioSystem::DEVICE_OUT_ALL)
            switch(mHandle->channels) {
                case 4:
                    *channels |= AudioSystem::CHANNEL_OUT_BACK_LEFT;
                    *channels |= AudioSystem::CHANNEL_OUT_BACK_RIGHT;
                    // Fall through...
                default:
                case 2:
                    *channels |= AudioSystem::CHANNEL_OUT_FRONT_RIGHT;
                    // Fall through...
                case 1:
                    *channels |= AudioSystem::CHANNEL_OUT_FRONT_LEFT;
                    break;
            }
        else
            switch(mHandle->channels) {
                default:
                case 2:
                    *channels |= AudioSystem::CHANNEL_IN_RIGHT;
                    // Fall through...
                case 1:
                    *channels |= AudioSystem::CHANNEL_IN_LEFT;
                    break;
            }
    }

    if (rate && *rate > 0) {
        if (mHandle->sampleRate != *rate)
            return BAD_VALUE;
    } else if (rate)
        *rate = mHandle->sampleRate;

    snd_pcm_format_t iformat = mHandle->format;

    if (format) {
        switch(*format) {
            case AudioSystem::FORMAT_DEFAULT:
                break;

            case AudioSystem::PCM_16_BIT:
                iformat = SND_PCM_FORMAT_S16_LE;
                break;

            case AudioSystem::PCM_8_BIT:
                iformat = SND_PCM_FORMAT_S8;
                break;

			case AudioSystem::PCM_20_BIT:
				iformat = SND_PCM_FORMAT_S20_3LE;
				break;

			case AudioSystem::PCM_24_BIT:
				iformat = SND_PCM_FORMAT_S24_LE;
            default:
                LOGE("Unknown PCM format %i. Forcing default", *format);
                break;
        }

        if (mHandle->format != iformat) {
			LOGE("$$ALSAStreamOps::set, format mismatch");
            return BAD_VALUE;
        }

        switch(iformat) {
            default:
            case SND_PCM_FORMAT_S16_LE:
                *format = AudioSystem::PCM_16_BIT;
                break;
            case SND_PCM_FORMAT_S8:
                *format = AudioSystem::PCM_8_BIT;
				break;
				
			case SND_PCM_FORMAT_S20_3LE:
				*format = AudioSystem::PCM_20_BIT;
                break;

			case SND_PCM_FORMAT_S24_LE:
				*format = AudioSystem::PCM_24_BIT;
				break;
        }
    }

	LOGV("$$ALSAStreamOps::set--, format:0x%x, channels:0x%x, rate:0x%x", *format, *channels, *rate);
    return NO_ERROR;
}

status_t ALSAStreamOps::setParameters(const String8& keyValuePairs)
{
    AudioParameter param = AudioParameter(keyValuePairs);
    String8 key = String8(AudioParameter::keyRouting);
    status_t status = NO_ERROR;
    int device;
    LOGV("setParameters() %s", keyValuePairs.string());

    if (param.getInt(key, device) == NO_ERROR) {
        mParent->mALSADevice->route(mHandle, (uint32_t)device, mParent->mode());
        param.remove(key);
    }

    if (param.size()) {
        status = BAD_VALUE;
    }
    return status;
}

String8 ALSAStreamOps::getParameters(const String8& keys)
{
    AudioParameter param = AudioParameter(keys);
    String8 value;
    String8 key = String8(AudioParameter::keyRouting);

    if (param.get(key, value) == NO_ERROR) {
        param.addInt(key, (int)mHandle->curDev);
    }

    LOGV("getParameters() %s", param.toString().string());
    return param.toString();
}

uint32_t ALSAStreamOps::sampleRate() const
{
	//LOGD("$$ALSAStreamOps::sampleRate,mHandle->sampleRate:0x%x",mHandle->sampleRate);
    return mHandle->sampleRate;
}

//
// Return the number of bytes (not frames)
//
size_t ALSAStreamOps::bufferSize() const
{
    snd_pcm_uframes_t bufferSize = mHandle->bufferSize;
    snd_pcm_uframes_t periodSize;

    snd_pcm_get_params(mHandle->handle, &bufferSize, &periodSize);

    size_t bytes = static_cast<size_t>(snd_pcm_frames_to_bytes(mHandle->handle, bufferSize));

	LOGV("$$ALSAStreamOps::bufferSize 111, bytes:%d, bufferSize:%d",bytes,bufferSize);
	
    // Not sure when this happened, but unfortunately it now
    // appears that the bufferSize must be reported as a
    // power of 2. This might be for OSS compatibility.
    for (size_t i = 1; (bytes & ~i) != 0; i<<=1)
        bytes &= ~i;

	LOGV("$$ALSAStreamOps::bufferSize, bytes:%d, mHandle->bufferSize:%d",bytes,mHandle->bufferSize);
    return bytes;
}

int ALSAStreamOps::format() const
{
    int pcmFormatBitWidth;
    int audioSystemFormat;

    snd_pcm_format_t ALSAFormat = mHandle->format;

    pcmFormatBitWidth = snd_pcm_format_physical_width(ALSAFormat);
    switch(pcmFormatBitWidth) {
        case 8:
            audioSystemFormat = AudioSystem::PCM_8_BIT;
            break;

        case 16:
            audioSystemFormat = AudioSystem::PCM_16_BIT;
            break;

		case 20:
			audioSystemFormat = AudioSystem::PCM_20_BIT;
			break;
			
		case 32:
			audioSystemFormat = AudioSystem::PCM_24_BIT;
			break;

		default:
			LOG_FATAL("Unknown AudioSystem bit width %i!", pcmFormatBitWidth);
			audioSystemFormat = AudioSystem::PCM_16_BIT;
			break;

    }

	//LOGD("$$ALSAStreamOps::format, audioSystemFormat:0x%x", audioSystemFormat);
    return audioSystemFormat;
}

uint32_t ALSAStreamOps::frameSize() const
{
	int frameSize = 0;

	LOGV("$$ALSAStreamOps::frameSize++");
	if(format()==AudioSystem::PCM_16_BIT) {
		frameSize = sizeof(int16_t);
	}
	else if(format()==AudioSystem::PCM_8_BIT) {
		frameSize = sizeof(int8_t);
	}
	else if(format()==AudioSystem::PCM_20_BIT) {
		frameSize = sizeof(int32_t);
	}
	else if(format()==AudioSystem::PCM_24_BIT) {
		frameSize = sizeof(int32_t);
	}
	else {
		frameSize = sizeof(int16_t);
	}

	LOGV("$$ALSAStreamOps::frameSize, frameSize:0x%x", frameSize);
	return AudioSystem::popCount(channels())*frameSize; 
}

uint32_t ALSAStreamOps::channels() const
{
    unsigned int count = mHandle->channels;
    uint32_t channels = 0;

	//LOGD("$$ALSAStreamOps::channels,count:%d",count);
	
    if (mHandle->curDev & AudioSystem::DEVICE_OUT_ALL)
        switch(count) {
			case 6:
                channels |= AudioSystem::CHANNEL_OUT_FRONT_CENTER;
                channels |= AudioSystem::CHANNEL_OUT_LOW_FREQUENCY;
                // Fall through...
            case 4:
                channels |= AudioSystem::CHANNEL_OUT_BACK_LEFT;
                channels |= AudioSystem::CHANNEL_OUT_BACK_RIGHT;
                // Fall through...
            default:
            case 2:
                channels |= AudioSystem::CHANNEL_OUT_FRONT_RIGHT;
                // Fall through...
            case 1:
                channels |= AudioSystem::CHANNEL_OUT_FRONT_LEFT;
                break;
        }
    else
        switch(count) {
            default:
            case 2:
                channels |= AudioSystem::CHANNEL_IN_RIGHT;
                // Fall through...
            case 1:
                channels |= AudioSystem::CHANNEL_IN_LEFT;
                break;
        }

	//LOGD("$$ALSAStreamOps::channels-- channels:0x%x",channels);
    return channels;
}

void ALSAStreamOps::close()
{
	LOGD("$$ALSAStreamOps::close");
    mParent->mALSADevice->close(mHandle);
}

//
// Set playback or capture PCM device.  It's possible to support audio output
// or input from multiple devices by using the ALSA plugins, but this is
// not supported for simplicity.
//
// The AudioHardwareALSA API does not allow one to set the input routing.
//
// If the "routes" value does not map to a valid device, the default playback
// device is used.
//
status_t ALSAStreamOps::open(int mode)
{
	LOGD("$$ALSAStreamOps::open");
    return mParent->mALSADevice->open(mHandle, mHandle->curDev, mode);
}

}       // namespace android
