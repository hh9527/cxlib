#ifndef __CX_QUEUE_H__
#define __CX_QUEUE_H__

typedef struct cx_queue_s {
	struct cx_queue_s* next;
	struct cx_queue_s* prev;
} cx_queue_t;

#define CX_QUEUE_ZERO(x) { &x, &x }

#define cx_queue_init(h) do { \
	cx_queue_t* __h = h; \
	__h->next = __h->prev = __h; \
} while (0)

#define cx_queue_head(h) ((h)->next)
#define cx_queue_tail(h) ((h)->prev)
#define cx_queue_next(e) ((e)->next)
#define cx_queue_prev(e) ((e)->prev)
#define cx_queue_sentinel(h) (h)
#define cx_queue_empty(h) ((h) == (h)->prev)

#define cx_queue_push(h, e) do { \
	cx_queue_t* __h = h; \
	cx_queue_t* __e = e; \
	__e->prev = __h->prev; \
	__e->prev->next = __e; \
	__e->next = h; \
	__h->prev = e; \
} while (0)

#define cx_queue_push_front(h, e) do { \
	cx_queue_t* __h = h; \
	cx_queue_t* __e = e; \
	__e->next = __h->next; \
	__e->next->prev = __e; \
	__e->prev = __h; \
	__h->next = __e; \
} while (0)

#define cx_queue_remove0(e) do { \
	cx_queue_t* __e = e; \
	__e->next->prev = __e->prev; \
	__e->prev->next = __e->next; \
} while (0)

#define cx_queue_remove(e) do { \
	cx_queue_t* __e = e; \
	__e->next->prev = __e->prev; \
	__e->prev->next = __e->next; \
	__e->prev = __e->next = NULL; \
} while (0)

#define cx_queue_concat(h, h2) do { \
	cx_queue_t* __h = h; \
	cx_queue_t* __h2 = h2; \
	__h->prev->next = __h2->next; \
	__h2->next->prev = __h->prev; \
	__h2->prev->next = __h; \
	__h->prev = __h2->prev; \
	__h2->next = __h2->prev = __h2; \
} while (0)

#define cx_queue_swap(h, h2) do { \
	cx_queue_t* __h = h; \
	cx_queue_t* __h2 = h2; \
	cx_queue_t __t = *__h; \
	*__h = *__h2; \
	*__h2 = __t; \
	__h->prev->next = __h->next->prev = __h; \
	__h2->prev->next = __h2->next->prev = __h2; \
} while (0)

#define cx_queue_each(e, h) for (e = (h)->next; e != (h); e = e->next)
#define cx_queue_each2(e, e2, h) for (e = (h)->next; e2 = e->next, e != (h); e = e2)
#define cx_queue_reach(e, h) for (e = (h)->prev; e != (h); e = e->prev)
#define cx_queue_reach2(e, e2, h) for (e = (h)->prev; e2 = e->prev, e != (h); e = e2)

#endif

