#ifndef _HKY_STRING_H_INCLUDE_
#define _HKY_STRING_H_INCLUDE_

#include "Config.h"

typedef struct {
	size_t      len;
	u_char *data;
} hky_str_t;

typedef struct {
	hky_str_t   key;
	hky_str_t   value;
} hky_keyval_t;


typedef struct {
	unsigned    len : 28;

	unsigned    valid : 1;
	unsigned    no_cacheable : 1;
	unsigned    not_found : 1;
	unsigned    escape : 1;

	u_char     *data;
} hky_variable_value_t;


#define hky_string(str)     { sizeof(str) - 1, (u_char *) str }
#define hky_null_string     { 0, NULL }
#define hky_str_set(str, text)                                               \
(str)->len = sizeof(text) - 1; (str)->data = (u_char *) text
#define hky_str_null(str)   (str)->len = 0; (str)->data = NULL


#define hky_tolower(c)      (u_char) ((c >= 'A' && c <= 'Z') ? (c | 0x20) : c)
#define hky_toupper(c)      (u_char) ((c >= 'a' && c <= 'z') ? (c & ~0x20) : c)

void hky_strlow(u_char *dst, u_char *src, size_t n);


#define hky_strncmp(s1, s2, n)  strncmp((const char *) s1, (const char *) s2, n)


#define hky_strcmp(s1, s2)  strcmp((const char *) s1, (const char *) s2)


#define hky_strstr(s1, s2)  strstr((const char *) s1, (const char *) s2)
#define hky_strlen(s)       strlen((const char *) s)

size_t hky_strnlen(u_char *p, size_t n);

#define hky_strchr(s1, c)   strchr((const char *) s1, (int) c)

static hky_inline u_char *
hky_strlchr(u_char *p, u_char *last, u_char c)
{
	while (p < last) {

		if (*p == c) {
			return p;
		}

		p++;
	}

	return NULL;
}

#define hky_memzero(buf, n)       (void) memset(buf, 0, n)
#define hky_memset(buf, c, n)     (void) memset(buf, c, n)

#if (HKY_MEMCPY_LIMIT)

void *hky_memcpy(void *dst, const void *src, size_t n);
#define hky_cpymem(dst, src, n)   (((u_char *) hky_memcpy(dst, src, n)) + (n))

#else

/*
* gcc3, msvc, and icc7 compile memcpy() to the inline "rep movs".
* gcc3 compiles memcpy(d, s, 4) to the inline "mov"es.
* icc8 compile memcpy(d, s, 4) to the inline "mov"es or XMM moves.
*/
#define hky_memcpy(dst, src, n)   (void) memcpy(dst, src, n)
#define hky_cpymem(dst, src, n)   (((u_char *) memcpy(dst, src, n)) + (n))

#endif


#if ( __INTEL_COMPILER >= 800 )

/*
* the simple inline cycle copies the variable length strings up to 16
* bytes faster than icc8 autodetecting _intel_fast_memcpy()
*/

static hky_inline u_char *
hky_copy(u_char *dst, u_char *src, size_t len)
{
	if (len < 17) {

		while (len) {
			*dst++ = *src++;
			len--;
		}

		return dst;

	}
	else {
		return hky_cpymem(dst, src, len);
	}
}

#else

#define hky_copy                  hky_cpymem

#endif


#define hky_memmove(dst, src, n)   (void) memmove(dst, src, n)
#define hky_movemem(dst, src, n)   (((u_char *) memmove(dst, src, n)) + (n))


/* msvc and icc7 compile memcmp() to the inline loop */
#define hky_memcmp(s1, s2, n)  memcmp((const char *) s1, (const char *) s2, n)

u_char *hky_cpystrn(u_char *dst, u_char *src, size_t n);
/*
u_char *hky_pstrdup(hky_pool_t *pool, hky_str_t *src);
u_char * hky_cdecl hky_sprintf(u_char *buf, const char *fmt, ...);
u_char * hky_cdecl hky_snprintf(u_char *buf, size_t max, const char *fmt, ...);
u_char * hky_cdecl hky_slprintf(u_char *buf, u_char *last, const char *fmt,
	...);
	*/

#endif