#ifndef _HKY_BUF_H_INCLUDE_
#define _HKY_BUF_H_INCLUDE_

#include "Config.h"

typedef void *            hky_buf_tag_t;

typedef struct hky_buf_s  hky_buf_t;

struct hky_buf_s {
	u_char          *pos;
	u_char          *last;
	off_t            file_pos;
	off_t            file_last;

	u_char          *start;         /* start of buffer */
	u_char          *end;           /* end of buffer */
	hky_buf_tag_t    tag;
	hky_file_t      *file;
	hky_buf_t       *shadow;


	/* the buf's content could be changed */
	unsigned         temporary : 1;

	/*
	* the buf's content is in a memory cache or in a read only memory
	* and must not be changed
	*/
	unsigned         memory : 1;

	/* the buf's content is mmap()ed and must not be changed */
	unsigned         mmap : 1;

	unsigned         recycled : 1;
	unsigned         in_file : 1;
	unsigned         flush : 1;
	unsigned         sync : 1;
	unsigned         last_buf : 1;
	unsigned         last_in_chain : 1;

	unsigned         last_shadow : 1;
	unsigned         temp_file : 1;

	/* STUB */ int   num;
};


struct hky_chain_s {
	hky_buf_t    *buf;
	hky_chain_t  *next;
};


typedef struct {
	hky_int_t    num;
	size_t       size;
} hky_bufs_t;


typedef struct hky_output_chain_ctx_s  hky_output_chain_ctx_t;

typedef hky_int_t(*hky_output_chain_filter_pt)(void *ctx, hky_chain_t *in);

typedef void(*hky_output_chain_aio_pt)(hky_output_chain_ctx_t *ctx,
	hky_file_t *file);

struct hky_output_chain_ctx_s {
	hky_buf_t                   *buf;
	hky_chain_t                 *in;
	hky_chain_t                 *free;
	hky_chain_t                 *busy;

	unsigned                     sendfile : 1;
	unsigned                     directio : 1;
	unsigned                     unaligned : 1;
	unsigned                     need_in_memory : 1;
	unsigned                     need_in_temp : 1;
	unsigned                     aio : 1;

#if (HKY_HAVE_FILE_AIO || HKY_COMPAT)
	hky_output_chain_aio_pt      aio_handler;
#if (HKY_HAVE_AIO_SENDFILE || HKY_COMPAT)
	ssize_t(*aio_preload)(hky_buf_t *file);
#endif
#endif

#if (HKY_THREADS || HKY_COMPAT)
	hky_int_t(*thread_handler)(hky_thread_task_t *task,
		hky_file_t *file);
	hky_thread_task_t           *thread_task;
#endif

	off_t                        alignment;

	hky_pool_t                  *pool;
	hky_int_t                    allocated;
	hky_bufs_t                   bufs;
	hky_buf_tag_t                tag;

	hky_output_chain_filter_pt   output_filter;
	void                        *filter_ctx;
};


typedef struct {
	hky_chain_t                 *out;
	hky_chain_t                **last;
	/*TODO*/
	//hky_connection_t            *connection;
	hky_pool_t                  *pool;
	off_t                        limit;
} hky_chain_writer_ctx_t;


#define HKY_CHAIN_ERROR     (hky_chain_t *) HKY_ERROR


#define hky_buf_in_memory(b)        (b->temporary || b->memory || b->mmap)
#define hky_buf_in_memory_only(b)   (hky_buf_in_memory(b) && !b->in_file)

#define hky_buf_special(b)                                                   \
    ((b->flush || b->last_buf || b->sync)                                    \
     && !hky_buf_in_memory(b) && !b->in_file)

#define hky_buf_sync_only(b)                                                 \
    (b->sync                                                                 \
     && !hky_buf_in_memory(b) && !b->in_file && !b->flush && !b->last_buf)

#define hky_buf_size(b)                                                      \
    (hky_buf_in_memory(b) ? (off_t) (b->last - b->pos):                      \
                            (b->file_last - b->file_pos))

hky_buf_t *hky_create_temp_buf(hky_pool_t *pool, size_t size);
hky_chain_t *hky_create_chain_of_bufs(hky_pool_t *pool, hky_bufs_t *bufs);


#define hky_alloc_buf(pool)  hky_palloc(pool, sizeof(hky_buf_t))
#define hky_calloc_buf(pool) hky_pcalloc(pool, sizeof(hky_buf_t))

hky_chain_t *hky_alloc_chain_link(hky_pool_t *pool);
#define hky_free_chain(pool, cl)                                             \
    cl->next = pool->chain;                                                  \
    pool->chain = cl



hky_int_t hky_output_chain(hky_output_chain_ctx_t *ctx, hky_chain_t *in);
hky_int_t hky_chain_writer(void *ctx, hky_chain_t *in);

hky_int_t hky_chain_add_copy(hky_pool_t *pool, hky_chain_t **chain,
	hky_chain_t *in);
hky_chain_t *hky_chain_get_free_buf(hky_pool_t *p, hky_chain_t **free);
void hky_chain_update_chains(hky_pool_t *p, hky_chain_t **free,
	hky_chain_t **busy, hky_chain_t **out, hky_buf_tag_t tag);

off_t hky_chain_coalesce_file(hky_chain_t **in, off_t limit);

hky_chain_t *hky_chain_update_sent(hky_chain_t *in, off_t sent);

#endif // !_HKY_BUF_H_INCLUDE_
