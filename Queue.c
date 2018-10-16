
#include "Config.h"

hky_queue_t *
hky_queue_middle(hky_queue_t *queue)
{
	hky_queue_t  *middle, *next;

	middle = hky_queue_head(queue);

	if (middle == hky_queue_last(queue)) {
		return middle;
	}

	next = hky_queue_head(queue);

	for (;; ) {
		middle = hky_queue_next(middle);

		next = hky_queue_next(next);

		if (next == hky_queue_last(queue)) {
			return middle;
		}

		next = hky_queue_next(next);

		if (next == hky_queue_last(queue)) {
			return middle;
		}
	}
}


/* the stable insertion sort */
void
hky_queue_sort(hky_queue_t *queue, hky_int_t(*cmp)(const hky_queue_t *, const hky_queue_t *))
{
	hky_queue_t  *q, *prev, *next;

	q = hky_queue_head(queue);

	if (q == hky_queue_last(queue)) {
		return;
	}

	for (q = hky_queue_next(q); q != hky_queue_sentinel(queue); q = next) {

		prev = hky_queue_prev(q);
		next = hky_queue_next(q);

		hky_queue_remove(q);

		do {
			if (cmp(prev, q) <= 0) {
				break;
			}

			prev = hky_queue_prev(prev);

		} while (prev != hky_queue_sentinel(queue));

		hky_queue_insert_after(prev, q);
	}
}