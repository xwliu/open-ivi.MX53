/*
 * Copyright (C) 2011 Freescale Semiconductor Inc.
 * Copyright (C) 2008 - 2011 Atheros Corporation. All rights reserved.
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
 *
 */

#include <errno.h>
#include <pthread.h>
#include <termios.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <math.h>
#include <time.h>
#include <semaphore.h>
#include <signal.h>
#include <unistd.h>
#include <stdio.h>

#define  LOG_TAG  "tractor_gps"

#include <cutils/log.h>
#include <cutils/sockets.h>
#include <cutils/properties.h>
#include <hardware/gps.h>

#define  GPS_DEBUG
//#define GPS_DEBUG_TOKEN

#define KNOTS_TO_KILOMETERS     1.852f
#define KNOTS_TO_METERS     0.514f



#define  DFR(...)   LOGD(__VA_ARGS__)

#ifdef GPS_DEBUG
#  define  D(...)   LOGD(__VA_ARGS__)
#else
#  define  D(...)   ((void)0)
#endif

#define GPS_STATUS_CB(_cb, _s)    \
  if ((_cb).status_cb) {          \
    GpsStatus gps_status;         \
    gps_status.status = (_s);     \
    (_cb).status_cb(&gps_status); \
    DFR("gps status callback: 0x%x", _s); \
  }

static time_t utc_mktime(struct tm *_tm);

/* Nmea Parser stuff */
#define  NMEA_MAX_SIZE  83

enum {
  STATE_QUIT  = 0,
  STATE_INIT  = 1,
  STATE_START = 2
};

typedef struct {
    int     pos;
    int     overflow;
    int     utc_year;
    int     utc_mon;
    int     utc_day;
    GpsLocation  fix;
    GpsSvStatus  sv_status;
    int     sv_status_changed;
    char    in[ NMEA_MAX_SIZE+1];
    int     gsa_fixed;
} NmeaReader;

/* Since NMEA parser requires locks */
/*
TODO: 1. Change to function call
      2. Add processing of EINVAL, ENOSYS, EDEADLK, EINTR (debug output at least)
*/
#define GPS_STATE_LOCK_FIX(_s)         \
{                                      \
  int ret;                             \
  do {                                 \
    ret = sem_wait(&(_s)->fix_sem);    \
  } while (ret < 0 && errno == EINTR);   \
}

/*
TODO: 1. Change to function call
      2. Add processing of EINVAL, ENOSYS (debug output at least)
*/
#define GPS_STATE_UNLOCK_FIX(_s)       \
  sem_post(&(_s)->fix_sem)

typedef struct {
    int                     init;
    int                     fd;
    GpsCallbacks            callbacks;
    pthread_t               thread;
    pthread_t               tmr_thread;
    int                     control[2];
    int                     fix_freq;
    sem_t                   fix_sem;
    int                     first_fix;
    NmeaReader              reader;
} GpsState;

static GpsState  _gps_state[1];
static GpsState *gps_state = _gps_state;

static void *gps_timer_thread( void*  arg );


/*****************************************************************/
/*****************************************************************/
/*****                                                       *****/
/*****                 U T I L I T I E S                     *****/
/*****                                                       *****/
/*****************************************************************/
/*****************************************************************/


/*****************************************************************/
/*****************************************************************/
/*****                                                       *****/
/*****       N M E A   T O K E N I Z E R                     *****/
/*****                                                       *****/
/*****************************************************************/
/*****************************************************************/

typedef struct {
    const char*  p;
    const char*  end;
} Token;

#define  MAX_NMEA_TOKENS  32

typedef struct {
    int     count;
    Token   tokens[ MAX_NMEA_TOKENS ];
} NmeaTokenizer;

static int
nmea_tokenizer_init( NmeaTokenizer*  t, const char*  p, const char*  end )
{
    int    count = 0;
    char*  q;

    // the initial '$' is optional
    if (p < end && p[0] == '$')
        p += 1;

    // remove trailing newline
    if (end > p && end[-1] == '\n') {
        end -= 1;
        if (end > p && end[-1] == '\r')
            end -= 1;
    }

    // get rid of checksum at the end of the sentecne
    if (end >= p+3 && end[-3] == '*') {
        end -= 3;
    }

    while (p < end) {
        const char*  q = p;

        q = memchr(p, ',', end-p);
        if (q == NULL)
            q = end;

        if (count < MAX_NMEA_TOKENS) {
            t->tokens[count].p   = p;
            t->tokens[count].end = q;
            count += 1;
        }

        if (q < end)
            q += 1;

        p = q;
    }

    t->count = count;
    return count;
}

static Token
nmea_tokenizer_get( NmeaTokenizer*  t, int  index )
{
    Token  tok;
    static const char*  dummy = "";

    if (index < 0 || index >= t->count) {
        tok.p = tok.end = dummy;
    } else
        tok = t->tokens[index];

    return tok;
}


static int
str2int( const char*  p, const char*  end )
{
    int   result = 0;
    int   len    = end - p;

    if (len == 0) {
      return -1;
    }

    for ( ; len > 0; len--, p++ )
    {
        int  c;

        if (p >= end)
            goto Fail;

        c = *p - '0';
        if ((unsigned)c >= 10)
            goto Fail;

        result = result*10 + c;
    }
    return  result;

Fail:
    return -1;
}

static double
str2float( const char*  p, const char*  end )
{
    int   result = 0;
    int   len    = end - p + 1;
    char  temp[32];

    if (len == 0) {
      return -1.0;
    }

    if (len >= (int)sizeof(temp))
        return 0.;

    memcpy( temp, p, len );
    temp[len] = 0;
    return strtod( temp, NULL );
}

/*****************************************************************/
/*****************************************************************/
/*****                                                       *****/
/*****       N M E A   P A R S E R                           *****/
/*****                                                       *****/
/*****************************************************************/
/*****************************************************************/

static void nmea_reader_init( NmeaReader*  r )
{
    memset( r, 0, sizeof(*r) );

    r->pos      = 0;
    r->overflow = 0;
    r->utc_year = -1;
    r->utc_mon  = -1;
    r->utc_day  = -1;

}

static int
nmea_reader_update_time( NmeaReader*  r, Token  tok )
{
    int        hour, minute;
    double     seconds;
    struct tm  tm;
    time_t     fix_time;

    if (tok.p + 6 > tok.end)
        return -1;

    if (r->utc_year < 0) {
        // no date yet, get current one
        time_t  now = time(NULL);
        gmtime_r( &now, &tm );
        r->utc_year = tm.tm_year + 1900;
        r->utc_mon  = tm.tm_mon + 1;
        r->utc_day  = tm.tm_mday;
    }

    hour    = str2int(tok.p,   tok.p+2);
    minute  = str2int(tok.p+2, tok.p+4);
    seconds = str2float(tok.p+4, tok.end);

    tm.tm_hour = hour;
    tm.tm_min  = minute;
    tm.tm_sec  = (int) seconds;
    tm.tm_year = r->utc_year - 1900;
    tm.tm_mon  = r->utc_mon - 1;
    tm.tm_mday = r->utc_day;

    fix_time = utc_mktime( &tm );
    r->fix.timestamp = (long long)fix_time * 1000;

	//D("nmea_reader_update_time: %lf", (double)r->fix.timestamp);
    return 0;
}

static int
nmea_reader_update_cdate( NmeaReader*  r, Token  tok_d, Token tok_m, Token tok_y )
{

    if ( (tok_d.p + 2 > tok_d.end) ||
         (tok_m.p + 2 > tok_m.end) ||
         (tok_y.p + 4 > tok_y.end) )
        return -1;

    r->utc_day = str2int(tok_d.p,   tok_d.p+2);
    r->utc_mon = str2int(tok_m.p, tok_m.p+2);
    r->utc_year = str2int(tok_y.p, tok_y.end+4);

    return 0;
}

static int
nmea_reader_update_date( NmeaReader*  r, Token  date, Token  mtime )
{
    Token  tok = date;
    int    day, mon, year;

    if (tok.p + 6 != tok.end) {
 
        D("no date info, use host time as default: '%.*s'", tok.end-tok.p, tok.p);
        /* no date info, will use host time in _update_time function
           jhung 2010/05/12
         */
        // return -1;
    }
    /* normal case */
    day  = str2int(tok.p, tok.p+2);
    mon  = str2int(tok.p+2, tok.p+4);
    year = str2int(tok.p+4, tok.p+6) + 2000;

    if ((day|mon|year) < 0) {
        D("date not properly formatted: '%.*s'", tok.end-tok.p, tok.p);
        return -1;
    }

    r->utc_year  = year;
    r->utc_mon   = mon;
    r->utc_day   = day;
	
	//D("nmea_reader_update_time: %d, %d, %d", r->utc_year, r->utc_mon, r->utc_day);
    return nmea_reader_update_time( r, mtime );
}


static double
convert_from_hhmm( Token  tok )
{
    double  val     = str2float(tok.p, tok.end);
    int     degrees = (int)(floor(val) / 100);
    double  minutes = val - degrees*100.;
    double  dcoord  = degrees + minutes / 60.0;
    return dcoord;
}


static int
nmea_reader_update_latlong( NmeaReader*  r,
                            Token        latitude,
                            char         latitudeHemi,
                            Token        longitude,
                            char         longitudeHemi )
{
    double   lat, lon;
    Token    tok;

    tok = latitude;
    if (tok.p + 6 > tok.end) {
        D("latitude is too short: '%.*s'", tok.end-tok.p, tok.p);
        return -1;
    }
    lat = convert_from_hhmm(tok);
    if (latitudeHemi == 'S')
        lat = -lat;

    tok = longitude;
    if (tok.p + 6 > tok.end) {
        D("longitude is too short: '%.*s'", tok.end-tok.p, tok.p);
        return -1;
    }
    lon = convert_from_hhmm(tok);
    if (longitudeHemi == 'W')
        lon = -lon;

    r->fix.flags    |= GPS_LOCATION_HAS_LAT_LONG;
    r->fix.latitude  = lat;
    r->fix.longitude = lon;

	//D("nmea_reader_update_latlong: %1f, %1f", lat, lon);
    return 0;
}


static int
nmea_reader_update_altitude( NmeaReader*  r,
                             Token        altitude,
                             Token        units )
{
    double  alt;
    Token   tok = altitude;

    if (tok.p >= tok.end)
        return -1;

    r->fix.flags   |= GPS_LOCATION_HAS_ALTITUDE;
    r->fix.altitude = str2float(tok.p, tok.end);

	//D("nmea_reader_update_altitude: %1f", r->fix.altitude);
    return 0;
}

static int
nmea_reader_update_accuracy( NmeaReader*  r,
                             Token        accuracy )
{
    double  acc;
    Token   tok = accuracy;

    if (tok.p >= tok.end)
        return -1;

    //tok is cep*cc, we only want cep
    r->fix.accuracy = str2float(tok.p, tok.end);

    if (r->fix.accuracy == 99.99){
      return 0;
    }

    r->fix.flags   |= GPS_LOCATION_HAS_ACCURACY;
    return 0;
}

static int
nmea_reader_update_bearing( NmeaReader*  r,
                            Token        bearing )
{
    double  alt;
    Token   tok = bearing;

    if (tok.p >= tok.end)
        return -1;

    r->fix.flags   |= GPS_LOCATION_HAS_BEARING;
    r->fix.bearing  = str2float(tok.p, tok.end);
    return 0;
}


static int
nmea_reader_update_speed_meters( NmeaReader*  r,
                          Token        speed )
{
    double  alt;
    Token   tok = speed;

    if (tok.p >= tok.end)
        return -1;

    r->fix.flags   |= GPS_LOCATION_HAS_SPEED;
    r->fix.speed    = str2float(tok.p, tok.end);

	//D("nmea_reader_update_speed: %1f", r->fix.speed*KNOTS_TO_METERS);
	//LOGE("$$$nmea_reader_update_speed: %1f", r->fix.speed);

	r->fix.speed = (r->fix.speed*KNOTS_TO_METERS);
    return 0;
}

static int
nmea_reader_update_speed( NmeaReader*  r,
                          Token        speed )
{
    double  alt;
    Token   tok = speed;

    if (tok.p >= tok.end)
        return -1;

    r->fix.flags   |= GPS_LOCATION_HAS_SPEED;
    r->fix.speed    = str2float(tok.p, tok.end);

	//D("nmea_reader_update_speed: %1f", r->fix.speed);
    return 0;
}


static void
nmea_reader_parse( NmeaReader*  r )
{
   /* we received a complete sentence, now parse it to generate
    * a new GPS fix...
    */
    NmeaTokenizer  tzer[1];
    Token          tok;

    //"Received: '%.*s'", r->pos, r->in);

    if (r->pos < 9)
	{
        D("Too short. discarded.");
        return;
    }

#if 1 
	/* for NMEAListener callback */
    if (gps_state->callbacks.nmea_cb)
	{
        //D("NMEAListener callback... ");
        struct timeval tv;
        unsigned long long mytimems;

        gettimeofday(&tv,NULL);
        mytimems = tv.tv_sec * 1000LL + tv.tv_usec / 1000;

        gps_state->callbacks.nmea_cb(mytimems, r->in, r->pos);

        //D("NMEAListener callback ended... ");
    }
#endif

    nmea_tokenizer_init(tzer, r->in, r->in + r->pos);
#ifdef GPS_DEBUG_TOKEN
    {
        int  n;
        D("Found %d tokens", tzer->count);
        for (n = 0; n < tzer->count; n++) {
            Token  tok = nmea_tokenizer_get(tzer,n);
            D("%2d: '%.*s'", n, tok.end-tok.p, tok.p);
        }
    }
#endif

    tok = nmea_tokenizer_get(tzer, 0);

    if (tok.p + 5 > tok.end) {
        /* for $PUNV sentences, jhung */
        if ( !memcmp(tok.p, "PUNV", 4) )
		{
            // D("$PUNV sentences found!");
            Token tok_cfg = nmea_tokenizer_get(tzer,1);

            if (!memcmp(tok_cfg.p, "CFG_R", 5)) {
                D("$PUNV,CFG_R found! ");
            } else if ( !memcmp(tok_cfg.p, "QUAL", 4) ) {
                Token  tok_sigma_x   = nmea_tokenizer_get(tzer, 3);

                if (tok_sigma_x.p[0] != ',') {
                    Token tok_accuracy      = nmea_tokenizer_get(tzer, 10);
                    nmea_reader_update_accuracy(r, tok_accuracy);
                }
            } else if (!memcmp(tok_cfg.p, "TIMEMAP", 7))
            {
                Token systime = nmea_tokenizer_get(tzer, 8); // system time token
                Token timestamp = nmea_tokenizer_get(tzer, 2); // UTC time token
                D("TIMEMAP systime='%.*s'", systime.end - systime.p, systime.p);
                D("TIMEMAP timestamp='%.*s'", timestamp.end - timestamp.p, timestamp.p);
                //nmea_reader_update_timemap(r, systime, timestamp);
            }
        }else{
            D("sentence id '%.*s' too short, ignored.", tok.end-tok.p, tok.p);
        }
        return;
    }

    // ignore first two characters.
    tok.p += 2;

    if ( !memcmp(tok.p, "GGA", 3) ) {
        // GPS fix
        //D("$GGA");
        Token  tok_fixstaus      = nmea_tokenizer_get(tzer,6);

        if (tok_fixstaus.p[0] > '0') {

          Token  tok_time          = nmea_tokenizer_get(tzer,1);
          Token  tok_latitude      = nmea_tokenizer_get(tzer,2);
          Token  tok_latitudeHemi  = nmea_tokenizer_get(tzer,3);
          Token  tok_longitude     = nmea_tokenizer_get(tzer,4);
          Token  tok_longitudeHemi = nmea_tokenizer_get(tzer,5);
          Token  tok_altitude      = nmea_tokenizer_get(tzer,9);
          Token  tok_altitudeUnits = nmea_tokenizer_get(tzer,10);

          nmea_reader_update_time(r, tok_time);
          nmea_reader_update_latlong(r, tok_latitude,
                                        tok_latitudeHemi.p[0],
                                        tok_longitude,
                                        tok_longitudeHemi.p[0]);
          nmea_reader_update_altitude(r, tok_altitude, tok_altitudeUnits);
        }

    } else if ( !memcmp(tok.p, "GLL", 3) ) {
		//D("$GLL");
        Token  tok_fixstaus      = nmea_tokenizer_get(tzer,6);

        if (tok_fixstaus.p[0] == 'A') {

          Token  tok_latitude      = nmea_tokenizer_get(tzer,1);
          Token  tok_latitudeHemi  = nmea_tokenizer_get(tzer,2);
          Token  tok_longitude     = nmea_tokenizer_get(tzer,3);
          Token  tok_longitudeHemi = nmea_tokenizer_get(tzer,4);
          Token  tok_time          = nmea_tokenizer_get(tzer,5);

          nmea_reader_update_time(r, tok_time);
          nmea_reader_update_latlong(r, tok_latitude,
                                        tok_latitudeHemi.p[0],
                                        tok_longitude,
                                        tok_longitudeHemi.p[0]);
        }
    } else if ( !memcmp(tok.p, "GSA", 3) ) {
		//D("$GSA");
        Token  tok_fixStatus   = nmea_tokenizer_get(tzer, 2);
        int i;

        if (tok_fixStatus.p[0] != '\0' && tok_fixStatus.p[0] != '1') {

//          Token  tok_accuracy      = nmea_tokenizer_get(tzer, 15);

//          nmea_reader_update_accuracy(r, tok_accuracy);

          r->sv_status.used_in_fix_mask = 0ul;

          for (i = 3; i <= 14; ++i){

            Token  tok_prn  = nmea_tokenizer_get(tzer, i);
            int prn = str2int(tok_prn.p, tok_prn.end);

            /* only available for PRN 1-32 */
            if ((prn > 0) && (prn < 33)){
              // bug, prn 1 should be put on bit 0
              // r->sv_status.used_in_fix_mask |= (1ul << (32 - prn));
              r->sv_status.used_in_fix_mask |= (1ul << (prn-1));
              r->sv_status_changed = 1;
              /* mark this parameter to identify the GSA is in fixed state */
              r->gsa_fixed = 1;
              // D("%s: fix mask is %ld (PRN: %d)", __FUNCTION__, r->sv_status.used_in_fix_mask, prn);
            }

          }

        }else {
          if (r->gsa_fixed == 1) {
            D("%s: GPGSA fixed -> unfixed", __FUNCTION__);
            r->sv_status.used_in_fix_mask = 0ul;
            r->sv_status_changed = 1;
            r->gsa_fixed = 0;
          }
        }
    } else if ( !memcmp(tok.p, "GSV", 3) ) {
		//D("$GSV");
        Token  tok_noSatellites  = nmea_tokenizer_get(tzer, 3);
        int    noSatellites = str2int(tok_noSatellites.p, tok_noSatellites.end);

        if (noSatellites > 0) {

          Token  tok_noSentences   = nmea_tokenizer_get(tzer, 1);
          Token  tok_sentence      = nmea_tokenizer_get(tzer, 2);

          int sentence = str2int(tok_sentence.p, tok_sentence.end);
          int totalSentences = str2int(tok_noSentences.p, tok_noSentences.end);
          int curr;
          int i;
 
          if (sentence == 1) {
              r->sv_status_changed = 0;
              r->sv_status.num_svs = 0;
          }

          curr = r->sv_status.num_svs;

          i = 0;

          while (i < 4 && r->sv_status.num_svs < noSatellites){

                 Token  tok_prn = nmea_tokenizer_get(tzer, i * 4 + 4);
                 Token  tok_elevation = nmea_tokenizer_get(tzer, i * 4 + 5);
                 Token  tok_azimuth = nmea_tokenizer_get(tzer, i * 4 + 6);
                 Token  tok_snr = nmea_tokenizer_get(tzer, i * 4 + 7);

                 r->sv_status.sv_list[curr].prn = str2int(tok_prn.p, tok_prn.end);
                 r->sv_status.sv_list[curr].elevation = str2float(tok_elevation.p, tok_elevation.end);
                 r->sv_status.sv_list[curr].azimuth = str2float(tok_azimuth.p, tok_azimuth.end);
                 r->sv_status.sv_list[curr].snr = str2float(tok_snr.p, tok_snr.end);

                 r->sv_status.num_svs += 1;

                 curr += 1;

                 i += 1;
          }

          if (sentence == totalSentences) {
              r->sv_status_changed = 1;
          }

          // D("%s: GSV message with total satellites %d", __FUNCTION__, noSatellites);

        }

    } else if ( !memcmp(tok.p, "RMC", 3) ) {
		//D("$RMC");
        Token  tok_fixStatus     = nmea_tokenizer_get(tzer,2);

        /* always write back location and time information
           jhung, 20100512
        */
        if (tok_fixStatus.p[0] == 'A')
        {
          Token  tok_time          = nmea_tokenizer_get(tzer,1);
          Token  tok_latitude      = nmea_tokenizer_get(tzer,3);
          Token  tok_latitudeHemi  = nmea_tokenizer_get(tzer,4);
          Token  tok_longitude     = nmea_tokenizer_get(tzer,5);
          Token  tok_longitudeHemi = nmea_tokenizer_get(tzer,6);
          Token  tok_speed         = nmea_tokenizer_get(tzer,7);
          Token  tok_bearing       = nmea_tokenizer_get(tzer,8);
          Token  tok_date          = nmea_tokenizer_get(tzer,9);

            nmea_reader_update_date( r, tok_date, tok_time );

            nmea_reader_update_latlong( r, tok_latitude,
                                           tok_latitudeHemi.p[0],
                                           tok_longitude,
                                           tok_longitudeHemi.p[0] );

            nmea_reader_update_bearing( r, tok_bearing );
            nmea_reader_update_speed_meters  ( r, tok_speed );
        }

    } else if ( !memcmp(tok.p, "VTG", 3) ) {
		//D("$VTG");
        Token  tok_fixStatus     = nmea_tokenizer_get(tzer,9);

        if (tok_fixStatus.p[0] != '\0' && tok_fixStatus.p[0] != 'N')
        {
        	
            Token  tok_bearing       = nmea_tokenizer_get(tzer,1);
            Token  tok_speed         = nmea_tokenizer_get(tzer,5);

            nmea_reader_update_bearing( r, tok_bearing );
            nmea_reader_update_speed_meters  ( r, tok_speed );
        }
    } else if ( !memcmp(tok.p, "ZDA", 3) ) {
		//D("$ZDA");
        Token  tok_time;
        Token  tok_year  = nmea_tokenizer_get(tzer,4);

        if (tok_year.p[0] != '\0') {

          Token  tok_day   = nmea_tokenizer_get(tzer,2);
          Token  tok_mon   = nmea_tokenizer_get(tzer,3);

          nmea_reader_update_cdate( r, tok_day, tok_mon, tok_year );

        }

        tok_time  = nmea_tokenizer_get(tzer,1);

        if (tok_time.p[0] != '\0') {

          nmea_reader_update_time(r, tok_time);

        }


    } else {
        tok.p -= 2;
        DFR("unknown sentence '%.*s", tok.end-tok.p, tok.p);
    }

    if (!gps_state->first_fix &&
        gps_state->init == STATE_INIT &&
        r->fix.flags & GPS_LOCATION_HAS_LAT_LONG) {

        if (gps_state->callbacks.location_cb) {
            gps_state->callbacks.location_cb( &r->fix );
            r->fix.flags = 0;
        }

        gps_state->first_fix = 1;
    }

}

static void
nmea_reader_addc( NmeaReader*  r, int  c )
{
    int cnt;

    if (r->overflow) {
        r->overflow = (c != '\n');
        return;
    }

    if (r->pos >= (int) sizeof(r->in)-1 ) {
        r->overflow = 1;
        r->pos      = 0;
        return;
    }

    r->in[r->pos] = (char)c;
    r->pos       += 1;

    if (c == '\n')
	{
        GPS_STATE_LOCK_FIX(gps_state);
        // D("Parsing NMEA commands");
        nmea_reader_parse( r );
        GPS_STATE_UNLOCK_FIX(gps_state);
        r->pos = 0;
    }
}

/*****************************************************************/
/*****************************************************************/
/*****                                                       *****/
/*****       C O N N E C T I O N   S T A T E                 *****/
/*****                                                       *****/
/*****************************************************************/
/*****************************************************************/

/* commands sent to the gps thread */
enum {
    CMD_QUIT  = 0,
    CMD_START = 1,
    CMD_STOP  = 2
};


static void gps_state_update_fix_freq(GpsState *s, int fix_freq)
{

  s->fix_freq = fix_freq;
  return;

}


static void
gps_state_done( GpsState*  s )
{
    // tell the thread to quit, and wait for it
    char   cmd = CMD_QUIT;
    void*  dummy;
    int ret;

    DFR("gps send quit command");

    do { ret=write( s->control[0], &cmd, 1 ); }
    while (ret < 0 && errno == EINTR);

    DFR("gps waiting for command thread to stop");

    pthread_join(s->thread, &dummy);

    /* Timer thread depends on this state check */
    s->init = STATE_QUIT;
    s->fix_freq = -1;

    // close the control socket pair
    close( s->control[0] ); s->control[0] = -1;
    close( s->control[1] ); s->control[1] = -1;

    // close connection to the QEMU GPS daemon
    close( s->fd ); s->fd = -1;

    sem_destroy(&s->fix_sem);

    memset(s, 0, sizeof(*s));

    DFR("gps deinit complete");

}


static void
gps_state_start( GpsState*  s )
{
    char  cmd = CMD_START;
    int   ret;

    do
	{
		ret=write( s->control[0], &cmd, 1 );
	}while (ret < 0 && errno == EINTR);



    if (ret != 1)
        D("%s: could not send CMD_START command: ret=%d: %s",
          __FUNCTION__, ret, strerror(errno));
}


static void
gps_state_stop( GpsState*  s )
{
    char  cmd = CMD_STOP;
    int   ret;

    do { ret=write( s->control[0], &cmd, 1 ); }
    while (ret < 0 && errno == EINTR);

    if (ret != 1)
        D("%s: could not send CMD_STOP command: ret=%d: %s",
          __FUNCTION__, ret, strerror(errno));
}


static int
epoll_register( int  epoll_fd, int  fd )
{
    struct epoll_event  ev;
    int                 ret, flags;

    /* important: make the fd non-blocking */
    flags = fcntl(fd, F_GETFL);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);

    ev.events  = EPOLLIN;
    ev.data.fd = fd;
    do {
        ret = epoll_ctl( epoll_fd, EPOLL_CTL_ADD, fd, &ev );
    } while (ret < 0 && errno == EINTR);
    return ret;
}


static int
epoll_deregister( int  epoll_fd, int  fd )
{
    int  ret;
    do {
        ret = epoll_ctl( epoll_fd, EPOLL_CTL_DEL, fd, NULL );
    } while (ret < 0 && errno == EINTR);
    return ret;
}

/* this is the main thread, it waits for commands from gps_state_start/stop and,
 * when started, messages from the QEMU GPS daemon. these are simple NMEA sentences
 * that must be parsed to be converted into GPS fixes sent to the framework
 */
static void*
gps_state_thread( void*  arg )
{
    GpsState*   state = (GpsState*) arg;
    NmeaReader  *reader;
    int         epoll_fd   = epoll_create(2);
    int         started    = 0;
    int         gps_fd     = state->fd;
    int         control_fd = state->control[1];

    reader = &state->reader;

    nmea_reader_init( reader );

    // register control file descriptors for polling
    epoll_register( epoll_fd, control_fd );
    epoll_register( epoll_fd, gps_fd );

    D("gps thread running");


    // now loop
    for (;;)
	{
        struct epoll_event   events[2];
        int                  ne, nevents;

        nevents = epoll_wait( epoll_fd, events, 2, -1 );
        if (nevents < 0)
		{
            if (errno != EINTR)
                LOGE("epoll_wait() unexpected error: %s", strerror(errno));
            continue;
        }
        // D("gps thread received %d events", nevents);
        for (ne = 0; ne < nevents; ne++)
		{
            if ((events[ne].events & (EPOLLERR|EPOLLHUP)) != 0)
			{
                LOGE("EPOLLERR or EPOLLHUP after epoll_wait() !?");
                goto Exit;
            }
            if ((events[ne].events & EPOLLIN) != 0)
			{
                int  fd = events[ne].data.fd;

                if (fd == control_fd)
                {
                    char  cmd = 255;
                    int   ret;
                    D("gps control fd event");
                    do
					{
                        ret = read( fd, &cmd, 1 );
                    } while (ret < 0 && errno == EINTR);

                    if (cmd == CMD_QUIT)
					{
                        D("gps thread quitting on demand");
                        goto Exit;
                    }
                    else if (cmd == CMD_START)
					{
                        if (!started)
						{
                            D("gps thread starting  location_cb=%p", state->callbacks.location_cb);
                            started = 1;

                            GPS_STATUS_CB(state->callbacks, GPS_STATUS_SESSION_BEGIN);

                            state->init = STATE_START;

						    state->tmr_thread = state->callbacks.create_thread_cb("tractor_gps_tmr", gps_timer_thread, state);
						    if (!state->tmr_thread)
							{
                                LOGE("could not create gps timer thread: %s", strerror(errno));
                                started = 0;
                                state->init = STATE_INIT;
                                goto Exit;
                            }

                        }
                    }
                    else if (cmd == CMD_STOP) {
                        if (started) {
                            void *dummy;
                            D("gps thread stopping");
                            started = 0;

                            state->init = STATE_INIT;

                            pthread_join(state->tmr_thread, &dummy);

                            GPS_STATUS_CB(state->callbacks, GPS_STATUS_SESSION_END);

                        }
                    }
                }
                else if (fd == gps_fd)
                {
                    char buf[512];
                    int  nn, ret;

		    		// D("gps fd event start");
                    do {
							fd_set readfds;
							FD_ZERO(&readfds);
							FD_SET(fd, &readfds);

							ret = select(FD_SETSIZE, &readfds, NULL, NULL, NULL);
							if (ret < 0)
								continue;
							if (FD_ISSET(fd, &readfds))
								ret = read( fd, buf, sizeof(buf) );
                        // D("read buffer size: %d", ret);
                    } while (ret < 0 && errno == EINTR);

                    if (ret > 0)
                        for (nn = 0; nn < ret; nn++)
                            nmea_reader_addc( reader, buf[nn] );

                    // D("gps fd event end");
                }
                else
                {
                    LOGE("epoll_wait() returned unkown fd %d ?", fd);
                }
            }
        }
    }
Exit:

    return NULL;
}

static void*
gps_timer_thread( void*  arg )
{
	int need_sleep = 0;
	int sleep_val = 0;
	int continue_thread = 0;

  GpsState *state = (GpsState *)arg;

  DFR("gps entered timer thread");

  do {

    //DFR ("gps timer exp");

    GPS_STATE_LOCK_FIX(state);

    //if (state->reader.fix.flags != 0) {
	// RGE 2011-02-08
	if ((state->reader.fix.flags & GPS_LOCATION_HAS_LAT_LONG) != 0)
	{
      //D("$gps fix cb: 0x%x", state->reader.fix.flags);

      if (state->callbacks.location_cb)
	  {
          state->callbacks.location_cb( &state->reader.fix );
          state->reader.fix.flags = 0;
          state->first_fix = 1;
      }

      if (state->fix_freq == 0)
	  {
        state->fix_freq = -1;
      }

    }

    if (state->reader.sv_status_changed != 0)
	{

      // D("gps sv status callback");

      if (state->callbacks.sv_status_cb)
	  {
          state->callbacks.sv_status_cb( &state->reader.sv_status );
          state->reader.sv_status_changed = 0;
      }

    }

	// RGE 2011-02-08
	need_sleep = (state->fix_freq != -1 && state->init == STATE_START ? 1 : 0);
	sleep_val = state->fix_freq;

	GPS_STATE_UNLOCK_FIX(state);

	// RGE 2011-02-08
    if (need_sleep)
	{
		sleep(sleep_val);
	}

    GPS_STATE_LOCK_FIX(state);
	continue_thread = state->init == STATE_START ? 1 : 0;
	GPS_STATE_UNLOCK_FIX(state);

  } while(continue_thread);

  DFR("gps timer thread destroyed");

  return NULL;

}

static void
gps_state_init( GpsState*  state )
{
    char   prop[PROPERTY_VALUE_MAX];
    char   device[256];
    int    ret;
    int    done = 0;

    struct sigevent tmr_event;

    state->init       = STATE_INIT;
    state->control[0] = -1;
    state->control[1] = -1;
    state->fd         = -1;
    state->fix_freq   = -1;
    state->first_fix  = 0;

    if (sem_init(&state->fix_sem, 0, 1) != 0) {
      D("gps semaphore initialization failed! errno = %d", errno);
      return;
    }

    do
	{
	 // In tractor, we are using ttymxc1 as GPS Uart
        state->fd = open( "/dev/ttymxc1", O_RDWR | O_NOCTTY | O_NONBLOCK);
    } while (state->fd < 0 && errno == EINTR);

    if (state->fd < 0)
	{
        LOGE("could not open gps serial device %s: %s", device, strerror(errno) );
        return;
    }

	D("gps will read from file handle:%d", state->fd);

    // disable echo on serial lines
    if ( isatty( state->fd ) )
	{
        struct termios  ios;
        tcgetattr( state->fd, &ios );

        D("set to 9600bps, 8-N-1");
        bzero(&ios, sizeof(ios));
        ios.c_cflag = B9600 | CS8 | CLOCAL | CREAD;
        ios.c_iflag = IGNPAR;
        ios.c_oflag = 0;
        ios.c_lflag = 0;  /* disable ECHO, ICANON, etc... */

        tcsetattr( state->fd, TCSANOW, &ios );
    }

    if ( socketpair( AF_LOCAL, SOCK_STREAM, 0, state->control ) < 0 )
	{
        LOGE("could not create thread control socket pair: %s", strerror(errno));
        goto Fail;
    }

	state->thread = state->callbacks.create_thread_cb("tractor_gps", gps_state_thread, state);
        if (!state->thread)
	{
        LOGE("could not create gps thread: %s", strerror(errno));
        goto Fail;
    }

	D("gps state initialized");

    return;

Fail:
    gps_state_done( state );
}


/*****************************************************************/
/*****************************************************************/
/*****                                                       *****/
/*****       I N T E R F A C E                               *****/
/*****                                                       *****/
/*****************************************************************/
/*****************************************************************/


static int gps_init(GpsCallbacks* callbacks)
{
    GpsState*  s = _gps_state;

    s->callbacks = *callbacks;

    if (!s->init)
        gps_state_init(s);

    if (s->fd < 0)
        return -1;

    return 0;
}

static void
gps_cleanup(void)
{
    GpsState*  s = _gps_state;

    if (s->init)
        gps_state_done(s);
}

static int run_hook(char* name)
{
    char   prop[PROPERTY_VALUE_MAX];
    char   buf[PROPERTY_VALUE_MAX + 20];
    if (property_get("athr.gps.hookspath",prop,"") == 0)
	{
        DFR("%s: athr.gps.hookspath property is not set", __FUNCTION__);
		return 0;
    }
    sprintf(buf,"%s/%s" , prop, name);
	DFR("%s: going to execute hook  \"%s\"", __FUNCTION__, buf);
    return !system(buf);
}

static int run_hook_start()
{
	return run_hook("start");
}

static int run_hook_stop()
{
	return run_hook("stop");
}

static int gps_start()
{
    GpsState*  s = _gps_state;

    if (!s->init)
	{
        DFR("%s: called with uninitialized state !!", __FUNCTION__);
        return -1;
    }

    D("%s: called", __FUNCTION__);

    if (run_hook_start())
	{
        DFR("%s: Hook says: quit now", __FUNCTION__);
    }
	else
	{
        /* send $PUNV,WAKEUP*2C command, jhung */
		D("Send WAKEUP command to GPS");
        if (write(s->fd, "$PUNV,WAKEUP*2C\r\n", 17) < 0)
        {
            D("Send WAKEUP command ERROR!");
        }
    }
    gps_state_start(s);
    return 0;
}


static int
gps_stop()
{
    GpsState*  s = _gps_state;

    if (!s->init) {
        DFR("%s: called with uninitialized state !!", __FUNCTION__);
        return -1;
    }

	D("%s: called", __FUNCTION__);

    if (run_hook_stop()) {
        DFR("%s: Hook says: quit now", __FUNCTION__);
    } else {
        /* send $PUNV,SLEEP*7E command, jhung */
        D("Send SLEEP command to GPS");
        if (write(s->fd, "$PUNV,SLEEP*7E\r\n", 16) < 0)
        {
            D("Send SLEEP command ERROR!");
        }
    }
    gps_state_stop(s);
    return 0;
}


static void
gps_set_fix_frequency(int freq)
{
    GpsState*  s = _gps_state;

    if (!s->init) {
        DFR("%s: called with uninitialized state !!", __FUNCTION__);
        return;
    }

    D("%s: called", __FUNCTION__);

    s->fix_freq = (freq <= 0) ? 1 : freq;

    D("gps fix frquency set to %d secs", freq);
}

static int
gps_inject_time(GpsUtcTime time, int64_t timeReference, int uncertainty)
{
    return 0;
}

static void
gps_delete_aiding_data(GpsAidingData flags)
{
}

static int
gps_inject_location(double latitude, double longitude, float accuracy)
{
    return 0;
}



static int gps_set_position_mode(GpsPositionMode mode, GpsPositionRecurrence recurrence,
									  uint32_t min_interval, uint32_t preferred_accuracy, uint32_t preferred_time)
{
	GpsState*  s = _gps_state;

	// only standalone is supported for now.

	if (mode != GPS_POSITION_MODE_STANDALONE)
	{
		D("%s: set GPS POSITION mode error! (mode:%d) ", __FUNCTION__, mode);
		D("Set as standalone mode currently! ");
		//        return -1;
	}

	if (!s->init || min_interval < 0) {
		D("%s: called with uninitialized state !!", __FUNCTION__);
		return -1;
	}

	s->fix_freq = min_interval/1000;
	if (s->fix_freq ==0)
	{
		s->fix_freq =1;
	}

	D("gps fix frquency set to %d milli-secs", min_interval);

	return 0;

}



static const void* gps_get_extension(const char* name)
{
    if (strcmp(name, GPS_XTRA_INTERFACE) == 0)
    {
    }
    else if (strcmp(name, GPS_NI_INTERFACE) == 0)
    {
    }
    D("%s: no GPS extension for %s is found", __FUNCTION__, name);
    return NULL;
}

// RGE 2011-02-08
static const GpsInterface  TractorGpsInterface = {
    .size =sizeof(GpsInterface),
    .init = gps_init,
    .start = gps_start,
    .stop = gps_stop,
    .cleanup = gps_cleanup,
    .inject_time = gps_inject_time,
    .inject_location = gps_inject_location,
    .delete_aiding_data = gps_delete_aiding_data,
    .set_position_mode = gps_set_position_mode,
    .get_extension = gps_get_extension,
};

const GpsInterface* gps_get_hardware_interface()
{
    return &TractorGpsInterface;
}


#define SECONDS_PER_MIN (60)
#define SECONDS_PER_HOUR (60*SECONDS_PER_MIN)
#define SECONDS_PER_DAY  (24*SECONDS_PER_HOUR)
#define SECONDS_PER_NORMAL_YEAR (365*SECONDS_PER_DAY) 
#define SECONDS_PER_LEAP_YEAR (SECONDS_PER_NORMAL_YEAR+SECONDS_PER_DAY) 


static int days_per_month_no_leap[] =
    {31,28,31,30,31,30,31,31,30,31,30,31};
static int days_per_month_leap[] =
    {31,29,31,30,31,30,31,31,30,31,30,31};

static int is_leap_year(int year)
{
    if ((year%400) == 0)
        return 1;
    if ((year%100) == 0)
        return 0;
    if ((year%4) == 0)
        return 1;
    return 0;
}
static int number_of_leap_years_in_between(int from, int to)
{
    int n_400y, n_100y, n_4y;
    n_400y = to/400 - from/400;
    n_100y = to/100 - from/100;
    n_4y = to/4 - from/4;
    return (n_4y - n_100y + n_400y);
}

static time_t utc_mktime(struct tm *_tm)
{
    time_t t_epoch=0;
    int m; 
    int *days_per_month;
    if (is_leap_year(_tm->tm_year+1900))
        days_per_month = days_per_month_leap;
    else
        days_per_month = days_per_month_no_leap;
    t_epoch += (_tm->tm_year - 70)*SECONDS_PER_NORMAL_YEAR; 
    t_epoch += number_of_leap_years_in_between(1970,_tm->tm_year+1900) *
        SECONDS_PER_DAY;
    for (m=0; m<_tm->tm_mon; m++) {
        t_epoch += days_per_month[m]*SECONDS_PER_DAY;
    }
    t_epoch += (_tm->tm_mday-1)*SECONDS_PER_DAY;
    t_epoch += _tm->tm_hour*SECONDS_PER_HOUR;
    t_epoch += _tm->tm_min*SECONDS_PER_MIN;
    t_epoch += _tm->tm_sec;
    return t_epoch;
}

/* for GPSXtraIterface */

/*****************************************************************/
/*****************************************************************/
/*****                                                       *****/
/*****       D E V I C E                                     *****/
/*****                                                       *****/
/*****************************************************************/
/*****************************************************************/
