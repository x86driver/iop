#ifndef _UTL_FUNC_H
#define _UTL_FUNC_H

#include <time.h>
#include "mt3329_types.h"

time_t
UTL_get_time( double gps_tow,
              s32    wn_lng,
              s32    leap_seconds);

#endif

