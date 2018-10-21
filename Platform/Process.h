#ifndef _HKY_PROCESS_H_INCLUDED_
#define _HKY_PROCESS_H_INCLUDED_

#include "../Config.h"

typedef DWORD               hky_pid_t;
#define HKY_INVALID_PID     0

#define hky_getpid          GetCurrentProcessId
#define hky_getppid()       0
#define hky_log_pid         hky_pid

#define hky_debug_point()
#define hky_sched_yield()   SwitchToThread()

extern hky_pid_t      hky_pid;

#endif // !_HKY_PROCESS_H_INCLUDED_
