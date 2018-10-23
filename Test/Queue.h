#ifndef _HKY_QUEUE_H_INCLUDE_
#define _HKY_QUEUE_H_INCLUDE_

#include "Config.h"

typedef struct hky_queue_s hky_queue_t;

struct hky_queue_s {
	hky_queue_t *prev;
	hky_queue_t *next;
};

#define hky_queue_init(q)                                                     \
    (q)->prev = q;                                                            \
    (q)->next = q


#define hky_queue_empty(h)                                                    \
    (h == (h)->prev)


#define hky_queue_insert_head(h, x)                                           \
    (x)->next = (h)->next;                                                    \
    (x)->next->prev = x;                                                      \
    (x)->prev = h;                                                            \
    (h)->next = x


#define hky_queue_insert_after   hky_queue_insert_head


#define hky_queue_insert_tail(h, x)                                           \
    (x)->prev = (h)->prev;                                                    \
    (x)->prev->next = x;                                                      \
    (x)->next = h;                                                            \
    (h)->prev = x


#define hky_queue_head(h)                                                     \
    (h)->next


#define hky_queue_last(h)                                                     \
    (h)->prev


#define hky_queue_sentinel(h)                                                 \
    (h)


#define hky_queue_next(q)                                                     \
    (q)->next


#define hky_queue_prev(q)                                                     \
    (q)->prev


#if (HKY_DEBUG)

#define hky_queue_remove(x)                                                   \
    (x)->next->prev = (x)->prev;                                              \
    (x)->prev->next = (x)->next;                                              \
    (x)->prev = NULL;                                                         \
    (x)->next = NULL

#else

#define hky_queue_remove(x)                                                   \
    (x)->next->prev = (x)->prev;                                              \
    (x)->prev->next = (x)->next

#endif


#define hky_queue_split(h, q, n)                                              \
    (n)->prev = (h)->prev;                                                    \
    (n)->prev->next = n;                                                      \
    (n)->next = q;                                                            \
    (h)->prev = (q)->prev;                                                    \
    (h)->prev->next = h;                                                      \
    (q)->prev = n;


#define hky_queue_add(h, n)                                                   \
    (h)->prev->next = (n)->next;                                              \
    (n)->next->prev = (h)->prev;                                              \
    (h)->prev = (n)->prev;                                                    \
    (h)->prev->next = h;


#define hky_queue_data(q, type, link)                                         \
    (type *) ((u_char *) q - offsetof(type, link))

hky_queue_t *hky_queue_middle(hky_queue_t *queue);
void hky_queue_sort(hky_queue_t *queue, hky_int_t(*cmp)(const hky_queue_t *, const hky_queue_t *));

#endif // !_HKY_QUEUE_H_INCLUDE_
