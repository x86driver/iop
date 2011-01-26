#include "UTL_func.h"

static s32
MTH_round_d( double const *value )
{
    s32 r_value;

    if ( *value < 0.0 )
    {
        r_value = (s32) (*value - 0.5);
    } else
    {
        r_value = (s32) (*value + 0.5);
    }
    return r_value;
}

static boolean
UTL_cnvt_gps_time( void* out,
                   void* in)
{
#define UTL_scnds_per_day (60*60*24)

    enum {
        scnds_per_week   = 60*60*24*7,
        days_per_year    = 365,
        days_per_week    = 7,
        gps_to_grm_days  = ((1990-1980)*days_per_year - 5 + 3 -1),
        gps_to_grm_weeks = gps_to_grm_days / days_per_week,
        gps_to_grm_scnds = gps_to_grm_days * UTL_scnds_per_day,
        max_wn           = (0xffffffff / scnds_per_week) + gps_to_grm_weeks,
        tow_at_max       = 0xffffffff % scnds_per_week
    };

    s32 val;

    val = ((UTL_gps_time_type*)in)->wn;
    while (val < gps_to_grm_weeks)
    {
        val = val + 0x400;
    }

    val = val - gps_to_grm_weeks;

    *(date_time_t32*)out = (val * scnds_per_week) + ((UTL_gps_time_type*)in)->tow;

    return TRUE;
}

static boolean
UTL_cnvt_unix_epoch( void* out,
                     void* in)
{
    #define BIAS   (631065600)
    *(u32*)out = *(date_time_t32*)in + BIAS;
    return TRUE;
}

#define UTL_TWN                (u32)8
#define UTL_SCND               (u32)1
#define UTL_MINUTE_SCND        (u32)(UTL_SCND*60)
#define UTL_HOUR_SCND          (u32)(UTL_MINUTE_SCND*60)
#define SYC_DFTL_UTC_TIME_OFST (UTL_TWN*(s32)UTL_HOUR_SCND)

static time_t
HWM_convert_rtc( date_time_t32 const dt)
{
    time_t         linux_time;
    date_time_t32  local_dt;

    local_dt = dt;
    local_dt += SYC_DFTL_UTC_TIME_OFST; /* Add the time diff of TWN:UTC+8 */
    UTL_cnvt_unix_epoch(&linux_time,&local_dt);

    return linux_time;
}

time_t
UTL_get_time( double gps_tow,
              s32    wn_lng,
              s32    leap_seconds)
{
    UTL_gps_time_type  gps_time;
    double             temp_tow;
    date_time_t32      time_sec;

    temp_tow     = gps_tow - leap_seconds;
    gps_time.tow = MTH_round_d( &temp_tow );
    gps_time.wn  = wn_lng;
    UTL_cnvt_gps_time( &time_sec, &gps_time);

    return HWM_convert_rtc(time_sec);
}

