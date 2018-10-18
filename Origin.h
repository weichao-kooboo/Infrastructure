
#ifndef _HKY_ORIGIN_H_INCLUDE_
#define _HKY_ORIGIN_H_INCLUDE_

#include "Config.h"

//汇总所有功能的结构
struct hky_origin_s {

	//保存文件的路径
	hky_array_t paths;
	//保存log结构
	hky_log_t   *log;
};

extern volatile hky_origin_t  *hky_origin;
#endif // !_HKY_ORIGIN_H_INCLUDE_
