
#ifndef _HKY_ORIGIN_H_INCLUDE_
#define _HKY_ORIGIN_H_INCLUDE_

#include "Config.h"

//汇总所有功能的结构
struct hky_origin_s {

	//保存文件的路径
	hky_array_t paths;
	//保存log结构
	hky_log_t   *log;

	hky_array_t config_dump;

	hky_rbtree_t config_dump_rbtree;

	hky_pool_t *pool;

	hky_list_t open_files;

	hky_str_t conf_param;

	hky_str_t conf_prefix;

	hky_str_t prefix;
};

extern volatile hky_origin_t  *hky_origin;
extern hky_uint_t hky_dump_config;
#endif // !_HKY_ORIGIN_H_INCLUDE_
