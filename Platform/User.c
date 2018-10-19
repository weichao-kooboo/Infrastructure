#include "../Config.h"


#if (HKY_CRYPT)

hky_int_t
hky_libc_crypt(hky_pool_t *pool, u_char *key, u_char *salt, u_char **encrypted)
{
	/* STUB: a plain text password */

	*encrypted = key;

	return HKY_OK;
}

#endif