
#ifndef _HKY_TIME_H_INCLUDE_
#define _HKY_TIME_H_INCLUDE_

#include "Config.h"


typedef hky_rbtree_key_t      hky_msec_t;
typedef hky_rbtree_key_int_t  hky_msec_int_t;

typedef SYSTEMTIME            hky_tm_t;
typedef FILETIME              hky_mtime_t;

#define hky_tm_sec            wSecond
#define hky_tm_min            wMinute
#define hky_tm_hour           wHour
#define hky_tm_mday           wDay
#define hky_tm_mon            wMonth
#define hky_tm_year           wYear
#define hky_tm_wday           wDayOfWeek

#define hky_tm_sec_t          u_short
#define hky_tm_min_t          u_short
#define hky_tm_hour_t         u_short
#define hky_tm_mday_t         u_short
#define hky_tm_mon_t          u_short
#define hky_tm_year_t         u_short
#define hky_tm_wday_t         u_short


#define hky_msleep            Sleep

#define HKY_HAVE_GETTIMEZONE  1

#define  hky_timezone_update()

hky_int_t hky_gettimezone(void);
void hky_libc_localtime(time_t s, struct tm *tm);
void hky_libc_gmtime(time_t s, struct tm *tm);
void hky_gettimeofday(struct timeval *tp);


#endif // !_HKY_TIME_H_INCLUDE_
