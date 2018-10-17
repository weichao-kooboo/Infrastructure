#ifndef _HKY_PALLOC_H_INCLUDE_
#define _HKY_PALLOC_H_INCLUDE_

#include "Config.h"

#define HKY_MAX_ALLOC_FROM_POOL  (hky_pagesize - 1)

#define HKY_DEFAULT_POOL_SIZE    (16 * 1024)

#define HKY_POOL_ALIGNMENT       16
#define HKY_MIN_POOL_SIZE                                                     \
    hky_align((sizeof(hky_pool_t) + 2 * sizeof(hky_pool_large_t)),            \
              HKY_POOL_ALIGNMENT)


typedef void(*hky_pool_cleanup_pt)(void *data);

typedef struct hky_pool_cleanup_s  hky_pool_cleanup_t;

struct hky_pool_cleanup_s {
	hky_pool_cleanup_pt   handler;
	void                 *data;
	hky_pool_cleanup_t   *next;
};


typedef struct hky_pool_large_s  hky_pool_large_t;

struct hky_pool_large_s {
	hky_pool_large_t     *next;
	void                 *alloc;
};


typedef struct {
	u_char               *last;
	u_char               *end;
	hky_pool_t           *next;
	hky_uint_t            failed;
} hky_pool_data_t;


struct hky_pool_s {
	hky_pool_data_t       d;
	size_t                max;
	hky_pool_t           *current;
	hky_chain_t          *chain;
	hky_pool_large_t     *large;
	hky_pool_cleanup_t   *cleanup;
	hky_log_t            *log;
};


typedef struct {
	hky_fd_t              fd;
	u_char               *name;
	hky_log_t            *log;
} hky_pool_cleanup_file_t;

hky_pool_t *hky_create_pool(size_t size, hky_log_t *log);
void hky_destroy_pool(hky_pool_t *pool);
void hky_reset_pool(hky_pool_t *pool);

void *hky_palloc(hky_pool_t *pool, size_t size);
void *hky_pnalloc(hky_pool_t *pool, size_t size);
void *hky_pcalloc(hky_pool_t *pool, size_t size);
void *hky_pmemalign(hky_pool_t *pool, size_t size, size_t alignment);
hky_int_t hky_pfree(hky_pool_t *pool, void *p);


hky_pool_cleanup_t *hky_pool_cleanup_add(hky_pool_t *p, size_t size);
void hky_pool_run_cleanup_file(hky_pool_t *p, hky_fd_t fd);
void hky_pool_cleanup_file(void *data);
void hky_pool_delete_file(void *data);


#endif