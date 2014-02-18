/*
 * Copyright 2007, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.android.commands.kmsgtracker;

import android.app.ActivityManagerNative;
import android.app.IActivityController;
import android.app.IActivityManager;
import android.content.ComponentName;
import android.content.Intent;
import android.content.pm.IPackageManager;
import android.content.pm.ResolveInfo;
import android.os.Build;
import android.os.Debug;
import android.os.Environment;
import android.os.Process;
import android.os.RemoteException;
import android.os.ServiceManager;
import android.os.StrictMode;
import android.os.SystemClock;
import android.os.SystemProperties;
import android.view.IWindowManager;

import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.ByteArrayInputStream;
import java.io.DataInputStream;
import java.io.File;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.Writer;
import java.security.SecureRandom;
import java.util.ArrayList;
import java.util.HashSet;
import java.util.Iterator;
import java.util.List;
import java.util.Random;

import android.util.Log;
import android.os.SystemProperties;
import java.io.File;
import java.util.Date;
import java.text.SimpleDateFormat;
import java.io.FileWriter;
import java.util.Random;




/**
 * Application that injects random key events and other actions into the system.
 */
public class KmsgTracker {
	
    private final static String COMMAND = "cat /proc/kmsg";

	private final static String TAG = "KmsgTracker";

    /**
     * Command-line entry point.
     *
     * @param args The command-line arguments
     */
    public static void main(String[] args) {
		Log.i(TAG, "run KmsgTracker with arg: " + args);
		int resultCode = (new KmsgTracker()).run(args);
        System.exit(resultCode);
    }

	/**
     * Run the command!
     *
     * @param args The command-line arguments
     * @return Returns a posix-style result code. 0 for no error.
     */
    private int run(String[] args) {
		Log.i(TAG, "KmsgTracker running +++");
	
		try {
            // Process must be fully qualified here because android.os.Process
            // is used elsewhere
            java.lang.Process process = Runtime.getRuntime().exec(COMMAND);

            // pipe everything from process stdout -> System.err
            InputStream inStream = process.getInputStream();
            InputStreamReader inReader = new InputStreamReader(inStream);
            BufferedReader inBuffer = new BufferedReader(inReader);
            String s;
			
            while (true) {
				String lineLog = inBuffer.readLine();
				if(lineLog != null){
					Log.d(TAG, lineLog);
				}
            }
           
        } catch (Exception e) {
            System.err.println(e.toString());
        }
		Log.i(TAG, "KmsgTracker running ---");
		return 0;
	}

}
