
#ifndef _HKY_ORIGIN_H_INCLUDE_
#define _HKY_ORIGIN_H_INCLUDE_

#include "Config.h"

//�������й��ܵĽṹ
struct hky_origin_s {

	//�����ļ���·��
	hky_array_t paths;
	//����log�ṹ
	hky_log_t   *log;
};

extern volatile hky_origin_t  *hky_origin;
#endif // !_HKY_ORIGIN_H_INCLUDE_
