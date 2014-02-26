/* //device/content/providers/media/src/com/android/providers/media/MediaScannerService.java
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

import android.app.Service;
import android.content.ContentValues;
import android.content.Context;
import android.content.Intent;
import android.content.res.Configuration;
import android.content.res.Resources;
import android.database.Cursor;
import android.os.ServiceManager;
import android.media.IMediaScannerListener;
import android.media.IMediaScannerService;
import android.os.storage.IMountService;
import android.media.MediaScanner;
import android.net.Uri;
import android.os.Bundle;
import android.os.Environment;
import android.os.FileUtils;
import android.os.Handler;
import android.os.IBinder;
import android.os.Looper;
import android.os.Message;
import android.os.PowerManager;
import android.os.Process;
import android.os.SystemProperties;
import android.provider.MediaStore;
import android.util.Config;
import android.util.Log;
import java.util.Arrays;


import java.io.File;
import java.util.Locale;

public class MediaScannerService extends Service implements Runnable
{
    private static final String TAG = "MediaScannerService";
    private static final boolean TWLS_DEBUG_INFO = true;
    private static final boolean TWLS_DEBUG_VERBOSE = false;


    private volatile Looper mServiceLooper;
    private volatile ServiceHandler mServiceHandler;
    private PowerManager.WakeLock mWakeLock;
    private DirectoryScanThread mScanDirThr = null;
    
    public class DirectoryScanThread extends Thread {
            private String[] mDirectories;
            private String mVolume;
            public DirectoryScanThread(String[] directories, String volume){
                mDirectories = Arrays.copyOf(directories,  directories.length); 
                mVolume = volume;
            }
            public void run() {
                Log.i(TAG, "DirectoryScanThread Start to run with Volume:  " + mVolume);
                // reduce priority below other background threads to avoid interfering
                // with other services at boot time.
                Process.setThreadPriority(Process.THREAD_PRIORITY_LOWEST);
                scan(mDirectories, mVolume);
                Log.i(TAG,  "DirectoryScanThread finish Volume:  " + mVolume);                
            }
    }
    
    private void openDatabase(String volumeName) {
        try {
            ContentValues values = new ContentValues();
            values.put("name", volumeName);
            getContentResolver().insert(Uri.parse("content://media/"), values);
        } catch (IllegalArgumentException ex) {
            Log.w(TAG, "failed to open media database");
        }         
    }

    private void closeDatabase(String volumeName) {
        try {
            getContentResolver().delete(
                    Uri.parse("content://media/" + volumeName), null, null);
        } catch (Exception e) {
            Log.w(TAG, "failed to close media database " + volumeName + " exception: " + e);
        }
    }

    private MediaScanner createMediaScanner() {
        MediaScanner scanner = new MediaScanner(this);
        Locale locale = getResources().getConfiguration().locale;
        if (locale != null) {
            String language = locale.getLanguage();
            String country = locale.getCountry();
            String localeString = null;
            if (language != null) {
                if (country != null) {
                    scanner.setLocale(language + "_" + country);
                } else {
                    scanner.setLocale(language);
                }
            }    
        }
        
        return scanner;
    }

    private void scan(String[] directories, String volumeName) {
        // don't sleep while scanning
        mWakeLock.acquire();
        if(TWLS_DEBUG_INFO) Log.i(TAG, "scan directories: " + directories.toString() + " volumeName: " + volumeName);
        ContentValues values = new ContentValues();
        values.put(MediaStore.MEDIA_SCANNER_VOLUME, volumeName);
        Uri scanUri = getContentResolver().insert(MediaStore.getMediaScannerUri(), values);

        Uri uri = Uri.parse("file://" + directories[0]);
        sendBroadcast(new Intent(Intent.ACTION_MEDIA_SCANNER_STARTED, uri));
        
        try {
            if (volumeName.equals(MediaProvider.EXTERNAL_VOLUME_SD) ||
                volumeName.equals(MediaProvider.EXTERNAL_VOLUME_UDISK) ||
                volumeName.equals(MediaProvider.EXTERNAL_VOLUME_EXTSD)) {
                openDatabase(volumeName);    
            }
            MediaScanner scanner = createMediaScanner();
            if(TWLS_DEBUG_VERBOSE) Log.i(TAG, "++Scan Directory:  " + directories.toString());            
            scanner.scanDirectories(directories, volumeName);
            if(TWLS_DEBUG_VERBOSE) Log.i(TAG, "--Scan Directory:  " + directories.toString());            
        } catch (Exception e) {
            Log.e(TAG, "exception in MediaScanner.scan()", e); 
        }

        getContentResolver().delete(scanUri, null, null);

        sendBroadcast(new Intent(Intent.ACTION_MEDIA_SCANNER_FINISHED, uri));
        mWakeLock.release();
    }
    
    @Override
    public void onCreate()
    {
        PowerManager pm = (PowerManager)getSystemService(Context.POWER_SERVICE);
        mWakeLock = pm.newWakeLock(PowerManager.PARTIAL_WAKE_LOCK, TAG);

        // Start up the thread running the service.  Note that we create a
        // separate thread because the service normally runs in the process's
        // main thread, which we don't want to block.
        Thread thr = new Thread(null, this, "MediaScannerService");
        thr.start();
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId)
    {
        Log.i(TAG, "onStartCommand+");
        while (mServiceHandler == null) {
            synchronized (this) {
                try {
                    wait(100);
                } catch (InterruptedException e) {
                }
            }
        }

        if (intent == null) {
            Log.e(TAG, "Intent is null in onStartCommand: ",
                new NullPointerException());
            return Service.START_NOT_STICKY;
        }

        Message msg = mServiceHandler.obtainMessage();
        msg.arg1 = startId;
        if(TWLS_DEBUG_VERBOSE) Log.i(TAG, "onStartCommand startId: " + startId);

        msg.obj = intent.getExtras();
        String filePath = ((Bundle)(msg.obj)).getString("filepath");
        if(filePath == null){   // if filepath is null, which means we are in the volume scan mode
            String volume = ((Bundle)(msg.obj)).getString("volume");
            if (MediaProvider.INTERNAL_VOLUME.equals(volume))   // if it is internal volume, we have to scan immediately, as there will some incoming call which need ringtone database
                mServiceHandler.sendMessage(msg);
            else
                mServiceHandler.sendMessageDelayed(msg, 10000);
        }else{  // if we are scan file, we send the message without any delay
            mServiceHandler.sendMessage(msg);
        }
        Log.i(TAG, "onStartCommand-");
        // Try again later if we are killed before we can finish scanning.
        return Service.START_REDELIVER_INTENT;
    }

    @Override
    public void onDestroy()
    {
        // Make sure thread has started before telling it to quit.
        while (mServiceLooper == null) {
            synchronized (this) {
                try {
                    wait(100);
                } catch (InterruptedException e) {
                }
            }
        }
        mServiceLooper.quit();
    }

    public void run()
    {
        Looper.prepare();

        mServiceLooper = Looper.myLooper();
        mServiceHandler = new ServiceHandler();

        Looper.loop();
    }
   
    private Uri scanFile(String path, String mimeType) {
        String volumeName = MediaProvider.INTERNAL_VOLUME;
        String externalStoragePathSD = Environment.getExternalSDStorageDirectory().getPath();
        String externalStoragePathUDISK = Environment.getExternalUDiskStorageDirectory().getPath();
        String externalStoragePathEXTSD = Environment.getExternalExtSDStorageDirectory().getPath();

        if (path.startsWith(externalStoragePathSD)) {
            volumeName = MediaProvider.EXTERNAL_VOLUME_SD;
            openDatabase(MediaProvider.EXTERNAL_VOLUME_SD);
        } else if (path.startsWith(externalStoragePathUDISK)) {
            volumeName = MediaProvider.EXTERNAL_VOLUME_UDISK;
            openDatabase(MediaProvider.EXTERNAL_VOLUME_UDISK);
        } else if (path.startsWith(externalStoragePathEXTSD)) {
            volumeName = MediaProvider.EXTERNAL_VOLUME_EXTSD;
            openDatabase(MediaProvider.EXTERNAL_VOLUME_EXTSD);
        }
        if(TWLS_DEBUG_VERBOSE) Log.d(TAG, "scanFile Create MediaScanner");
        MediaScanner scanner = createMediaScanner();
        if(TWLS_DEBUG_VERBOSE) Log.d(TAG, "scanFile scanSingleFile");
        return scanner.scanSingleFile(path, volumeName, mimeType);
    }

    @Override
    public IBinder onBind(Intent intent)
    {
        return mBinder;
    }
    
    private final IMediaScannerService.Stub mBinder = 
            new IMediaScannerService.Stub() {
        public void requestScanFile(String path, String mimeType, IMediaScannerListener listener)
        {
            if (Config.LOGD) {
                Log.d(TAG, "IMediaScannerService.scanFile: " + path + " mimeType: " + mimeType);
            }
            Bundle args = new Bundle();
            args.putString("filepath", path);
            args.putString("mimetype", mimeType);
            if (listener != null) {
                args.putIBinder("listener", listener.asBinder());
            }
            startService(new Intent(MediaScannerService.this,
                    MediaScannerService.class).putExtras(args));
        }

        public void scanFile(String path, String mimeType) {
            requestScanFile(path, mimeType, null);
        }
    };

    IMountService getMs() {
        IBinder service = ServiceManager.getService("mount");
        if (service != null) {
            return IMountService.Stub.asInterface(service);
        } else {
            Log.e(TAG, "Can't get mount service");
        }
        return null;
    }


    private final class ServiceHandler extends Handler
    {
        @Override
        public void handleMessage(Message msg)
        {
            if(TWLS_DEBUG_VERBOSE) Log.d(TAG, "handleMessage+");
            Bundle arguments = (Bundle) msg.obj;
            String filePath = arguments.getString("filepath");
            try {
                if (filePath != null) {
                    IBinder binder = arguments.getIBinder("listener");
                    IMediaScannerListener listener = 
                        (binder == null ? null : IMediaScannerListener.Stub.asInterface(binder));
                    if(TWLS_DEBUG_INFO) Log.d(TAG, "++scanning file " + filePath);
                    Uri uri = scanFile(filePath, arguments.getString("mimetype"));
                    if(TWLS_DEBUG_INFO) Log.d(TAG, "--scanning file " + filePath);
                    if (listener != null) {
                        listener.scanCompleted(filePath, uri);
                    }
                } else {
                    String volume = arguments.getString("volume");
                    String[] directories = null;

                    if (MediaProvider.INTERNAL_VOLUME.equals(volume)) {
                        // scan internal media storage
                        directories = new String[] {
                            Environment.getRootDirectory() + "/media",
                        };
                        // for the internal volume, we have to scan immediately....
                        scan(directories, volume);
                        return;
                    }
                    else if (MediaProvider.EXTERNAL_VOLUME_SD.equals(volume)) {
                        // scan external SD storage
                        directories = new String[] {
                            Environment.getExternalSDStorageDirectory().getPath(),
                        };
                    }
                    else if (MediaProvider.EXTERNAL_VOLUME_UDISK.equals(volume)) {
                        // scan external UDISK storage
                        directories = new String[] {
                            Environment.getExternalUDiskStorageDirectory().getPath(),
                        };
                    }
                     else if (MediaProvider.EXTERNAL_VOLUME_EXTSD.equals(volume)) {
                        // scan external EXTSD storage, should not be called anymore
                        directories = new String[] {
                            Environment.getExternalExtSDStorageDirectory().getPath(),
                        };
                    }
                     if(!Environment.MEDIA_MOUNTED.equals(getMs().getVolumeState(directories[0]))){
                        Log.d(TAG, "Volume " + volume + " Not Mounted ");
                    }else{                    
                        if (directories != null) {
                            if(mScanDirThr == null){
                                if(TWLS_DEBUG_INFO) Log.d(TAG, "start scanning volume " + volume);
                                mScanDirThr = new DirectoryScanThread(directories, volume);
                                mScanDirThr.start();
                            }else{
                                if(mScanDirThr.getState() == Thread.State.TERMINATED){
                                    if(TWLS_DEBUG_INFO) Log.d(TAG, "start scanning volume " + volume);
                                    mScanDirThr = new DirectoryScanThread(directories, volume);
                                    mScanDirThr.start();
                                }else{
                                    Log.d(TAG, "Directory Scan Thread Already Running...Retry 10 seconds later...");
                                    Message msg2 = Message.obtain(msg);
                                    mServiceHandler.sendMessageDelayed(msg2, 10000);
                                }
                            }
                        }
                    }
                }
            } catch (Exception e) {
                Log.e(TAG, "Exception in handleMessage", e);
            }
            //stopSelf(msg.arg1); // can we delete this?
            if(TWLS_DEBUG_VERBOSE) Log.d(TAG, "handleMessage-");
        }
    };
}

