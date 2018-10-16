
#include "../Config.h"

#define HKY_LOG_STDERR            0
#define HKY_LOG_EMERG             1
#define HKY_LOG_ALERT             2
#define HKY_LOG_CRIT              3
#define HKY_LOG_ERR               4
#define HKY_LOG_WARN              5
#define HKY_LOG_NOTICE            6
#define HKY_LOG_INFO              7
#define HKY_LOG_DEBUG             8

#define HKY_LOG_DEBUG_CORE        0x010
#define HKY_LOG_DEBUG_ALLOC       0x020
#define HKY_LOG_DEBUG_MUTEX       0x040
#define HKY_LOG_DEBUG_EVENT       0x080
#define HKY_LOG_DEBUG_HTTP        0x100
#define HKY_LOG_DEBUG_MAIL        0x200
#define HKY_LOG_DEBUG_STREAM      0x400

struct hky_log_s {
	int test;
};