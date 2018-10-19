#ifndef _HKY_THREAD_H_INCLUDED_
#define _HKY_THREAD_H_INCLUDED_
#include "../Config.h"

typedef HANDLE  hky_tid_t;
typedef DWORD   hky_thread_value_t;


hky_err_t hky_create_thread(hky_tid_t *tid,
	hky_thread_value_t(__stdcall *func)(void *arg), void *arg, hky_log_t *log);

#define hky_log_tid                 GetCurrentThreadId()
#define HKY_TID_T_FMT               "%ud"


#endif // !_HKY_THREAD_H_INCLUDED_
