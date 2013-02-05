#ifndef __CX_QUEUE_H__
#define __CX_QUEUE_H__

typedef struct cx_queue_s {
	struct cx_queue_s* next;
	struct cx_queue_s* prev;
} cx_queue_t;

#define CX_QUEUE_ZERO(x) { &x, &x }

#define cx_queue_init(__h) do { \
	cx_queue_t* h = __h; \
	h->next = h->prev = h; \
} while (0)

#define cx_queue_head(h) ((h)->next)
#define cx_queue_tail(h) ((h)->prev)
#define cx_queue_next(e) ((e)->next)
#define cx_queue_prev(e) ((e)->prev)
#define cx_queue_sentinel(h) (h)
#define cx_queue_empty(h) ((h) == (h)->prev)

#define cx_queue_push(__h, __e) do { \
	cx_queue_t* h = __h; \
	cx_queue_t* e = __e; \
	e->prev = h->prev; \
	e->prev->next = e; \
	e->next = h; \
	h->prev = e; \
} while (0)

#define cx_queue_push_front(__h, __e) do { \
	cx_queue_t* h = __h; \
	cx_queue_t* e = __e; \
	e->next = h->next; \
	e->next->prev = e; \
	e->prev = h; \
	h->next = e; \
} while (0)

#define cx_queue_remove0(__e) do { \
	cx_queue_t* e = __e; \
	e->next->prev = e->prev; \
	e->prev->next = e->next; \
} while (0)

#define cx_queue_remove(__e) do { \
	cx_queue_t* e = __e; \
	e->next->prev = e->prev; \
	e->prev->next = e->next; \
	e->prev = e->next = NULL; \
} while (0)

#define cx_queue_concat(__h, __h2) do { \
	cx_queue_t* h = __h; \
	cx_queue_t* h2 = __h2; \
	h->prev->next = h2->next; \
	h2->next->prev = h->prev; \
	h2->prev->next = h; \
	h->prev = h2->prev; \
	h2->next = h2->prev = h2; \
} while (0)

#define cx_queue_swap(__h, __h2) do { \
	cx_queue_t* h = __h; \
	cx_queue_t* h2 = __h2; \
	cx_queue_t t = *h; \
	*h = *h2; \
	*h2 = t; \
	h->prev->next = h->next->prev = h; \
	h2->prev->next = h2->next->prev = h2; \
} while (0)

#define cx_queue_each(e, h) for (e = (h)->next; e != (h); e = e->next)
#define cx_queue_each2(e, e2, h) for (e = (h)->next; e2 = e->next, e != (h); e = e2)
#define cx_eueue_reach(e, h) for (e = (h)->prev; e != (h); e = e->prev)
#define cx_eueue_reach2(e, e2, h) for (e = (h)->prev; e2 = e->prev, e != (h); e = e2)

#endif

