/* //device/content/providers/media/src/com/android/providers/media/MediaScannerReceiver.java
**
** Copyright 2007, The Android Open Source Project
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
/* Copyright (c) 2011 Freescale Semiconductor, Inc. */

package com.android.providers.media;

import android.content.Context;
import android.content.Intent;
import android.content.BroadcastReceiver;
import android.net.Uri;
import android.os.Environment;
import android.os.Bundle;
import android.util.Slog;
import java.io.File;

public class MediaScannerReceiver extends BroadcastReceiver
{
    private final static String TAG = "MediaScannerReceiver";

    @Override
    public void onReceive(Context context, Intent intent) {
        String action = intent.getAction();
        Uri uri = intent.getData();
        String externalSDStoragePath = Environment.getExternalSDStorageDirectory().getPath();
        String externalUDiskStoragePath = Environment.getExternalUDiskStorageDirectory().getPath();
         if (action.equals(Intent.ACTION_BOOT_COMPLETED)) {     // although we hope not scan it, but the bluetooth ringtone still may need it...
            scan(context, MediaProvider.INTERNAL_VOLUME);
        } else{
            // OK ,whatever, if the intent is file related, we will do following check...
            if (uri.getScheme().equals("file")) {
                // handle intents related to external storage
                String path = uri.getPath();
                if (action.equals(Intent.ACTION_MEDIA_MOUNTED)) {
                    if (externalSDStoragePath.equals(path)) // if sdcard is plug in...
                        scan(context, MediaProvider.EXTERNAL_VOLUME_SD);    // notify the scannerservice to start
                    else if (externalUDiskStoragePath.equals(path)) // if udisk is plug in
                        scan(context, MediaProvider.EXTERNAL_VOLUME_UDISK); // notify the scannerservice to start
                    else 
                         Slog.w(TAG, "Volume path " + path + " Audo Scan Not Support");
                } else if (action.equals(Intent.ACTION_MEDIA_SCANNER_SCAN_FILE) &&
                    path != null && (path.startsWith(externalSDStoragePath + "/") || path.startsWith(externalUDiskStoragePath + "/"))) {
                    scanFile(context, path);
                }
            }
        }
    }

    private void scan(Context context, String volume) {
        Bundle args = new Bundle();
        args.putString("volume", volume);
        Slog.i(TAG, "scan volume: " + volume);
        context.startService(
                new Intent(context, MediaScannerService.class).putExtras(args));
    }    

    private void scanFile(Context context, String path) {
        Bundle args = new Bundle();
        args.putString("filepath", path);
        Slog.i(TAG, "scanFile path: " + path);
        context.startService(
                new Intent(context, MediaScannerService.class).putExtras(args));
    }    
}


