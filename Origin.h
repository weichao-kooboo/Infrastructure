
#ifndef _HKY_ORIGIN_H_INCLUDE_
#define _HKY_ORIGIN_H_INCLUDE_

#include "Config.h"

//�������й��ܵĽṹ
struct hky_origin_s {

	//�����ļ���·��
	hky_array_t paths;
	//����log�ṹ
	hky_log_t   *log;

	hky_log_t new_log;

	hky_uint_t log_use_stderr;

	hky_array_t config_dump;

	hky_rbtree_t config_dump_rbtree;

	hky_pool_t *pool;

	hky_list_t open_files;

	hky_str_t conf_param;

	hky_str_t conf_prefix;

	hky_str_t prefix;

	hky_str_t hostname;
};

extern volatile hky_origin_t  *hky_origin;
extern hky_uint_t hky_dump_config;
#endif // !_HKY_ORIGIN_H_INCLUDE_
