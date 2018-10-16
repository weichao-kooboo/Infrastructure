#include "Config.h"
void
hky_spinlock(hky_atomic_t *lock, hky_atomic_int_t value, hky_uint_t spin)
{

#if (HKY_HAVE_ATOMIC_OPS)

	hky_uint_t  i, n;

	for (;; ) {

		if (*lock == 0 && hky_atomic_cmp_set(lock, 0, value)) {
			return;
		}

		/*TODO*/
		/*
		if (hky_ncpu > 1) {

			for (n = 1; n < spin; n <<= 1) {

				for (i = 0; i < n; i++) {
					hky_cpu_pause();
				}

				if (*lock == 0 && hky_atomic_cmp_set(lock, 0, value)) {
					return;
				}
			}
		}
		*/

		hky_sched_yield();
	}

#else

#if (HKY_THREADS)

#error hky_spinlock() or hky_atomic_cmp_set() are not defined !

#endif

#endif

}
