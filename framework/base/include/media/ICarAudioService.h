/*
 * Copyright (C) 2009 The Android Open Source Project
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

#ifndef ANDROID_ICARAUDIOSERVICE_H
#define ANDROID_ICARAUDIOSERVICE_H

#include <stdint.h>
#include <sys/types.h>
#include <unistd.h>

#include <utils/RefBase.h>
#include <utils/Errors.h>
#include <binder/IInterface.h>
#include <media/AudioSystem.h>

namespace android {

// ----------------------------------------------------------------------------

class ICarAudioService : public IInterface
{
public:
    DECLARE_META_INTERFACE(CarAudioService);

    //
    // ICarAudioService interface (see CarAudioServiceInterface for method descriptions)
    //
    virtual status_t getIVIAudioLibVersionH() = 0;
	virtual status_t getIVIAudioLibVersionL() = 0;

	virtual status_t switchChannel(int channel, bool on) = 0;

	virtual status_t adjustHardwareVolume(int directions) = 0;
	virtual status_t muteAll(int mute) = 0;
	virtual status_t popLess(int mute) = 0;
	virtual status_t shutdownMute(int mute) = 0;
	virtual status_t isMute() = 0;
	virtual status_t muteMic(int mute) = 0;
	virtual status_t isMicMute() = 0;
	virtual status_t a2dpHolding(int holding) = 0;
};


// ----------------------------------------------------------------------------

class BnCarAudioService : public BnInterface<ICarAudioService>
{
public:
    virtual status_t    onTransact( uint32_t code,
                                    const Parcel& data,
                                    Parcel* reply,
                                    uint32_t flags = 0);
};

// ----------------------------------------------------------------------------

}; // namespace android

#endif // ANDROID_ICARAUDIOSERVICE_H