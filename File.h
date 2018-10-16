#ifndef _HKY_FILE_H_INCLUDE_
#define _HKY_FILE_H_INCLUDE_

#include "Config.h"

struct hky_file_s {
	hky_fd_t                   fd;
	hky_str_t                  name;
	hky_file_info_t            info;

	off_t                      offset;
	off_t                      sys_offset;

	hky_log_t                 *log;

#if (HKY_THREADS || HKY_COMPAT)
	hky_int_t(*thread_handler)(hky_thread_task_t *task,
		hky_file_t *file);
	void                      *thread_ctx;
	hky_thread_task_t         *thread_task;
#endif

#if (HKY_HAVE_FILE_AIO || HKY_COMPAT)
	hky_event_aio_t           *aio;
#endif

	unsigned                   valid_info : 1;
	unsigned                   directio : 1;
};


#define HKY_MAX_PATH_LEVEL  3


typedef hky_msec_t(*hky_path_manager_pt) (void *data);
typedef hky_msec_t(*hky_path_purger_pt) (void *data);
typedef void(*hky_path_loader_pt) (void *data);


typedef struct {
	hky_str_t                  name;
	size_t                     len;
	size_t                     level[HKY_MAX_PATH_LEVEL];

	hky_path_manager_pt        manager;
	hky_path_purger_pt         purger;
	hky_path_loader_pt         loader;
	void                      *data;

	u_char                    *conf_file;
	hky_uint_t                 line;
} hky_path_t;


typedef struct {
	hky_str_t                  name;
	size_t                     level[HKY_MAX_PATH_LEVEL];
} hky_path_init_t;


typedef struct {
	hky_file_t                 file;
	off_t                      offset;
	hky_path_t                *path;
	hky_pool_t                *pool;
	char                      *warn;

	hky_uint_t                 access;

	unsigned                   log_level : 8;
	unsigned                   persistent : 1;
	unsigned                   clean : 1;
	unsigned                   thread_write : 1;
} hky_temp_file_t;


typedef struct {
	hky_uint_t                 access;
	hky_uint_t                 path_access;
	time_t                     time;
	hky_fd_t                   fd;

	unsigned                   create_path : 1;
	unsigned                   delete_file : 1;

	hky_log_t                 *log;
} hky_ext_rename_file_t;


typedef struct {
	off_t                      size;
	size_t                     buf_size;

	hky_uint_t                 access;
	time_t                     time;

	hky_log_t                 *log;
} hky_copy_file_t;


typedef struct hky_tree_ctx_s  hky_tree_ctx_t;

typedef hky_int_t(*hky_tree_init_handler_pt) (void *ctx, void *prev);
typedef hky_int_t(*hky_tree_handler_pt) (hky_tree_ctx_t *ctx, hky_str_t *name);

struct hky_tree_ctx_s {
	off_t                      size;
	off_t                      fs_size;
	hky_uint_t                 access;
	time_t                     mtime;

	hky_tree_init_handler_pt   init_handler;
	hky_tree_handler_pt        file_handler;
	hky_tree_handler_pt        pre_tree_handler;
	hky_tree_handler_pt        post_tree_handler;
	hky_tree_handler_pt        spec_handler;

	void                      *data;
	size_t                     alloc;

	hky_log_t                 *log;
};
//
//hky_int_t hky_get_full_name(hky_pool_t *pool, hky_str_t *prefix,
//	hky_str_t *name);
//
//ssize_t hky_write_chain_to_temp_file(hky_temp_file_t *tf, hky_chain_t *chain);
//hky_int_t hky_create_temp_file(hky_file_t *file, hky_path_t *path,
//	hky_pool_t *pool, hky_uint_t persistent, hky_uint_t clean,
//	hky_uint_t access);
//void hky_create_hashed_filename(hky_path_t *path, u_char *file, size_t len);
//hky_int_t hky_create_path(hky_file_t *file, hky_path_t *path);
//hky_err_t hky_create_full_path(u_char *dir, hky_uint_t access);
//hky_int_t hky_add_path(hky_conf_t *cf, hky_path_t **slot);
//hky_int_t hky_create_paths(hky_cycle_t *cycle, hky_uid_t user);
//hky_int_t hky_ext_rename_file(hky_str_t *src, hky_str_t *to,
//	hky_ext_rename_file_t *ext);
//hky_int_t hky_copy_file(u_char *from, u_char *to, hky_copy_file_t *cf);
//hky_int_t hky_walk_tree(hky_tree_ctx_t *ctx, hky_str_t *tree);
//
//hky_atomic_uint_t hky_next_temp_number(hky_uint_t collision);
//
//char *hky_conf_set_path_slot(hky_conf_t *cf, hky_command_t *cmd, void *conf);
//char *hky_conf_merge_path_value(hky_conf_t *cf, hky_path_t **path,
//	hky_path_t *prev, hky_path_init_t *init);
//char *hky_conf_set_access_slot(hky_conf_t *cf, hky_command_t *cmd, void *conf);


extern hky_atomic_t      *hky_temp_number;
extern hky_atomic_int_t   hky_random_number;

#endif // !_HKY_FILE_H_INCLUDE_
