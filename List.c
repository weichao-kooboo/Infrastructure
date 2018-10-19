#include "Config.h"

hky_list_t *
hky_list_create(hky_pool_t *pool, hky_uint_t n, size_t size)
{
	hky_list_t  *list;

	list = hky_palloc(pool, sizeof(hky_list_t));
	if (list == NULL) {
		return NULL;
	}

	if (hky_list_init(list, pool, n, size) != HKY_OK) {
		return NULL;
	}

	return list;
}


void *
hky_list_push(hky_list_t *l)
{
	void             *elt;
	hky_list_part_t  *last;

	last = l->last;

	if (last->nelts == l->nalloc) {

		/* the last part is full, allocate a new list part */

		last = hky_palloc(l->pool, sizeof(hky_list_part_t));
		if (last == NULL) {
			return NULL;
		}

		last->elts = hky_palloc(l->pool, l->nalloc * l->size);
		if (last->elts == NULL) {
			return NULL;
		}

		last->nelts = 0;
		last->next = NULL;

		l->last->next = last;
		l->last = last;
	}

	elt = (char *)last->elts + l->size * last->nelts;
	last->nelts++;

	return elt;
}
