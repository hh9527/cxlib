#ifndef __CBUF_H__
#define __CBUF_H__

#include "cx-base.h"
#include "cx-queue.h"

typedef struct cbuf_s cbuf_t;
typedef struct cbufs_s cbufs_t;
typedef struct ctrunk_s ctrunk_t;

struct cbuf_s {
	struct crbuf_s* raw;
	ssize_t start;
	ssize_t end;
};

#define CBUF_ZERO(x) {NULL, 0, 0}

struct cbufs_s {
	ssize_t length;
	cx_queue_t bufs;
};

#define CBUFS_ZERO(x) {0, CX_QUEUE_ZERO((x).bufs)}

struct ctrunk_s {
	int cbufs;
	int nbufs;
	ssize_t length;
	uv_buf_t* bufs;
};

API cbuf_t*   cbuf_init(cbuf_t* self, const void* data, ssize_t length);
API char*     cbuf_init2(cbuf_t* self, ssize_t length);
API cbuf_t*   cbuf_fini(cbuf_t* self);
API ssize_t   cbuf_length(cbufs_t* self);
API char*     cbuf_base(cbufs_t* self, ssize_t n);
API void      cbuf_swap(cbuf_t* self, cbuf_t* other);
API cbuf_t    cbuf_ref(cbuf_t* self, int transfer_reference);
API cbuf_t    cbuf_slice(cbuf_t* self, ssize_t start, ssize_t end, int transfer_reference);
API cbuf_t    cbuf_mid(cbuf_t* self, ssize_t start, ssize_t n, int transfer_reference);
API ssize_t   cbuf_copy(cbuf_t* self, ssize_t start, ssize_t n, void* target);
API ssize_t   cbuf_shift(cbuf_t* self, ssize_t n, cbuf_t* target);
API ssize_t   cbuf_pop(cbuf_t* self, ssize_t n, cbuf_t* target);
API int       cbuf_is_solid(cbuf_t* self, cbuf_t* other);
API ssize_t   cbuf_find(cbuf_t* self, int ch);

API cbufs_t*  cbufs_init(cbufs_t* self);
API cbufs_t*  cbufs_fini(cbufs_t* self);
API ssize_t   cbufs_length(cbufs_t* self);
API char*     cbufs_base(cbufs_t* self, ssize_t n);
API void      cbufs_swap(cbufs_t* self, cbufs_t* other);
API void      cbufs_concat(cbufs_t* self, cbufs_t* other);
API void      cbufs_push(cbufs_t* self, cbuf_t* buf, int transfer_reference);
API void      cbufs_push_front(cbufs_t* self, cbuf_t* buf, int transfer_reference);
API ssize_t   cbufs_shift(cbufs_t* self, ssize_t n, cbufs_t* target);
API ssize_t   cbufs_shift_to(cbufs_t* self, ssize_t n, void* target);
API ssize_t   cbufs_shift_to_trunk(cbufs_t* self, ssize_t n, ctrunk_t* target);
API ssize_t   cbufs_pop(cbufs_t* self, ssize_t n);
API ssize_t   cbufs_find(cbuf_t* self, int ch);

API ctrunk_t* ctrunk_init(ctrunk_t* self, int cbufs);
API ctrunk_t* ctrunk_fini(ctrunk_t* self);
API ctrunk_t* ctrunk_clear(ctrunk_t* self);
API int       ctrunk_push(ctrunk_t* self, cbuf_t* buf, int transfer_reference);

#endif

