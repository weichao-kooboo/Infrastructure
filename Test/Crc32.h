#ifndef _HKY_CRC32_H_INCLUDE_
#define _HKY_CRC32_H_INCLUDE_

#include "Config.h"

extern uint32_t   hky_crc32_table256[];

static hky_inline uint32_t
hky_crc32_long(u_char *p, size_t len)
{
	uint32_t  crc;

	crc = 0xffffffff;

	while (len--) {
		crc = hky_crc32_table256[(crc ^ *p++) & 0xff] ^ (crc >> 8);
	}

	return crc ^ 0xffffffff;
}


#endif // !_HKY_CRC32_H_INCLUDE_
