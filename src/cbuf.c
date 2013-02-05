#include "cbuf.h"

#ifndef MALLOC
# define MALLOC(n) malloc(n)
# define REALLOC(p, n) realloc(p, n)
# define FREE(p) free(p)
#endif

struct crbuf_s {
	int     rc;
	ssize_t length;
	char    data[1];
};

struct crbuf_s* crbuf_new(ssize_t length) {
	struct crbuf_s* raw = NULL;
	raw = (struct crbuf_s*)MALLOC(offsetof(struct crbuf_s, data) + length);
	raw->rc = 1;
	raw->length = length;
	return raw;
}

void crbuf_unref(struct crbuf_s* self) {
	if (--self->rc == 0)
		FREE(self);
}

cbuf_t* cbuf_init(cbuf_t* self, const void* data, ssize_t length) {
	struct crbuf_s* raw = NULL;
	assert(length >= 0 || data != NULL);
	if (length < 0)
		length = strlen((const char*)data);
	if (length > 0) {
		raw = crbuf_new(length);
		if (data)
			memcpy(raw->data, data);
	}
	self->raw = raw;
	self->start = 0;
	self->end = length;
	return self;
}

char* cbuf_init2(cbuf_t* self, ssize_t length) {
	if (length > 0) {
		struct crbuf_s* raw = crbuf_new(length);
		self->raw = raw;
		self->start = 0;
		self->end = length;
		return raw->data;
	} else {
		assert(length == 0);
		self->raw = NULL;
		self->start = 0;
		self->end = 0;
		return NULL;
	}
}

cbuf_t* cbuf_fini(cbuf_t* self) {
	if (self->raw) {
		crbuf_unref(self->raw);
		self->raw = NULL;
		self->start = self->end = 0;
	}

	return self;
}

ssize_t cbuf_length(cbufs_t* self) {
	return self->end - self->start;
}

char* cbuf_base(cbufs_t* self, ssize_t n) {
	return self->raw ? self->raw->data + self->start : NULL;
}

void cbuf_swap(cbuf_t* self, cbuf_t* other) {
	cbuf_t t = *self;
	*self = *other;
	*other = t;
}

cbuf_t cbuf_slice(cbuf_t* self, ssize_t start, ssize_t end, int transfer_reference) {
	cbuf_t r = *self;
	ssize_t length = self->end - self->start;
	if (start < 0)
		start += length;
	if (end < 0)
		end += length;
	assert(start >= 0 && start <= length);
	assert(end >= start && end <= length);
	if (end == start) {
		r.raw = NULL;
		r.start = r.end = 0;
		if (transfer_reference) {
			cbuf_fini(self);
		}
	} else {
		r.end = r.start + end;
		r.start += start;
		if (transfer_reference) {
			self->raw = NULL;
			self->start = self->end = 0;
		}
	}

	return r;
}

cbuf_t cbuf_mid(cbuf_t* self, ssize_t start, ssize_t n, int transfer_reference) {
	cbuf_t r = *self;
	ssize_t length = self->end - self->start;
	assert(start >= 0 && start <= length);
	if (n < 0)
		n = length - start;
	assert(n >= 0 && n <= length - start);
	if (n == 0) {
		r.raw = NULL;
		r.start = r.end = 0;
		if (transfer_reference) {
			cbuf_fini(self);
		}
	} else {
		r.start += start;
		r.end = r.start + n;
		if (transfer_reference) {
			self->raw = NULL;
			self->start = self->end = 0;
		}
	}

	return r;
}

ssize_t cbuf_copy(cbuf_t* self, ssize_t start, ssize_t n, void* target) {
	ssize_t length = self->end - self->start;

	assert(start >= 0 && start <= length);
	if (n < 0)
		n = length - start;
	assert(n >= 0 && n <= length - start);
	if (n > 0) {
		char* p = self->raw->data + self->start + start;
		memcpy(target, p, n);
	}

	return n;
}

ssize_t cbuf_shift(cbuf_t* self, ssize_t n, cbuf_t* target) {
	ssize_t length = self->end - self->start;
	if (n < 0 || n > length)
		n = length;
	if (n == 0) {
		if (target) {
			target->raw = NULL;
			target->start = target->end = 0;
		}
	} else {
		if (n == length) {
			if (target) {
				target->raw = self->raw;
				target->start = self->start;
				target->end = self->end;
			} else {
				cbuf_fini(self);
			}
		} else {
			if (target) {
				target->raw = self->raw;
				target->start = self->start;
				target->end = self->start + n;
				++self->raw->rc;
			}
			self->start += n;
		}
	}

	return n;
}

ssize_t cbuf_pop(cbuf_t* self, ssize_t n, cbuf_t* target) {
	ssize_t length = self->end - self->start;
	if (n < 0 || n > length)
		n = length;
	if (n == 0) {
		if (target) {
			target->raw = NULL;
			target->start = target->end = 0;
		}
	} else {
		if (n == length) {
			if (target) {
				target->raw = self->raw;
				target->start = self->start;
				target->end = self->end;
			} else {
				cbuf_fini(self);
			}
		} else {
			if (target) {
				target->raw = self->raw;
				target->start = self->end - n;
				target->end = self->end;
				++self->raw->rc;
			}
			self->end -= n;
		}
	}

	return n;
}

ssize_t cbuf_find(cbuf_t* self, int ch) {
	ssize_t length = self->end - self->start;
	if (length > 0) {
		char* p = self->raw->data + self->start;
		char* e = p + length;
		ssize_t r = 0;
		while (p != e) {
			if (*p == ch)
				return r;
			++r;
			++p;
		}
	}

	return -1;
}

struct cbufe_s {
	struct cbuf_t buf;
	cqueue_t qh;
};

cbufs_t* cbufs_init(cbufs_t* self) {
	self->length = 0;
	cx_queue_init(&self->bufs);
}

cbufs_t* cbufs_fini(cbufs_t* self) {
	if (self->length > 0) {
		self->length = 0;
		cx_queue_t *q, *q2;
		cx_queue_each2(q, q2, &self->bufs) {
			struct cbufe_s* e = CX_GET_SELF(q, struct cbufe_s, qh);
			cbuf_fini(&e->buf);
			FREE(e);
		}
		cx_queue_init(&self->bufs);
	}

	return self;
}

ssize_t cbufs_length(cbufs_t* self) {
	return self->length;
}

char* cbufs_base(cbufs_t* self, ssize_t n) {
}

void cbufs_swap(cbufs_t* self, cbufs_t* other) {
	ssize_t length = self->length;
	self->length = other->length;
	other->length = length;
	cx_queue_swap(&self->bufs, &other->bufs);
}

void cbufs_concat(cbufs_t* self, cbufs_t* other) {
	if (other->length > 0) {
		self->length += other->length;
		cx_queue_concat(&self->bufs, &other->bufs);
	}
}

void cbufs_push(cbufs_t* self, cbuf_t* buf, int transfer_reference) {
	if (buf->end > buf->start) {
		struct cbufe_s* e = CX_GET_SELF(cx_queue_tail(&self->bufs), struct cbufe_s, qh);
		self->length += (buf->end - buf->start);
		if (cbuf_is_solid(&e->buf, buf)) {
			e->buf.end = buf->end;
			if (transfer_reference)
				cbuf_fini(buf);
		} else {
			e = CX_NEW(MALLOC, struct cbufe_s, 0);
			*e->buf = cbuf_ref(buf, transfer_reference);
			cx_queue_push(&self->bufs, &e->qh);
		}
	}
}

void cbufs_push_front(cbufs_t* self, cbuf_t* buf, int transfer_reference) {
	if (buf->end > buf->start) {
		struct cbufe_s* e = CX_GET_SELF(cx_queue_head(&self->bufs), struct cbufe_s, qh);
		self->length += (buf->end - buf->start);
		if (cbuf_is_solid(buf, &e->buf)) {
			e->buf.start = buf->start;
			if (transfer_reference)
				cbuf_fini(buf);
		} else {
			e = CX_NEW(MALLOC, struct cbufe_s, 0);
			*e->buf = cbuf_ref(buf, transfer_reference);
			cx_queue_push_front(&self->bufs, &e->qh);
		}
	}
}

ssize_t cbufs_shift(cbufs_t* self, ssize_t n, cbufs_t* target) {
	if (n < 0 || n > self->length)
		n = self->length;
	if (n > 0) {
		ssize_t r = n;
		cx_queue_t *q, *q2;
		cx_queue_each2(q, q2, &self->bufs) {
			struct cbufe_s* e = CX_GET_SELF(q, struct cbufe_s, qh);
			ssize_t l = e->buf.end - e->buf.start;
			if (r >= l) {
				r -= l;
				cx_queue_remove0(q);
				if (target) {
					cx_queue_push(&target->bufs, q);
				} else {
					cbuf_fini(&e->buf);
					FREE(e);
				}
				if (r == 0)
					break;
			} else {
				if (target) {
					struct cbufe_s* e2 = CX_NEW(MALLOC, struct cbufe_s, 0);
					cbuf_shift(&e->buf, r, &e2->buf);
					cx_queue_push(&target->bufs, &e2->qh);
					target->length += r;
				} else {
					cbuf_shift(&e->buf, r, NULL);
				}
				break;
			}
		}

		self->length -= n;
	}

	return n;
}

ssize_t cbufs_shift_to(cbufs_t* self, ssize_t n, void* target) {
	if (n < 0 || n > self->length)
		n = self->length;
	if (n > 0) {
		ssize_t r = n;
		cx_queue_t *q, *q2;
		char* p = (char*)target;
		cx_queue_each2(q, q2, &self->bufs) {
			struct cbufe_s* e = CX_GET_SELF(q, struct cbufe_s, qh);
			ssize_t l = e->buf.end - e->buf.start;
			if (r >= l) {
				r -= l;
				cx_queue_remove0(q);
				memcpy(p, e->buf.raw->data + e->buf.start, l);
				cbuf_fini(&e->buf);
				FREE(e);
				if (r == 0)
					break;
			} else {
				memcpy(p, e->buf.raw->data + e->buf.start, r);
				cbuf_shift(&e->buf, r, NULL);
				break;
			}
		}

		self->length -= n;
	}

	return n;
}

ssize_t cbufs_shift_to_trunk(cbufs_t* self, ssize_t n, ctrunk_t* target) {
	if (n < 0 || n > self->length)
		n = self->length;
	if (n > 0) {
		ssize_t r = n;
		cx_queue_t *q, *q2;
		char* p = (char*)target;
		cx_queue_each2(q, q2, &self->bufs) {
			struct cbufe_s* e = CX_GET_SELF(q, struct cbufe_s, qh);
			ssize_t l = e->buf.end - e->buf.start;
			if (r >= l) {
				r -= l;
				cx_queue_remove0(q);
				ctrunk_push(target, &e->buf, 1);
				cbuf_fini(&e->buf);
				FREE(e);
				if (r == 0)
					break;
			} else {
				cbuf_t t;
				cbuf_shift(&e->buf, r, &t);
				ctrunk_push(target, &t, 1);
				break;
			}
		}

		self->length -= n;
	}

	return n;
}

ssize_t cbufs_pop(cbufs_t* self, ssize_t n) {
	if (n < 0 || n > self->length)
		n = self->length;
	if (n > 0) {
		ssize_t r = n;
		cx_queue_t *q, *q2;
		cx_queue_reach2(q, q2, &self->bufs) {
			struct cbufe_s* e = CX_GET_SELF(q, struct cbufe_s, qh);
			ssize_t l = e->buf.end - e->buf.start;
			if (r >= l) {
				r -= l;
				cx_queue_remove0(q);
				cbuf_fini(&e->buf);
				FREE(e);
				if (r == 0)
					break;
			} else {
				cbuf_pop(&e->buf, r, NULL);
				break;
			}
		}
	}

	return n;
}

ssize_t cbufs_find(cbuf_t* self, int ch) {
	ssize_t r = 0;
	cx_queue_t *q, *q2;
	cx_queue_reach2(q, q2, &self->bufs) {
		struct cbufe_s* e = CX_GET_SELF(q, struct cbufe_s, qh);
		ssize_t i = cbuf_find(&e->buf, ch);
		if (i >= 0)
			return r + i;
		r += (e->buf.end - e->buf.start);
	}

	return -1;
}

ctrunk_t* ctrunk_init(ctrunk_t* self, int cbufs) {
	void* p = NULL;

	if (cbufs > 0)
		p = MALLOC((sizeof(uv_buf_t) + sizeof(struct crbuf_s)) * cbufs);
	self->cbufs = cbufs;
	self->nbufs = 0;
	self->length = 0;
	self->bufs = (uv_buf_t*)p;
	return self;
}

ctrunk_t* ctrunk_fini(ctrunk_t* self) {
	if (self->bufs) {
		ctrunk_clear(self);
		FREE(self->bufs);
		self->bufs = NULL;
		self->cbufs = 0;
	}
}

ctrunk_t* ctrunk_clear(ctrunk_t* self) {
	if (self->nbufs > 0) {
		struct crbuf_s* p = (struct crbuf_s*)((void*)(self->bufs + self->cbufs));
		struct crbuf_s* e = p + self->nbufs;

		while (p != e) {
			crbuf_unref(p);
			++p;
		}

		self->nbufs = 0;
		self->length = 0;
	}
}

int ctrunk_push(ctrunk_t* self, cbuf_t* buf, int transfer_reference) {
	struct crbuf_s* raws;
	uv_buf_t* buf;

	if (self->nbufs == self->cbufs) {
		int cbufs = self->cbufs ? (self->cbufs * 2) : 4;
		uv_buf_t* bufs = (cbuf_t*)REALLOC(self->bufs, (sizeof(uv_buf_t) + sizeof(struct crbuf_s)) * cbufs);
		raws = (struct crbuf_s*)((void*)(bufs + cbufs));
		if (self->cbufs > 0) {
			struct crbuf_s* old_raws = (struct crbuf_s*)((void*)(bufs + self->cbufs));
			memmove(raws, old_raws, self->cbufs);
		}

		self->bufs = bufs;
		self->cbufs = cbufs;
	} else {
		raws = (struct crbuf_s*)((void*)(self->bufs + self->cbufs));
	}

	buf = self->bufs + self->nbuf;
	buf->base = cbuf_base(buf);
	buf->len = cbuf_length(buf);
	raws[self->nbuf] = buf->raw;
	++self->nbuf;

	if (transfer_reference) {
		buf->raw = NULL;
		buf->start = buf->end = 0;
	} else {
		++buf->raw;
	}

	return 0;
}

