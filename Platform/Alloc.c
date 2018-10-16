
#include "../Config.h"

hky_uint_t  hky_pagesize;
hky_uint_t  hky_pagesize_shift;
hky_uint_t  hky_cacheline_size;

void *hky_alloc(size_t size, hky_log_t *log)
{
	void  *p;

	p = malloc(size);
	if (p == NULL) {
		hky_log_error(HKY_LOG_EMERG, log, hky_errno,
			"malloc(%uz) failed", size);
	}

	hky_log_debug2(HKY_LOG_DEBUG_ALLOC, log, 0, "malloc: %p:%uz", p, size);

	return p;
}


void *hky_calloc(size_t size, hky_log_t *log)
{
	void  *p;

	p = hky_alloc(size, log);

	if (p) {
		hky_memzero(p, size);
	}

	return p;
}