#ifndef _HKY_ARRAY_H_INCLUDE_
#define _HKY_ARRAY_H_INCLUDE_

#include "Config.h"

typedef struct {
	void        *elts;
	hky_uint_t   nelts;
	size_t       size;
	hky_uint_t   nalloc;
	hky_pool_t  *pool;
} hky_array_t;

hky_array_t *hky_array_create(hky_pool_t *p, hky_uint_t n, size_t size);
void hky_array_destroy(hky_array_t *a);
void *hky_array_push(hky_array_t *a);
void *hky_array_push_n(hky_array_t *a, hky_uint_t n);


static hky_inline hky_int_t
hky_array_init(hky_array_t *array, hky_pool_t *pool, hky_uint_t n, size_t size)
{
	/*
	* set "array->nelts" before "array->elts", otherwise MSVC thinks
	* that "array->nelts" may be used without having been initialized
	*/

	array->nelts = 0;
	array->size = size;
	array->nalloc = n;
	array->pool = pool;

	array->elts = hky_palloc(pool, n * size);
	if (array->elts == NULL) {
		return HKY_ERROR;
	}

	return HKY_OK;
}


#endif // !_HKY_ARRAY_H_INCLUDE_
