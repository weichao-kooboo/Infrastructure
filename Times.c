#include "Config.h"

static hky_msec_t hky_monotonic_time(time_t sec, hky_uint_t msec);

#define HKY_TIME_SLOTS   64

static hky_uint_t        slot;
static hky_atomic_t      hky_time_lock;

volatile hky_msec_t      hky_current_msec;
volatile hky_time_t     *hky_cached_time;
volatile hky_str_t       hky_cached_err_log_time;
volatile hky_str_t       hky_cached_http_time;
volatile hky_str_t       hky_cached_http_log_time;
volatile hky_str_t       hky_cached_http_log_iso8601;
volatile hky_str_t       hky_cached_syslog_time;

#if !(HKY_WIN32)

static hky_int_t         cached_gmtoff;
#endif

static hky_time_t        cached_time[HKY_TIME_SLOTS];
static u_char            cached_err_log_time[HKY_TIME_SLOTS]
[sizeof("1970/09/28 12:00:00")];
static u_char            cached_http_time[HKY_TIME_SLOTS]
[sizeof("Mon, 28 Sep 1970 06:00:00 GMT")];
static u_char            cached_http_log_time[HKY_TIME_SLOTS]
[sizeof("28/Sep/1970:12:00:00 +0600")];
static u_char            cached_http_log_iso8601[HKY_TIME_SLOTS]
[sizeof("1970-09-28T12:00:00+06:00")];
static u_char            cached_syslog_time[HKY_TIME_SLOTS]
[sizeof("Sep 28 12:00:00")];


static char  *week[] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
static char  *months[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun",
"Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };

void
hky_time_init(void)
{
	hky_cached_err_log_time.len = sizeof("1970/09/28 12:00:00") - 1;
	hky_cached_http_time.len = sizeof("Mon, 28 Sep 1970 06:00:00 GMT") - 1;
	hky_cached_http_log_time.len = sizeof("28/Sep/1970:12:00:00 +0600") - 1;
	hky_cached_http_log_iso8601.len = sizeof("1970-09-28T12:00:00+06:00") - 1;
	hky_cached_syslog_time.len = sizeof("Sep 28 12:00:00") - 1;

	hky_cached_time = &cached_time[0];

	hky_time_update();
}


void
hky_time_update(void)
{
	u_char          *p0, *p1, *p2, *p3, *p4;
	hky_tm_t         tm, gmt;
	time_t           sec;
	hky_uint_t       msec;
	hky_time_t      *tp;
	struct timeval   tv;

	if (!hky_trylock(&hky_time_lock)) {
		return;
	}

	hky_gettimeofday(&tv);

	sec = tv.tv_sec;
	msec = tv.tv_usec / 1000;

	hky_current_msec = hky_monotonic_time(sec, msec);

	tp = &cached_time[slot];

	if (tp->sec == sec) {
		tp->msec = msec;
		hky_unlock(&hky_time_lock);
		return;
	}

	if (slot == HKY_TIME_SLOTS - 1) {
		slot = 0;
	}
	else {
		slot++;
	}

	tp = &cached_time[slot];

	tp->sec = sec;
	tp->msec = msec;

	hky_gmtime(sec, &gmt);


	p0 = &cached_http_time[slot][0];

	(void)hky_sprintf(p0, "%s, %02d %s %4d %02d:%02d:%02d GMT",
		week[gmt.hky_tm_wday], gmt.hky_tm_mday,
		months[gmt.hky_tm_mon - 1], gmt.hky_tm_year,
		gmt.hky_tm_hour, gmt.hky_tm_min, gmt.hky_tm_sec);

#if (HKY_HAVE_GETTIMEZONE)

	tp->gmtoff = hky_gettimezone();
	hky_gmtime(sec + tp->gmtoff * 60, &tm);

#elif (HKY_HAVE_GMTOFF)

	hky_localtime(sec, &tm);
	cached_gmtoff = (hky_int_t)(tm.hky_tm_gmtoff / 60);
	tp->gmtoff = cached_gmtoff;

#else

	hky_localtime(sec, &tm);
	cached_gmtoff = hky_timezone(tm.hky_tm_isdst);
	tp->gmtoff = cached_gmtoff;

#endif


	p1 = &cached_err_log_time[slot][0];

	(void)hky_sprintf(p1, "%4d/%02d/%02d %02d:%02d:%02d",
		tm.hky_tm_year, tm.hky_tm_mon,
		tm.hky_tm_mday, tm.hky_tm_hour,
		tm.hky_tm_min, tm.hky_tm_sec);


	p2 = &cached_http_log_time[slot][0];

	(void)hky_sprintf(p2, "%02d/%s/%d:%02d:%02d:%02d %c%02i%02i",
		tm.hky_tm_mday, months[tm.hky_tm_mon - 1],
		tm.hky_tm_year, tm.hky_tm_hour,
		tm.hky_tm_min, tm.hky_tm_sec,
		tp->gmtoff < 0 ? '-' : '+',
		hky_abs(tp->gmtoff / 60), hky_abs(tp->gmtoff % 60));

	p3 = &cached_http_log_iso8601[slot][0];

	(void)hky_sprintf(p3, "%4d-%02d-%02dT%02d:%02d:%02d%c%02i:%02i",
		tm.hky_tm_year, tm.hky_tm_mon,
		tm.hky_tm_mday, tm.hky_tm_hour,
		tm.hky_tm_min, tm.hky_tm_sec,
		tp->gmtoff < 0 ? '-' : '+',
		hky_abs(tp->gmtoff / 60), hky_abs(tp->gmtoff % 60));

	p4 = &cached_syslog_time[slot][0];

	(void)hky_sprintf(p4, "%s %2d %02d:%02d:%02d",
		months[tm.hky_tm_mon - 1], tm.hky_tm_mday,
		tm.hky_tm_hour, tm.hky_tm_min, tm.hky_tm_sec);

	hky_memory_barrier();

	hky_cached_time = tp;
	hky_cached_http_time.data = p0;
	hky_cached_err_log_time.data = p1;
	hky_cached_http_log_time.data = p2;
	hky_cached_http_log_iso8601.data = p3;
	hky_cached_syslog_time.data = p4;

	hky_unlock(&hky_time_lock);
}


static hky_msec_t
hky_monotonic_time(time_t sec, hky_uint_t msec)
{
#if (HKY_HAVE_CLOCK_MONOTONIC)
	struct timespec  ts;

#if defined(CLOCK_MONOTONIC_FAST)
	clock_gettime(CLOCK_MONOTONIC_FAST, &ts);

#elif defined(CLOCK_MONOTONIC_COARSE)
	clock_gettime(CLOCK_MONOTONIC_COARSE, &ts);

#else
	clock_gettime(CLOCK_MONOTONIC, &ts);
#endif

	sec = ts.tv_sec;
	msec = ts.tv_nsec / 1000000;

#endif

	return (hky_msec_t)sec * 1000 + msec;
}


#if !(HKY_WIN32)

void
hky_time_sigsafe_update(void)
{
	u_char          *p, *p2;
	hky_tm_t         tm;
	time_t           sec;
	hky_time_t      *tp;
	struct timeval   tv;

	if (!hky_trylock(&hky_time_lock)) {
		return;
	}

	hky_gettimeofday(&tv);

	sec = tv.tv_sec;

	tp = &cached_time[slot];

	if (tp->sec == sec) {
		hky_unlock(&hky_time_lock);
		return;
	}

	if (slot == HKY_TIME_SLOTS - 1) {
		slot = 0;
	}
	else {
		slot++;
	}

	tp = &cached_time[slot];

	tp->sec = 0;

	hky_gmtime(sec + cached_gmtoff * 60, &tm);

	p = &cached_err_log_time[slot][0];

	(void)hky_sprintf(p, "%4d/%02d/%02d %02d:%02d:%02d",
		tm.hky_tm_year, tm.hky_tm_mon,
		tm.hky_tm_mday, tm.hky_tm_hour,
		tm.hky_tm_min, tm.hky_tm_sec);

	p2 = &cached_syslog_time[slot][0];

	(void)hky_sprintf(p2, "%s %2d %02d:%02d:%02d",
		months[tm.hky_tm_mon - 1], tm.hky_tm_mday,
		tm.hky_tm_hour, tm.hky_tm_min, tm.hky_tm_sec);

	hky_memory_barrier();

	hky_cached_err_log_time.data = p;
	hky_cached_syslog_time.data = p2;

	hky_unlock(&hky_time_lock);
}

#endif


u_char *
hky_http_time(u_char *buf, time_t t)
{
	hky_tm_t  tm;

	hky_gmtime(t, &tm);

	return hky_sprintf(buf, "%s, %02d %s %4d %02d:%02d:%02d GMT",
		week[tm.hky_tm_wday],
		tm.hky_tm_mday,
		months[tm.hky_tm_mon - 1],
		tm.hky_tm_year,
		tm.hky_tm_hour,
		tm.hky_tm_min,
		tm.hky_tm_sec);
}


u_char *
hky_http_cookie_time(u_char *buf, time_t t)
{
	hky_tm_t  tm;

	hky_gmtime(t, &tm);

	/*
	* Netscape 3.x does not understand 4-digit years at all and
	* 2-digit years more than "37"
	*/

	return hky_sprintf(buf,
		(tm.hky_tm_year > 2037) ?
		"%s, %02d-%s-%d %02d:%02d:%02d GMT" :
		"%s, %02d-%s-%02d %02d:%02d:%02d GMT",
		week[tm.hky_tm_wday],
		tm.hky_tm_mday,
		months[tm.hky_tm_mon - 1],
		(tm.hky_tm_year > 2037) ? tm.hky_tm_year :
		tm.hky_tm_year % 100,
		tm.hky_tm_hour,
		tm.hky_tm_min,
		tm.hky_tm_sec);
}


void
hky_gmtime(time_t t, hky_tm_t *tp)
{
	hky_int_t   yday;
	hky_uint_t  sec, min, hour, mday, mon, year, wday, days, leap;

	/* the calculation is valid for positive time_t only */

	if (t < 0) {
		t = 0;
	}

	days = t / 86400;
	sec = t % 86400;

	/*
	* no more than 4 year digits supported,
	* truncate to December 31, 9999, 23:59:59
	*/

	if (days > 2932896) {
		days = 2932896;
		sec = 86399;
	}

	/* January 1, 1970 was Thursday */

	wday = (4 + days) % 7;

	hour = sec / 3600;
	sec %= 3600;
	min = sec / 60;
	sec %= 60;

	/*
	* the algorithm based on Gauss' formula,
	* see src/core/hky_parse_time.c
	*/

	/* days since March 1, 1 BC */
	days = days - (31 + 28) + 719527;

	/*
	* The "days" should be adjusted to 1 only, however, some March 1st's go
	* to previous year, so we adjust them to 2.  This causes also shift of the
	* last February days to next year, but we catch the case when "yday"
	* becomes negative.
	*/

	year = (days + 2) * 400 / (365 * 400 + 100 - 4 + 1);

	yday = days - (365 * year + year / 4 - year / 100 + year / 400);

	if (yday < 0) {
		leap = (year % 4 == 0) && (year % 100 || (year % 400 == 0));
		yday = 365 + leap + yday;
		year--;
	}

	/*
	* The empirical formula that maps "yday" to month.
	* There are at least 10 variants, some of them are:
	*     mon = (yday + 31) * 15 / 459
	*     mon = (yday + 31) * 17 / 520
	*     mon = (yday + 31) * 20 / 612
	*/

	mon = (yday + 31) * 10 / 306;

	/* the Gauss' formula that evaluates days before the month */

	mday = yday - (367 * mon / 12 - 30) + 1;

	if (yday >= 306) {

		year++;
		mon -= 10;

		/*
		* there is no "yday" in Win32 SYSTEMTIME
		*
		* yday -= 306;
		*/

	}
	else {

		mon += 2;

		/*
		* there is no "yday" in Win32 SYSTEMTIME
		*
		* yday += 31 + 28 + leap;
		*/
	}

	tp->hky_tm_sec = (hky_tm_sec_t)sec;
	tp->hky_tm_min = (hky_tm_min_t)min;
	tp->hky_tm_hour = (hky_tm_hour_t)hour;
	tp->hky_tm_mday = (hky_tm_mday_t)mday;
	tp->hky_tm_mon = (hky_tm_mon_t)mon;
	tp->hky_tm_year = (hky_tm_year_t)year;
	tp->hky_tm_wday = (hky_tm_wday_t)wday;
}


time_t
hky_next_time(time_t when)
{
	time_t     now, next;
	struct tm  tm;

	now = hky_time();

	hky_libc_localtime(now, &tm);

	tm.tm_hour = (int)(when / 3600);
	when %= 3600;
	tm.tm_min = (int)(when / 60);
	tm.tm_sec = (int)(when % 60);

	next = mktime(&tm);

	if (next == -1) {
		return -1;
	}

	if (next - now > 0) {
		return next;
	}

	tm.tm_mday++;

	/* mktime() should normalize a date (Jan 32, etc) */

	next = mktime(&tm);

	if (next != -1) {
		return next;
	}

	return -1;
}
