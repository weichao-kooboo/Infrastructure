
#ifndef _HKY_ALLOC_H_INCLUDE_
#define _HKY_ALLOC_H_INCLUDE_

#include "../Config.h"

void *hky_alloc(size_t size, hky_log_t *log);
void *hky_calloc(size_t size, hky_log_t *log);

#define hky_free          free
#define hky_memalign(alignment, size, log)  hky_alloc(size, log)

extern hky_uint_t  hky_pagesize;
extern hky_uint_t  hky_pagesize_shift;
extern hky_uint_t  hky_cacheline_size;

#endif // !_HKY_ALLOC_H_INCLUDE_
