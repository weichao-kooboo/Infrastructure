#include "Config.h"


hky_array_t *
hky_array_create(hky_pool_t *p, hky_uint_t n, size_t size)
{
	hky_array_t *a;

	a = hky_palloc(p, sizeof(hky_array_t));
	if (a == NULL) {
		return NULL;
	}

	if (hky_array_init(a, p, n, size) != HKY_OK) {
		return NULL;
	}

	return a;
}


void
hky_array_destroy(hky_array_t *a)
{
	hky_pool_t  *p;

	p = a->pool;

	if ((u_char *)a->elts + a->size * a->nalloc == p->d.last) {
		p->d.last -= a->size * a->nalloc;
	}

	if ((u_char *)a + sizeof(hky_array_t) == p->d.last) {
		p->d.last = (u_char *)a;
	}
}


void *
hky_array_push(hky_array_t *a)
{
	void        *elt, *new;
	size_t       size;
	hky_pool_t  *p;

	if (a->nelts == a->nalloc) {

		/* the array is full */

		size = a->size * a->nalloc;

		p = a->pool;

		if ((u_char *)a->elts + size == p->d.last
			&& p->d.last + a->size <= p->d.end)
		{
			/*
			* the array allocation is the last in the pool
			* and there is space for new allocation
			*/

			p->d.last += a->size;
			a->nalloc++;

		}
		else {
			/* allocate a new array */

			new = hky_palloc(p, 2 * size);
			if (new == NULL) {
				return NULL;
			}

			hky_memcpy(new, a->elts, size);
			a->elts = new;
			a->nalloc *= 2;
		}
	}

	elt = (u_char *)a->elts + a->size * a->nelts;
	a->nelts++;

	return elt;
}


void *
hky_array_push_n(hky_array_t *a, hky_uint_t n)
{
	void        *elt, *new;
	size_t       size;
	hky_uint_t   nalloc;
	hky_pool_t  *p;

	size = n * a->size;

	if (a->nelts + n > a->nalloc) {

		/* the array is full */

		p = a->pool;

		if ((u_char *)a->elts + a->size * a->nalloc == p->d.last
			&& p->d.last + size <= p->d.end)
		{
			/*
			* the array allocation is the last in the pool
			* and there is space for new allocation
			*/

			p->d.last += size;
			a->nalloc += n;

		}
		else {
			/* allocate a new array */

			nalloc = 2 * ((n >= a->nalloc) ? n : a->nalloc);

			new = hky_palloc(p, nalloc * a->size);
			if (new == NULL) {
				return NULL;
			}

			hky_memcpy(new, a->elts, a->nelts * a->size);
			a->elts = new;
			a->nalloc = nalloc;
		}
	}

	elt = (u_char *)a->elts + a->size * a->nelts;
	a->nelts += n;

	return elt;
}
