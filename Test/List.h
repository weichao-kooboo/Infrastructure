#ifndef _HKY_LIST_H_INCLUDE_
#define _HKY_LIST_H_INCLUDE_

#include "Config.h"

typedef struct hky_list_part_s  hky_list_part_t;

struct hky_list_part_s {
	void             *elts;
	hky_uint_t        nelts;
	hky_list_part_t  *next;
};


typedef struct {
	hky_list_part_t  *last;
	hky_list_part_t   part;
	size_t            size;
	hky_uint_t        nalloc;
	hky_pool_t       *pool;
} hky_list_t;


hky_list_t *hky_list_create(hky_pool_t *pool, hky_uint_t n, size_t size);

static hky_inline hky_int_t
hky_list_init(hky_list_t *list, hky_pool_t *pool, hky_uint_t n, size_t size)
{
	list->part.elts = hky_palloc(pool, n * size);
	if (list->part.elts == NULL) {
		return HKY_ERROR;
	}

	list->part.nelts = 0;
	list->part.next = NULL;
	list->last = &list->part;
	list->size = size;
	list->nalloc = n;
	list->pool = pool;

	return HKY_OK;
}

void *hky_list_push(hky_list_t *list);

#endif // !_HKY_LIST_H_INCLUDE_
