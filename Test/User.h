#ifndef _HKY_USER_H_INCLUDE_
#define _HKY_USER_H_INCLUDE_
#include "Config.h"

#define hky_uid_t  hky_int_t
#define hky_gid_t  hky_int_t

hky_int_t hky_libc_crypt(hky_pool_t *pool, u_char *key, u_char *salt,
	u_char **encrypted);

#endif // !_HKY_USER_H_INCLUDE_
