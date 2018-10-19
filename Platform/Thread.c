#include "../Config.h"


hky_err_t
hky_create_thread(hky_tid_t *tid,
	hky_thread_value_t(__stdcall *func)(void *arg), void *arg, hky_log_t *log)
{
	u_long     id;
	hky_err_t  err;

	*tid = CreateThread(NULL, 0, func, arg, 0, &id);

	if (*tid != NULL) {
		hky_log_error(HKY_LOG_NOTICE, log, 0,
			"create thread " HKY_TID_T_FMT, id);
		return 0;
	}

	err = hky_errno;
	hky_log_error(HKY_LOG_ALERT, log, err, "CreateThread() failed");
	return err;
}
