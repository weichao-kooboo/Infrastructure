#ifndef _HKY_CONFIG_H_INCLUDE_
#define _HKY_CONFIG_H_INCLUDE_

#include "Platform\Win_Config.h"
#include "Platform\Win_Const.h"

typedef struct hky_log_s hky_log_t;
typedef struct hky_file_s hky_file_t;
typedef struct hky_chain_s hky_chain_t;
typedef struct hky_pool_s hky_pool_t;

#define LF     (u_char) '\n'
#define CR     (u_char) '\r'
#define CRLF   "\r\n"

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

#include "Platform/Log.h"
#include "Platform/Alloc.h"
#include "String.h"
#include "Times.h"
#include "Buf.h"
#include "Platform/Files.h"
#include "Palloc.h"
#include "File.h"

#endif // !_HKY_CONFIG_H_INCLUDE_
