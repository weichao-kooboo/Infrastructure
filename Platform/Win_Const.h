#include "../Config.h"

#ifndef HKY_HAVE_SO_SNDLOWAT
#define HKY_HAVE_SO_SNDLOWAT     1
#endif


#if !(HKY_WIN32)

#define hky_signal_helper(n)     SIG##n
#define hky_signal_value(n)      hky_signal_helper(n)

#define hky_random               random

/* TODO: #ifndef */
#define HKY_SHUTDOWN_SIGNAL      QUIT
#define HKY_TERMINATE_SIGNAL     TERM
#define HKY_NOACCEPT_SIGNAL      WINCH
#define HKY_RECONFIGURE_SIGNAL   HUP

#if (HKY_LINUXTHREADS)
#define HKY_REOPEN_SIGNAL        INFO
#define HKY_CHANGEBIN_SIGNAL     XCPU
#else
#define HKY_REOPEN_SIGNAL        USR1
#define HKY_CHANGEBIN_SIGNAL     USR2
#endif

#define hky_cdecl
#define hky_libc_cdecl

#endif

typedef intptr_t        hky_int_t;
typedef uintptr_t       hky_uint_t;
typedef intptr_t        hky_flag_t;


#define HKY_INT32_LEN   (sizeof("-2147483648") - 1)
#define HKY_INT64_LEN   (sizeof("-9223372036854775808") - 1)

#if (HKY_PTR_SIZE == 4)
#define HKY_INT_T_LEN   HKY_INT32_LEN
#define HKY_MAX_INT_T_VALUE  2147483647

#else
#define HKY_INT_T_LEN   HKY_INT64_LEN
#define HKY_MAX_INT_T_VALUE  9223372036854775807
#endif


#ifndef HKY_ALIGNMENT
#define HKY_ALIGNMENT   sizeof(unsigned long)    /* platform word */
#endif

#define hky_align(d, a)     (((d) + (a - 1)) & ~(a - 1))
#define hky_align_ptr(p, a)                                                   \
    (u_char *) (((uintptr_t) (p) + ((uintptr_t) a - 1)) & ~((uintptr_t) a - 1))


#define hky_abort       abort


/* TODO: platform specific: array[HKY_INVALID_ARRAY_INDEX] must cause SIGSEGV */
#define HKY_INVALID_ARRAY_INDEX 0x80000000


/* TODO: auto_conf: hky_inline   inline __inline __inline__ */
#ifndef hky_inline
#define hky_inline      inline
#endif

#ifndef INADDR_NONE  /* Solaris */
#define INADDR_NONE  ((unsigned int) -1)
#endif

#ifdef MAXHOSTNAMELEN
#define HKY_MAXHOSTNAMELEN  MAXHOSTNAMELEN
#else
#define HKY_MAXHOSTNAMELEN  256
#endif


#define HKY_MAX_UINT32_VALUE  (uint32_t) 0xffffffff
#define HKY_MAX_INT32_VALUE   (uint32_t) 0x7fffffff


#if (HKY_COMPAT)

#define HKY_COMPAT_BEGIN(slots)  uint64_t spare[slots];
#define HKY_COMPAT_END

#else

#define HKY_COMPAT_BEGIN(slots)
#define HKY_COMPAT_END

#endif
