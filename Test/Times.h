#ifndef _HKY_TIMES_H_INCLUDE_
#define _HKY_TIMES_H_INCLUDE_

#include "Config.h"

typedef struct {
	time_t      sec;
	hky_uint_t  msec;
	hky_int_t   gmtoff;
} hky_time_t;


void hky_time_init(void);
void hky_time_update(void);
void hky_time_sigsafe_update(void);
u_char *hky_http_time(u_char *buf, time_t t);
u_char *hky_http_cookie_time(u_char *buf, time_t t);
void hky_gmtime(time_t t, hky_tm_t *tp);

time_t hky_next_time(time_t when);
#define hky_next_time_n      "mktime()"


extern volatile hky_time_t  *hky_cached_time;

#define hky_time()           hky_cached_time->sec
#define hky_timeofday()      (hky_time_t *) hky_cached_time

extern volatile hky_str_t    hky_cached_err_log_time;
extern volatile hky_str_t    hky_cached_http_time;
extern volatile hky_str_t    hky_cached_http_log_time;
extern volatile hky_str_t    hky_cached_http_log_iso8601;
extern volatile hky_str_t    hky_cached_syslog_time;


extern volatile hky_msec_t  hky_current_msec;

#endif