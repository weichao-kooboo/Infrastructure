#ifndef _HKY_PARSE_H_INCLUDE_
#define _HKY_PARSE_H_INCLUDE_

#include "Config.h"

ssize_t hky_parse_size(hky_str_t *line);
off_t hky_parse_offset(hky_str_t *line);
hky_int_t hky_parse_time(hky_str_t *line, hky_uint_t is_sec);

#endif