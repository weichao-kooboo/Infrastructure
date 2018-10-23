#ifndef _HKY_CONF_FILE_H_INCLUDE_
#define _HKY_CONF_FILE_H_INCLUDE_

#include "Config.h"

/*
*        AAAA  number of arguments
*      FF      command flags
*    TT        command type, i.e. HTTP "location" or "server" command
*/

#define HKY_CONF_NOARGS      0x00000001
#define HKY_CONF_TAKE1       0x00000002
#define HKY_CONF_TAKE2       0x00000004
#define HKY_CONF_TAKE3       0x00000008
#define HKY_CONF_TAKE4       0x00000010
#define HKY_CONF_TAKE5       0x00000020
#define HKY_CONF_TAKE6       0x00000040
#define HKY_CONF_TAKE7       0x00000080

#define HKY_CONF_MAX_ARGS    8

#define HKY_CONF_TAKE12      (HKY_CONF_TAKE1|HKY_CONF_TAKE2)
#define HKY_CONF_TAKE13      (HKY_CONF_TAKE1|HKY_CONF_TAKE3)

#define HKY_CONF_TAKE23      (HKY_CONF_TAKE2|HKY_CONF_TAKE3)

#define HKY_CONF_TAKE123     (HKY_CONF_TAKE1|HKY_CONF_TAKE2|HKY_CONF_TAKE3)
#define HKY_CONF_TAKE1234    (HKY_CONF_TAKE1|HKY_CONF_TAKE2|HKY_CONF_TAKE3   \
                              |HKY_CONF_TAKE4)

#define HKY_CONF_ARGS_NUMBER 0x000000ff
#define HKY_CONF_BLOCK       0x00000100
#define HKY_CONF_FLAG        0x00000200
#define HKY_CONF_ANY         0x00000400
#define HKY_CONF_1MORE       0x00000800
#define HKY_CONF_2MORE       0x00001000

#define HKY_DIRECT_CONF      0x00010000

#define HKY_MAIN_CONF        0x01000000
#define HKY_ANY_CONF         0x1F000000



#define HKY_CONF_UNSET       -1
#define HKY_CONF_UNSET_UINT  (hky_uint_t) -1
#define HKY_CONF_UNSET_PTR   (void *) -1
#define HKY_CONF_UNSET_SIZE  (size_t) -1
#define HKY_CONF_UNSET_MSEC  (hky_msec_t) -1


#define HKY_CONF_OK          NULL
#define HKY_CONF_ERROR       (void *) -1

#define HKY_CONF_BLOCK_START 1
#define HKY_CONF_BLOCK_DONE  2
#define HKY_CONF_FILE_DONE   3

#define HKY_CORE_MODULE      0x45524F43  /* "CORE" */
#define HKY_CONF_MODULE      0x464E4F43  /* "CONF" */


#define HKY_MAX_CONF_ERRSTR  1024


struct hky_command_s {
	hky_str_t             name;
	hky_uint_t            type;
	char               *(*set)(hky_conf_t *cf, hky_command_t *cmd, void *conf);
	hky_uint_t            conf;
	hky_uint_t            offset;
	void                 *post;
};

#define hky_null_command  { hky_null_string, 0, NULL, 0, 0, NULL }


struct hky_open_file_s {
	hky_fd_t              fd;
	hky_str_t             name;

	void(*flush)(hky_open_file_t *file, hky_log_t *log);
	void                 *data;
};


typedef struct {
	hky_file_t            file;
	hky_buf_t            *buffer;
	hky_buf_t            *dump;
	hky_uint_t            line;
} hky_conf_file_t;


typedef struct {
	hky_str_t             name;
	hky_buf_t            *buffer;
} hky_conf_dump_t;


typedef char *(*hky_conf_handler_pt)(hky_conf_t *cf,
	hky_command_t *dummy, void *conf);


struct hky_conf_s {
	char                 *name;
	hky_array_t          *args;

	hky_origin_t          *cycle;
	hky_pool_t           *pool;
	hky_pool_t           *temp_pool;
	hky_conf_file_t      *conf_file;
	hky_log_t            *log;

	void                 *ctx;
	hky_uint_t            module_type;
	hky_uint_t            cmd_type;

	hky_conf_handler_pt   handler;
	void                 *handler_conf;
};


typedef char *(*hky_conf_post_handler_pt) (hky_conf_t *cf,
	void *data, void *conf);

typedef struct {
	hky_conf_post_handler_pt  post_handler;
} hky_conf_post_t;


typedef struct {
	hky_conf_post_handler_pt  post_handler;
	char                     *old_name;
	char                     *new_name;
} hky_conf_deprecated_t;


typedef struct {
	hky_conf_post_handler_pt  post_handler;
	hky_int_t                 low;
	hky_int_t                 high;
} hky_conf_num_bounds_t;


typedef struct {
	hky_str_t                 name;
	hky_uint_t                value;
} hky_conf_enum_t;


#define HKY_CONF_BITMASK_SET  1

typedef struct {
	hky_str_t                 name;
	hky_uint_t                mask;
} hky_conf_bitmask_t;


char * hky_conf_deprecated(hky_conf_t *cf, void *post, void *data);
char *hky_conf_check_num_bounds(hky_conf_t *cf, void *post, void *data);


#define hky_get_conf(conf_ctx, module)  conf_ctx[module.index]



#define hky_conf_init_value(conf, default)                                   \
    if (conf == HKY_CONF_UNSET) {                                            \
        conf = default;                                                      \
    }

#define hky_conf_init_ptr_value(conf, default)                               \
    if (conf == HKY_CONF_UNSET_PTR) {                                        \
        conf = default;                                                      \
    }

#define hky_conf_init_uint_value(conf, default)                              \
    if (conf == HKY_CONF_UNSET_UINT) {                                       \
        conf = default;                                                      \
    }

#define hky_conf_init_size_value(conf, default)                              \
    if (conf == HKY_CONF_UNSET_SIZE) {                                       \
        conf = default;                                                      \
    }

#define hky_conf_init_msec_value(conf, default)                              \
    if (conf == HKY_CONF_UNSET_MSEC) {                                       \
        conf = default;                                                      \
    }

#define hky_conf_merge_value(conf, prev, default)                            \
    if (conf == HKY_CONF_UNSET) {                                            \
        conf = (prev == HKY_CONF_UNSET) ? default : prev;                    \
    }

#define hky_conf_merge_ptr_value(conf, prev, default)                        \
    if (conf == HKY_CONF_UNSET_PTR) {                                        \
        conf = (prev == HKY_CONF_UNSET_PTR) ? default : prev;                \
    }

#define hky_conf_merge_uint_value(conf, prev, default)                       \
    if (conf == HKY_CONF_UNSET_UINT) {                                       \
        conf = (prev == HKY_CONF_UNSET_UINT) ? default : prev;               \
    }

#define hky_conf_merge_msec_value(conf, prev, default)                       \
    if (conf == HKY_CONF_UNSET_MSEC) {                                       \
        conf = (prev == HKY_CONF_UNSET_MSEC) ? default : prev;               \
    }

#define hky_conf_merge_sec_value(conf, prev, default)                        \
    if (conf == HKY_CONF_UNSET) {                                            \
        conf = (prev == HKY_CONF_UNSET) ? default : prev;                    \
    }

#define hky_conf_merge_size_value(conf, prev, default)                       \
    if (conf == HKY_CONF_UNSET_SIZE) {                                       \
        conf = (prev == HKY_CONF_UNSET_SIZE) ? default : prev;               \
    }

#define hky_conf_merge_off_value(conf, prev, default)                        \
    if (conf == HKY_CONF_UNSET) {                                            \
        conf = (prev == HKY_CONF_UNSET) ? default : prev;                    \
    }

#define hky_conf_merge_str_value(conf, prev, default)                        \
    if (conf.data == NULL) {                                                 \
        if (prev.data) {                                                     \
            conf.len = prev.len;                                             \
            conf.data = prev.data;                                           \
        } else {                                                             \
            conf.len = sizeof(default) - 1;                                  \
            conf.data = (u_char *) default;                                  \
        }                                                                    \
    }

#define hky_conf_merge_bufs_value(conf, prev, default_num, default_size)     \
    if (conf.num == 0) {                                                     \
        if (prev.num) {                                                      \
            conf.num = prev.num;                                             \
            conf.size = prev.size;                                           \
        } else {                                                             \
            conf.num = default_num;                                          \
            conf.size = default_size;                                        \
        }                                                                    \
    }

#define hky_conf_merge_bitmask_value(conf, prev, default)                    \
    if (conf == 0) {                                                         \
        conf = (prev == 0) ? default : prev;                                 \
    }


char *hky_conf_param(hky_conf_t *cf);
char *hky_conf_parse(hky_conf_t *cf, hky_str_t *filename);
char *hky_conf_include(hky_conf_t *cf, hky_command_t *cmd, void *conf);


hky_int_t hky_conf_full_name(hky_origin_t *cycle, hky_str_t *name,
	hky_uint_t conf_prefix);
hky_open_file_t *hky_conf_open_file(hky_origin_t *cycle, hky_str_t *name);
void hky_cdecl hky_conf_log_error(hky_uint_t level, hky_conf_t *cf,
	hky_err_t err, const char *fmt, ...);


char *hky_conf_set_flag_slot(hky_conf_t *cf, hky_command_t *cmd, void *conf);
char *hky_conf_set_str_slot(hky_conf_t *cf, hky_command_t *cmd, void *conf);
char *hky_conf_set_str_array_slot(hky_conf_t *cf, hky_command_t *cmd,
	void *conf);
char *hky_conf_set_keyval_slot(hky_conf_t *cf, hky_command_t *cmd, void *conf);
char *hky_conf_set_num_slot(hky_conf_t *cf, hky_command_t *cmd, void *conf);
char *hky_conf_set_size_slot(hky_conf_t *cf, hky_command_t *cmd, void *conf);
char *hky_conf_set_off_slot(hky_conf_t *cf, hky_command_t *cmd, void *conf);
char *hky_conf_set_msec_slot(hky_conf_t *cf, hky_command_t *cmd, void *conf);
char *hky_conf_set_sec_slot(hky_conf_t *cf, hky_command_t *cmd, void *conf);
char *hky_conf_set_bufs_slot(hky_conf_t *cf, hky_command_t *cmd, void *conf);
char *hky_conf_set_enum_slot(hky_conf_t *cf, hky_command_t *cmd, void *conf);
char *hky_conf_set_bitmask_slot(hky_conf_t *cf, hky_command_t *cmd, void *conf);


#endif // !_HKY_CONF_FILE_H_INCLUDE_
