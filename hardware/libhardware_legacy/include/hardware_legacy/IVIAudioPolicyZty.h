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


#include <stdint.h>
#include <sys/types.h>
#include <utils/Timers.h>
#include <utils/Errors.h>
#include <utils/KeyedVector.h>
#include <hardware_legacy/AudioPolicyInterface.h>


namespace android {

// ----------------------------------------------------------------------------

class IVIAudioPolicyZty
{

public:
                IVIAudioPolicyZty(AudioPolicyClientInterface *clientInterface);
        virtual ~IVIAudioPolicyZty();

		status_t setRoute(int channel, bool on);
		int getStreamIndex(int channel);
		
private:
	
		AudioPolicyClientInterface *mpClientInterface;	// audio policy client interface
	
		int mCurStreamActiveCount[AudioSystem::NUM_STREAM_TYPES];
		int mPrevStreamActiveCount[AudioSystem::NUM_STREAM_TYPES];

};

};

