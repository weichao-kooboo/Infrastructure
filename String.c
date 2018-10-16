#include "Config.h"
void
hky_strlow(u_char *dst, u_char *src, size_t n)
{
	while (n) {
		*dst = hky_tolower(*src);
		dst++;
		src++;
		n--;
	}
}


size_t
hky_strnlen(u_char *p, size_t n)
{
	size_t  i;

	for (i = 0; i < n; i++) {

		if (p[i] == '\0') {
			return i;
		}
	}

	return n;
}


#if (HKY_MEMCPY_LIMIT)

void *
hky_memcpy(void *dst, const void *src, size_t n)
{
	if (n > HKY_MEMCPY_LIMIT) {
		hky_log_error(HKY_LOG_ALERT, hky_cycle->log, 0, "memcpy %uz bytes", n);
		hky_debug_point();
	}

	return memcpy(dst, src, n);
}

#endif


u_char *
hky_cpystrn(u_char *dst, u_char *src, size_t n)
{
	if (n == 0) {
		return dst;
	}

	while (--n) {
		*dst = *src;

		if (*dst == '\0') {
			return dst;
		}

		dst++;
		src++;
	}

	*dst = '\0';

	return dst;
}