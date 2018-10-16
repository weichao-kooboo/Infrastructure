#ifndef _HKY_ATOMIC_H_INCLUDE_
#define _HKY_ATOMIC_H_INCLUDE_

#include "../Config.h"

#define HKY_HAVE_ATOMIC_OPS   1

typedef int32_t                     hky_atomic_int_t;
typedef uint32_t                    hky_atomic_uint_t;
typedef volatile hky_atomic_uint_t  hky_atomic_t;
#define HKY_ATOMIC_T_LEN            (sizeof("-2147483648") - 1)


#if defined( __WATCOMC__ ) || defined( __BORLANDC__ ) || defined(__GNUC__)    \
    || ( _MSC_VER >= 1300 )

/* the new SDK headers */

#define hky_atomic_cmp_set(lock, old, set)                                    \
    ((hky_atomic_uint_t) InterlockedCompareExchange((long *) lock, set, old)  \
                         == old)

#else

/* the old MS VC6.0SP2 SDK headers */

#define hky_atomic_cmp_set(lock, old, set)                                    \
    (InterlockedCompareExchange((void **) lock, (void *) set, (void *) old)   \
     == (void *) old)

#endif


#define hky_atomic_fetch_add(p, add) InterlockedExchangeAdd((long *) p, add)


#define hky_memory_barrier()


#if defined( __BORLANDC__ ) || ( __WATCOMC__ < 1230 )

/*
* Borland C++ 5.5 (tasm32) and Open Watcom C prior to 1.3
* do not understand the "pause" instruction
*/

#define hky_cpu_pause()
#else
#define hky_cpu_pause()       __asm { pause }
#endif


void hky_spinlock(hky_atomic_t *lock, hky_atomic_int_t value, hky_uint_t spin);

#define hky_trylock(lock)  (*(lock) == 0 && hky_atomic_cmp_set(lock, 0, 1))
#define hky_unlock(lock)    *(lock) = 0


#endif // !_HKY_ATOMIC_H_INCLUDE_
