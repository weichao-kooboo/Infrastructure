#ifndef _HKY_CONFIG_H_INCLUDE_
#define _HKY_CONFIG_H_INCLUDE_

#include "Platform\Win_Config.h"
#include "Platform\Win_Const.h"

typedef struct hky_log_s hky_log_t;
typedef struct hky_chain_s hky_chain_t;
typedef struct hky_pool_s hky_pool_t;
typedef struct hky_open_file_s hky_open_file_t;
typedef struct hky_command_s hky_command_t;
typedef struct hky_file_s hky_file_t;
typedef struct hky_conf_s hky_conf_t;
typedef struct hky_origin_s hky_origin_t;

#define LF     (u_char) '\n'
#define CR     (u_char) '\r'
#define CRLF   "\r\n"

#define hky_abs(value)       (((value) >= 0) ? (value) : - (value))
#define hky_max(val1, val2)  ((val1 < val2) ? (val2) : (val1))
#define hky_min(val1, val2)  ((val1 > val2) ? (val2) : (val1))

#define  HKY_OK          0
#define  HKY_ERROR      -1
#define  HKY_AGAIN      -2
#define  HKY_BUSY       -3
#define  HKY_DONE       -4
#define  HKY_DECLINED   -5
#define  HKY_ABORT      -6

#include "Platform/Errno.h"
#include "Platform/Atomic.h"
#include "Rbtree.h"
#include "Queue.h"
#include "Platform/Time.h"
/* none relationShip above */

#include "Platform/Alloc.h"
#include "Platform/Process.h"
#include "String.h"
#include "Times.h"
#include "Buf.h"
#include "Platform/Files.h"
#include "Platform/Thread.h"
#include "Palloc.h"
#include "List.h"
#include "Array.h"
#include "Inet.h"
#include "Platform/Os.h"
#include "Syslog.h"
#include "Log.h"
#include "Origin.h"
#include "Platform/User.h"
#include "File.h"
#include "Parse.h"
#include "Crc32.h"
#include "Conf_File.h"

#endif // !_HKY_CONFIG_H_INCLUDE_
