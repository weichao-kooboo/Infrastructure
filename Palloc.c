#include "Config.h"

static hky_inline void *hky_palloc_small(hky_pool_t *pool, size_t size,
	hky_uint_t align);
static void *hky_palloc_block(hky_pool_t *pool, size_t size);
static void *hky_palloc_large(hky_pool_t *pool, size_t size);



hky_pool_t *
hky_create_pool(size_t size, hky_log_t *log)
{
	hky_pool_t  *p;

	p = hky_memalign(HKY_POOL_ALIGNMENT, size, log);
	if (p == NULL) {
		return NULL;
	}

	p->d.last = (u_char *)p + sizeof(hky_pool_t);
	p->d.end = (u_char *)p + size;
	p->d.next = NULL;
	p->d.failed = 0;

	size = size - sizeof(hky_pool_t);
	p->max = (size < HKY_MAX_ALLOC_FROM_POOL) ? size : HKY_MAX_ALLOC_FROM_POOL;

	p->current = p;
	p->chain = NULL;
	p->large = NULL;
	p->cleanup = NULL;
	p->log = log;

	return p;
}

void
hky_destroy_pool(hky_pool_t *pool)
{
	hky_pool_t          *p, *n;
	hky_pool_large_t    *l;
	hky_pool_cleanup_t  *c;

	for (c = pool->cleanup; c; c = c->next) {
		if (c->handler) {
			hky_log_debug1(HKY_LOG_DEBUG_ALLOC, pool->log, 0,
				"run cleanup: %p", c);
			c->handler(c->data);
		}
	}

#if (HKY_DEBUG)

	/*
	* we could allocate the pool->log from this pool
	* so we cannot use this log while free()ing the pool
	*/

	for (l = pool->large; l; l = l->next) {
		hky_log_debug1(HKY_LOG_DEBUG_ALLOC, pool->log, 0, "free: %p", l->alloc);
	}

	for (p = pool, n = pool->d.next; /* void */; p = n, n = n->d.next) {
		hky_log_debug2(HKY_LOG_DEBUG_ALLOC, pool->log, 0,
			"free: %p, unused: %uz", p, p->d.end - p->d.last);

		if (n == NULL) {
			break;
		}
	}

#endif

	for (l = pool->large; l; l = l->next) {
		if (l->alloc) {
			hky_free(l->alloc);
		}
	}

	for (p = pool, n = pool->d.next; /* void */; p = n, n = n->d.next) {
		hky_free(p);

		if (n == NULL) {
			break;
		}
	}
}


void
hky_reset_pool(hky_pool_t *pool)
{
	hky_pool_t        *p;
	hky_pool_large_t  *l;

	for (l = pool->large; l; l = l->next) {
		if (l->alloc) {
			hky_free(l->alloc);
		}
	}

	for (p = pool; p; p = p->d.next) {
		p->d.last = (u_char *)p + sizeof(hky_pool_t);
		p->d.failed = 0;
	}

	pool->current = pool;
	pool->chain = NULL;
	pool->large = NULL;
}


void *
hky_palloc(hky_pool_t *pool, size_t size)
{
#if !(HKY_DEBUG_PALLOC)
	if (size <= pool->max) {
		return hky_palloc_small(pool, size, 1);
	}
#endif

	return hky_palloc_large(pool, size);
}


void *
hky_pnalloc(hky_pool_t *pool, size_t size)
{
#if !(HKY_DEBUG_PALLOC)
	if (size <= pool->max) {
		return hky_palloc_small(pool, size, 0);
	}
#endif

	return hky_palloc_large(pool, size);
}


void *
hky_pmemalign(hky_pool_t *pool, size_t size, size_t alignment)
{
	void              *p;
	hky_pool_large_t  *large;

	p = hky_memalign(alignment, size, pool->log);
	if (p == NULL) {
		return NULL;
	}

	large = hky_palloc_small(pool, sizeof(hky_pool_large_t), 1);
	if (large == NULL) {
		hky_free(p);
		return NULL;
	}

	large->alloc = p;
	large->next = pool->large;
	pool->large = large;

	return p;
}


hky_int_t
hky_pfree(hky_pool_t *pool, void *p)
{
	hky_pool_large_t  *l;

	for (l = pool->large; l; l = l->next) {
		if (p == l->alloc) {
			hky_log_debug1(HKY_LOG_DEBUG_ALLOC, pool->log, 0,
				"free: %p", l->alloc);
			hky_free(l->alloc);
			l->alloc = NULL;

			return HKY_OK;
		}
	}

	return HKY_DECLINED;
}


void *
hky_pcalloc(hky_pool_t *pool, size_t size)
{
	void *p;

	p = hky_palloc(pool, size);
	if (p) {
		hky_memzero(p, size);
	}

	return p;
}


hky_pool_cleanup_t *
hky_pool_cleanup_add(hky_pool_t *p, size_t size)
{
	hky_pool_cleanup_t  *c;

	c = hky_palloc(p, sizeof(hky_pool_cleanup_t));
	if (c == NULL) {
		return NULL;
	}

	if (size) {
		c->data = hky_palloc(p, size);
		if (c->data == NULL) {
			return NULL;
		}

	}
	else {
		c->data = NULL;
	}

	c->handler = NULL;
	c->next = p->cleanup;

	p->cleanup = c;

	hky_log_debug1(HKY_LOG_DEBUG_ALLOC, p->log, 0, "add cleanup: %p", c);

	return c;
}


void
hky_pool_run_cleanup_file(hky_pool_t *p, hky_fd_t fd)
{
	hky_pool_cleanup_t       *c;
	hky_pool_cleanup_file_t  *cf;

	for (c = p->cleanup; c; c = c->next) {
		if (c->handler == hky_pool_cleanup_file) {

			cf = c->data;

			if (cf->fd == fd) {
				c->handler(cf);
				c->handler = NULL;
				return;
			}
		}
	}
}


void
hky_pool_cleanup_file(void *data)
{
	hky_pool_cleanup_file_t  *c = data;

	hky_log_debug1(HKY_LOG_DEBUG_ALLOC, c->log, 0, "file cleanup: fd:%d",
		c->fd);

	if (hky_close_file(c->fd) == HKY_FILE_ERROR) {
		hky_log_error(HKY_LOG_ALERT, c->log, hky_errno,
			hky_close_file_n " \"%s\" failed", c->name);
	}
}


void
hky_pool_delete_file(void *data)
{
	hky_pool_cleanup_file_t  *c = data;

	hky_err_t  err;

	hky_log_debug2(HKY_LOG_DEBUG_ALLOC, c->log, 0, "file cleanup: fd:%d %s",
		c->fd, c->name);

	if (hky_delete_file(c->name) == HKY_FILE_ERROR) {
		err = hky_errno;

		if (err != HKY_ENOENT) {
			hky_log_error(HKY_LOG_CRIT, c->log, err,
				hky_delete_file_n " \"%s\" failed", c->name);
		}
	}

	if (hky_close_file(c->fd) == HKY_FILE_ERROR) {
		hky_log_error(HKY_LOG_ALERT, c->log, hky_errno,
			hky_close_file_n " \"%s\" failed", c->name);
	}
}


static hky_inline void *
hky_palloc_small(hky_pool_t *pool, size_t size, hky_uint_t align)
{
	u_char      *m;
	hky_pool_t  *p;

	p = pool->current;

	do {
		m = p->d.last;

		if (align) {
			m = hky_align_ptr(m, HKY_ALIGNMENT);
		}

		if ((size_t)(p->d.end - m) >= size) {
			p->d.last = m + size;

			return m;
		}

		p = p->d.next;

	} while (p);

	return hky_palloc_block(pool, size);
}


static void *
hky_palloc_block(hky_pool_t *pool, size_t size)
{
	u_char      *m;
	size_t       psize;
	hky_pool_t  *p, *new;

	psize = (size_t)(pool->d.end - (u_char *)pool);

	m = hky_memalign(HKY_POOL_ALIGNMENT, psize, pool->log);
	if (m == NULL) {
		return NULL;
	}

	new = (hky_pool_t *)m;

	new->d.end = m + psize;
	new->d.next = NULL;
	new->d.failed = 0;

	m += sizeof(hky_pool_data_t);
	m = hky_align_ptr(m, HKY_ALIGNMENT);
	new->d.last = m + size;

	for (p = pool->current; p->d.next; p = p->d.next) {
		if (p->d.failed++ > 4) {
			pool->current = p->d.next;
		}
	}

	p->d.next = new;

	return m;
}


static void *
hky_palloc_large(hky_pool_t *pool, size_t size)
{
	void              *p;
	hky_uint_t         n;
	hky_pool_large_t  *large;

	p = hky_alloc(size, pool->log);
	if (p == NULL) {
		return NULL;
	}

	n = 0;

	for (large = pool->large; large; large = large->next) {
		if (large->alloc == NULL) {
			large->alloc = p;
			return p;
		}

		if (n++ > 3) {
			break;
		}
	}

	large = hky_palloc_small(pool, sizeof(hky_pool_large_t), 1);
	if (large == NULL) {
		hky_free(p);
		return NULL;
	}

	large->alloc = p;
	large->next = pool->large;
	pool->large = large;

	return p;
}