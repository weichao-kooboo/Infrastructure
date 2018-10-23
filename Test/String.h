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
u_char *hky_pstrdup(hky_pool_t *pool, hky_str_t *src);
u_char * hky_cdecl hky_sprintf(u_char *buf, const char *fmt, ...);
u_char * hky_cdecl hky_snprintf(u_char *buf, size_t max, const char *fmt, ...);
u_char * hky_cdecl hky_slprintf(u_char *buf, u_char *last, const char *fmt,
	...);

u_char *hky_vslprintf(u_char *buf, u_char *last, const char *fmt, va_list args);
#define hky_vsnprintf(buf, max, fmt, args)                                   \
    hky_vslprintf(buf, buf + (max), fmt, args)

hky_int_t hky_strcasecmp(u_char *s1, u_char *s2);
hky_int_t hky_strncasecmp(u_char *s1, u_char *s2, size_t n);

u_char *hky_strnstr(u_char *s1, char *s2, size_t n);

u_char *hky_strstrn(u_char *s1, char *s2, size_t n);
u_char *hky_strcasestrn(u_char *s1, char *s2, size_t n);
u_char *hky_strlcasestrn(u_char *s1, u_char *last, u_char *s2, size_t n);

hky_int_t hky_rstrncmp(u_char *s1, u_char *s2, size_t n);
hky_int_t hky_rstrncasecmp(u_char *s1, u_char *s2, size_t n);
hky_int_t hky_memn2cmp(u_char *s1, u_char *s2, size_t n1, size_t n2);
hky_int_t hky_dns_strcmp(u_char *s1, u_char *s2);
hky_int_t hky_filename_cmp(u_char *s1, u_char *s2, size_t n);

hky_int_t hky_atoi(u_char *line, size_t n);
hky_int_t hky_atofp(u_char *line, size_t n, size_t point);
ssize_t hky_atosz(u_char *line, size_t n);
off_t hky_atoof(u_char *line, size_t n);
time_t hky_atotm(u_char *line, size_t n);
hky_int_t hky_hextoi(u_char *line, size_t n);

u_char *hky_hex_dump(u_char *dst, u_char *src, size_t len);

#define hky_base64_encoded_length(len)  (((len + 2) / 3) * 4)
#define hky_base64_decoded_length(len)  (((len + 3) / 4) * 3)

void hky_encode_base64(hky_str_t *dst, hky_str_t *src);
void hky_encode_base64url(hky_str_t *dst, hky_str_t *src);
hky_int_t hky_decode_base64(hky_str_t *dst, hky_str_t *src);
hky_int_t hky_decode_base64url(hky_str_t *dst, hky_str_t *src);

uint32_t hky_utf8_decode(u_char **p, size_t n);
size_t hky_utf8_length(u_char *p, size_t n);
u_char *hky_utf8_cpystrn(u_char *dst, u_char *src, size_t n, size_t len);


#define HKY_ESCAPE_URI            0
#define HKY_ESCAPE_ARGS           1
#define HKY_ESCAPE_URI_COMPONENT  2
#define HKY_ESCAPE_HTML           3
#define HKY_ESCAPE_REFRESH        4
#define HKY_ESCAPE_MEMCACHED      5
#define HKY_ESCAPE_MAIL_AUTH      6

#define HKY_UNESCAPE_URI       1
#define HKY_UNESCAPE_REDIRECT  2

uintptr_t hky_escape_uri(u_char *dst, u_char *src, size_t size,
	hky_uint_t type);
void hky_unescape_uri(u_char **dst, u_char **src, size_t size, hky_uint_t type);
uintptr_t hky_escape_html(u_char *dst, u_char *src, size_t size);
uintptr_t hky_escape_json(u_char *dst, u_char *src, size_t size);


typedef struct {
	hky_rbtree_node_t         node;
	hky_str_t                 str;
} hky_str_node_t;


void hky_str_rbtree_insert_value(hky_rbtree_node_t *temp,
	hky_rbtree_node_t *node, hky_rbtree_node_t *sentinel);
hky_str_node_t *hky_str_rbtree_lookup(hky_rbtree_t *rbtree, hky_str_t *name,
	uint32_t hash);


void hky_sort(void *base, size_t n, size_t size,
	hky_int_t(*cmp)(const void *, const void *));
#define hky_qsort             qsort


#define hky_value_helper(n)   #n
#define hky_value(n)          hky_value_helper(n)

#endif