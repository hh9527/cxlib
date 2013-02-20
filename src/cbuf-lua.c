#include <assert.h>
#include <stdlib.h>
#include <memory.h>
#include <lauxlib.h>

#include "cbuf.h"

#define EXPORT extern

#define L_BUF_META "cbuf.buf"
#define L_BUFS_META "cbuf.bufs"
#define L_STRUCT_META "cbuf.struct"

enum {
	CSTRUCT_OP_PADDING = 1,
	CSTRUCT_OP_STRING,
	CSTRUCT_OP_DSTRING,
	CSTRUCT_OP_ZSTRING,
	CSTRUCT_OP_DZSTRING,
	CSTRUCT_OP_INT8,
	CSTRUCT_OP_UINT8,
	CSTRUCT_OP_INT16,
	CSTRUCT_OP_UINT16,
	CSTRUCT_OP_INT32,
	CSTRUCT_OP_UINT32,
	CSTRUCT_OP_INT64,
	CSTRUCT_OP_UINT64,
	CSTRUCT_OP_FLOAT32,
	CSTRUCT_OP_FLOAT64,
	CSTRUCT_OP_SW_INT16,
	CSTRUCT_OP_SW_UINT16,
	CSTRUCT_OP_SW_INT32,
	CSTRUCT_OP_SW_UINT32,
	CSTRUCT_OP_SW_INT64,
	CSTRUCT_OP_SW_UINT64,
	CSTRUCT_OP_SW_FLOAT32,
	CSTRUCT_OP_SW_FLOAT64,
};

static int L_struct_new(lua_State* L) {
	const char* fp = luaL_checkstring(L, 1);
	int* self = NULL;
	int* data = NULL;
	int length = 0;
	int n = 0;
	int capacity = 0;
	int ch = *(fp++);
	int need_swap = 0;

#define PUSH(x) do { \
	if (n == capacity) { \
		capacity += 8; \
		data = realloc(data, sizeof(int) * capacity); \
	} \
	data[n++] = x; \
} while (0)

	while (ch != 0) {
		int op = 0;
		int rep = -1;
		switch (ch) {
		case '@':
			need_swap = 0;
			break;
		case '<':
#ifdef CX_IS_BIG_ENDIAN
			need_swap = 1;
#else
			need_swap = 0;
#endif
			break;
		case '>':
		case '!':
#ifdef CX_IS_BIG_ENDIAN
			need_swap = 0;
#else
			need_swap = 1;
#endif
			break;
		case 'x':
			op = CSTRUCT_OP_PADDING;
			rep = 1;
			break;
		case 's':
			op = CSTRUCT_OP_STRING;
			rep = 0;
			break;
		case 'z':
			op = CSTRUCT_OP_ZSTRING;
			length += 1;
			rep = 0;
			break;
		case 'b':
			op = CSTRUCT_OP_INT8;
			length += 1;
			break;
		case 'B':
			op = CSTRUCT_OP_UINT8;
			length += 1;
			break;
		case 'h':
			op = need_swap ? CSTRUCT_OP_INT16 : CSTRUCT_OP_SW_INT16;
			length += 2;
			break;
		case 'H':
			op = need_swap ? CSTRUCT_OP_UINT16 : CSTRUCT_OP_SW_UINT16;
			length += 2;
			break;
		case 'l':
			op = need_swap ? CSTRUCT_OP_INT32 : CSTRUCT_OP_SW_INT32;
			length += 4;
			break;
		case 'L':
			op = need_swap ? CSTRUCT_OP_UINT32 : CSTRUCT_OP_SW_UINT32;
			length += 4;
			break;
		case 'q':
			op = need_swap ? CSTRUCT_OP_INT64 : CSTRUCT_OP_SW_INT64;
			length += 8;
			break;
		case 'Q':
			op = need_swap ? CSTRUCT_OP_UINT64 : CSTRUCT_OP_SW_UINT64;
			length += 8;
			break;
		case 'f':
			op = need_swap ? CSTRUCT_OP_FLOAT32 : CSTRUCT_OP_SW_FLOAT32;
			length += 4;
			break;
		case 'd':
			op = need_swap ? CSTRUCT_OP_FLOAT64 : CSTRUCT_OP_SW_FLOAT64;
			length += 8;
			break;
		default:
			if (data)
				free(data);
			return luaL_error(L, "Invalid format character: '%c'", ch);
		}

		ch = *(fp++);

		if (rep >= 0) {
			if (ch == '#') {
				PUSH(op + 1);
			} else if (ch >= '1' && ch <= '9') {
				PUSH(op);
				rep = 0;
				do {
					rep = rep * 10 + (ch - '0');
					ch = *(fp++);
				} while (ch >= '0' && ch <= '9');
				PUSH(rep);
				length += rep;
			} else {
				if (rep == 0) {
					if (data)
						free(data);
					return luaL_error(L, "Invalid format character: near '%c'", ch);
				}
			}
		} else {
			PUSH(op);
		}
	}

	PUSH(0);
	self = (int*)lua_newuserdata(L, sizeof(int) * (n + 1));
	self[0] = length;
	memcpy(self + 1, data, sizeof(int) * n);
	luaL_setmetatable(L, L_STRUCT_META);
	return 1;
#undef PUSH
};

static int L_struct_len(lua_State* L) {
	int* self = (int*)luaL_checkudata(L, 1, L_STRUCT_META);
	lua_pushinteger(L, self[0]);
	return 1;
}

static int L_buf_new(lua_State* L) {
	if (lua_isnumber(L, 1)) {
		ssize_t l = (ssize_t)lua_tointeger(L, 1);
		cbuf_t* self = (cbuf_t*)lua_newuserdata(L, sizeof(cbuf_t));
		cbuf_init2(self, l);
	} else if (lua_isstring(L, 1)) {
		size_t l;
		const char* s = luaL_tolstring(L, 1, &l);
		cbuf_t* self = (cbuf_t*)lua_newuserdata(L, sizeof(cbuf_t));
		cbuf_init(self, s, (ssize_t)l);
	} else {
		luaL_argerror(L, 1, "integer or string value expected.");
	}

	luaL_setmetatable(L, L_BUF_META);
	return 1;
}

static int L_buf_gc(lua_State* L) {
	cbuf_t* self = (cbuf_t*)luaL_checkudata(L, 1, L_BUF_META);
	cbuf_fini(self);
	return 0;
}

static int L_buf_len(lua_State* L) {
	cbuf_t* self = (cbuf_t*)luaL_checkudata(L, 1, L_BUF_META);
	lua_pushinteger(L, self->end - self->start);
	return 1;
}

static int L_buf_base(lua_State* L) {
	cbuf_t* self = (cbuf_t*)luaL_checkudata(L, 1, L_BUF_META);
	lua_pushlightuserdata(L, cbuf_base(self));
	return 1;
}

static int L_buf_slice(lua_State* L) {
	cbuf_t* self = (cbuf_t*)luaL_checkudata(L, 1, L_BUF_META);
	int length = self->end - self->start;
	int start = luaL_optint(L, 1, 0);
	int end = luaL_optint(L, 2, length);
	if (start == 0 && end == length) {
		lua_settop(L, 1);
	} else {
		cbuf_t* obj = (cbuf_t*)lua_newuserdata(L, sizeof(cbuf_t));
		*obj = cbuf_slice(self, start, end, 0);
		luaL_setmetatable(L, L_BUF_META);
	}

	return 1;
}

static int L_buf_tostring(lua_State* L) {
	cbuf_t* self = (cbuf_t*)luaL_checkudata(L, 1, L_BUF_META);
	const char* s = cbuf_base(self);
	ssize_t len = cbuf_length(self);
	int start = luaL_optint(L, 2, 0);
	int n = luaL_optint(L, 3, -1);
	if (start < 0)
		start += len;
	if (start >= 0 && start < len) {
		if (n < 0)
			n = len - start;
		if (n > 0) {
			lua_pushlstring(L, s + start, n);
			return 1;
		}
	}

	lua_pushliteral(L, "");
	return 1;
}

typedef union {
	unsigned char b[8];
	int16_t  i16; 
	uint16_t u16;
	int32_t  i32; 
	uint32_t u32;
	int64_t  i64; 
	uint64_t u64;
	float    f32;
	double   f64;
} num_t;

static int L_buf_pack(lua_State* L) {
	cbuf_t* self = (cbuf_t*)luaL_checkudata(L, 1, L_BUF_META);
	int off = luaL_checkint(L, 2);
	int* sd = (int*)luaL_checkudata(L, 3, L_STRUCT_META) + 1;
	unsigned char* p = (unsigned char*)cbuf_base(self) + off;
	int n = 4;
	int op;
	num_t num;

	if (off < 0 || off >= cbuf_length(self))
		return luaL_argerror(L, 2, "offset out of range");

	while ((op = *(sd++)) != 0) {
		switch (op) {
		case CSTRUCT_OP_PADDING:
			{
				int rep = *(sd++);
				while (rep-- > 0) {
					*(p++) = 0;
				}
			}
			break;
		case CSTRUCT_OP_STRING:
			{
				int len = *(sd++);
				size_t length;
				const char* s = luaL_checklstring(L, n++, &length);
				if (length > (size_t)len)
					length = len;
				memcpy(p, s, length);
				p += len;
			}
			break;
		case CSTRUCT_OP_DSTRING:
			{
				int len = luaL_checkint(L, n++);
				size_t length;
				const char* s = luaL_checklstring(L, n++, &length);
				if (length > (size_t)len)
					length = len;
				memcpy(p, s, length);
				p += len;
			}
			break;
		case CSTRUCT_OP_ZSTRING:
			{
				int len = *(sd++);
				size_t length;
				const char* s = luaL_checklstring(L, n++, &length);
				if (length > (size_t)len)
					length = len;
				memcpy(p, s, length);
				p[length] = 0;
				p += (len + 1);
			}
			break;
		case CSTRUCT_OP_DZSTRING:
			{
				int len = luaL_checkint(L, n++);
				size_t length;
				const char* s = luaL_checklstring(L, n++, &length);
				if (length > (size_t)len)
					length = len;
				memcpy(p, s, length);
				p[length] = 0;
				p += (len + 1);
			}
			break;
		case CSTRUCT_OP_INT8:
		case CSTRUCT_OP_UINT8:
			{
				int v = luaL_checkint(L, n++);
				*(p++) = (unsigned)((char)v);
			}
			break;
		case CSTRUCT_OP_INT16:
			{
				num.i16 = (int16_t)luaL_checkint(L, n++);
				*(p++) = num.b[0];
				*(p++) = num.b[1];
			}
			break;
		case CSTRUCT_OP_UINT16:
			{
				num.u16 = (uint16_t)luaL_checkint(L, n++);
				*(p++) = num.b[0];
				*(p++) = num.b[1];
			}
			break;
		case CSTRUCT_OP_INT32:
			{
				num.i32 = (int32_t)luaL_checkinteger(L, n++);
				*(p++) = num.b[0];
				*(p++) = num.b[1];
				*(p++) = num.b[2];
				*(p++) = num.b[3];
			}
			break;
		case CSTRUCT_OP_UINT32:
			{
				num.u32 = (uint32_t)luaL_checkinteger(L, n++);
				*(p++) = num.b[0];
				*(p++) = num.b[1];
				*(p++) = num.b[2];
				*(p++) = num.b[3];
			}
			break;
		case CSTRUCT_OP_INT64:
			{
				num.i64 = (uint64_t)luaL_checkinteger(L, n++);
				*(p++) = num.b[0];
				*(p++) = num.b[1];
				*(p++) = num.b[2];
				*(p++) = num.b[3];
				*(p++) = num.b[4];
				*(p++) = num.b[5];
				*(p++) = num.b[6];
				*(p++) = num.b[7];
			}
			break;
		case CSTRUCT_OP_UINT64:
			{
				num.u64 = (uint64_t)luaL_checkunsigned(L, n++);
				*(p++) = num.b[0];
				*(p++) = num.b[1];
				*(p++) = num.b[2];
				*(p++) = num.b[3];
				*(p++) = num.b[4];
				*(p++) = num.b[5];
				*(p++) = num.b[6];
				*(p++) = num.b[7];
			}
			break;
		case CSTRUCT_OP_FLOAT32:
			{
				num.f32 = (float)luaL_checknumber(L, n++);
				*(p++) = num.b[0];
				*(p++) = num.b[1];
				*(p++) = num.b[2];
				*(p++) = num.b[3];
			}
			break;
		case CSTRUCT_OP_FLOAT64:
			{
				num.f64 = (float)luaL_checknumber(L, n++);
				*(p++) = num.b[0];
				*(p++) = num.b[1];
				*(p++) = num.b[2];
				*(p++) = num.b[3];
				*(p++) = num.b[4];
				*(p++) = num.b[5];
				*(p++) = num.b[6];
				*(p++) = num.b[7];
			}
			break;
		case CSTRUCT_OP_SW_INT16:
			{
				num.i16 = (int16_t)luaL_checkint(L, n++);
				*(p++) = num.b[1];
				*(p++) = num.b[0];
			}
			break;
		case CSTRUCT_OP_SW_UINT16:
			{
				num.u16 = (uint16_t)luaL_checkint(L, n++);
				*(p++) = num.b[1];
				*(p++) = num.b[0];
			}
			break;
		case CSTRUCT_OP_SW_INT32:
			{
				num.i32 = (int32_t)luaL_checkinteger(L, n++);
				*(p++) = num.b[3];
				*(p++) = num.b[2];
				*(p++) = num.b[1];
				*(p++) = num.b[0];
			}
			break;
		case CSTRUCT_OP_SW_UINT32:
			{
				num.u32 = (uint32_t)luaL_checkinteger(L, n++);
				*(p++) = num.b[3];
				*(p++) = num.b[2];
				*(p++) = num.b[1];
				*(p++) = num.b[0];
			}
			break;
		case CSTRUCT_OP_SW_INT64:
			{
				num.i64 = (uint64_t)luaL_checkinteger(L, n++);
				*(p++) = num.b[7];
				*(p++) = num.b[6];
				*(p++) = num.b[5];
				*(p++) = num.b[4];
				*(p++) = num.b[3];
				*(p++) = num.b[2];
				*(p++) = num.b[1];
				*(p++) = num.b[0];
			}
			break;
		case CSTRUCT_OP_SW_UINT64:
			{
				num.u64 = (uint64_t)luaL_checkunsigned(L, n++);
				*(p++) = num.b[7];
				*(p++) = num.b[6];
				*(p++) = num.b[5];
				*(p++) = num.b[4];
				*(p++) = num.b[3];
				*(p++) = num.b[2];
				*(p++) = num.b[1];
				*(p++) = num.b[0];
			}
			break;
		case CSTRUCT_OP_SW_FLOAT32:
			{
				num.f32 = (float)luaL_checknumber(L, n++);
				*(p++) = num.b[3];
				*(p++) = num.b[2];
				*(p++) = num.b[1];
				*(p++) = num.b[0];
			}
			break;
		case CSTRUCT_OP_SW_FLOAT64:
			{
				num.f64 = (float)luaL_checknumber(L, n++);
				*(p++) = num.b[7];
				*(p++) = num.b[6];
				*(p++) = num.b[5];
				*(p++) = num.b[4];
				*(p++) = num.b[3];
				*(p++) = num.b[2];
				*(p++) = num.b[1];
				*(p++) = num.b[0];
			}
			break;
		default:
			assert(0);
		}
	}

	return 0;
}

static int L_buf_unpack(lua_State* L) {
	cbuf_t* self = (cbuf_t*)luaL_checkudata(L, 1, L_BUF_META);
	int off = luaL_checkint(L, 2);
	int* sd = (int*)luaL_checkudata(L, 3, L_STRUCT_META) + 1;
	unsigned char* p = (unsigned char*)cbuf_base(self) + off;
	int n = 4;
	int r = 0;
	int op;
	num_t num;

	if (off < 0 || off >= cbuf_length(self))
		return luaL_argerror(L, 2, "offset out of range");

	while ((op = *(sd++)) != 0) {
		switch (op) {
		case CSTRUCT_OP_PADDING:
			{
				int rep = *(sd++);
				while (rep-- > 0) {
					++p;
				}
			}
			break;
		case CSTRUCT_OP_STRING:
			{
				int len = *(sd++);
				lua_pushlstring(L, (const char*)p, (size_t)len);
				++r;
				p += len;
			}
			break;
		case CSTRUCT_OP_DSTRING:
			{
				int len = luaL_checkint(L, n++);
				lua_pushlstring(L, (const char*)p, (size_t)len);
				++r;
				p += len;
			}
			break;
		case CSTRUCT_OP_ZSTRING:
			{
				int len = *(sd++);
				lua_pushlstring(L, (const char*)p, (size_t)len);
				++r;
				p += (len + 1);
			}
			break;
		case CSTRUCT_OP_DZSTRING:
			{
				int len = luaL_checkint(L, n++);
				lua_pushlstring(L, (const char*)p, (size_t)len);
				++r;
				p += (len + 1);
			}
			break;
		case CSTRUCT_OP_INT8:
			{
				lua_pushinteger(L, (char)*(p++));
				++p;
			}
			break;
		case CSTRUCT_OP_UINT8:
			{
				lua_pushinteger(L, *(p++));
				++r;
			}
			break;
		case CSTRUCT_OP_INT16:
			{
				if (((uintptr_t)p & 1) == 0) {
					lua_pushinteger(L, *((int16_t*)p));
					p += 2;
				} else {
					num.b[1] = *(p++);
					num.b[0] = *(p++);
					lua_pushinteger(L, num.i16);
				}
				++r;
			}
			break;
		case CSTRUCT_OP_UINT16:
			{
				if (((uintptr_t)p & 1) == 0) {
					lua_pushinteger(L, *((uint16_t*)p));
					p += 2;
				} else {
					num.b[1] = *(p++);
					num.b[0] = *(p++);
					lua_pushinteger(L, num.u16);
				}
				++r;
			}
			break;
		case CSTRUCT_OP_INT32:
			{
				if (((uintptr_t)p & 3) == 0) {
					lua_pushinteger(L, *((int32_t*)p));
					p += 4;
				} else {
					num.b[3] = *(p++);
					num.b[2] = *(p++);
					num.b[1] = *(p++);
					num.b[0] = *(p++);
					lua_pushinteger(L, num.i32);
				}
				++r;
			}
			break;
		case CSTRUCT_OP_UINT32:
			{
				if (((uintptr_t)p & 3) == 0) {
					lua_pushinteger(L, *((uint32_t*)p));
					p += 4;
				} else {
					num.b[3] = *(p++);
					num.b[2] = *(p++);
					num.b[1] = *(p++);
					num.b[0] = *(p++);
					lua_pushinteger(L, num.i32);
				}
				++r;
			}
			break;
		case CSTRUCT_OP_INT64:
			{
				if (((uintptr_t)p & 7) == 0) {
					lua_pushinteger(L, *((int64_t*)p));
					p += 8;
				} else {
					num.b[7] = *(p++);
					num.b[6] = *(p++);
					num.b[5] = *(p++);
					num.b[4] = *(p++);
					num.b[3] = *(p++);
					num.b[2] = *(p++);
					num.b[1] = *(p++);
					num.b[0] = *(p++);
					lua_pushinteger(L, num.i64);
				}
				++r;
			}
			break;
		case CSTRUCT_OP_UINT64:
			{
				if (((uintptr_t)p & 7) == 0) {
					lua_pushinteger(L, *((uint64_t*)p));
					p += 8;
				} else {
					num.b[7] = *(p++);
					num.b[6] = *(p++);
					num.b[5] = *(p++);
					num.b[4] = *(p++);
					num.b[3] = *(p++);
					num.b[2] = *(p++);
					num.b[1] = *(p++);
					num.b[0] = *(p++);
					lua_pushinteger(L, num.i64);
				}
				++r;
			}
			break;
		case CSTRUCT_OP_FLOAT32:
			{
				if (((uintptr_t)p & 3) == 0) {
					lua_pushnumber(L, *((float*)p));
					p += 4;
				} else {
					num.b[3] = *(p++);
					num.b[2] = *(p++);
					num.b[1] = *(p++);
					num.b[0] = *(p++);
					lua_pushnumber(L, num.f32);
				}
				++r;
			}
			break;
		case CSTRUCT_OP_FLOAT64:
			{
				if (((uintptr_t)p & 7) == 0) {
					lua_pushnumber(L, *((double*)p));
					p += 4;
				} else {
					num.b[7] = *(p++);
					num.b[6] = *(p++);
					num.b[5] = *(p++);
					num.b[4] = *(p++);
					num.b[3] = *(p++);
					num.b[2] = *(p++);
					num.b[1] = *(p++);
					num.b[0] = *(p++);
					lua_pushnumber(L, num.f64);
				}
				++r;
			}
			break;
		case CSTRUCT_OP_SW_INT16:
			{
				num.b[1] = *(p++);
				num.b[0] = *(p++);
				lua_pushinteger(L, num.i16);
				++r;
			}
			break;
		case CSTRUCT_OP_SW_UINT16:
			{
				num.b[1] = *(p++);
				num.b[0] = *(p++);
				lua_pushinteger(L, num.u16);
				++r;
			}
			break;
		case CSTRUCT_OP_SW_INT32:
			{
				num.b[3] = *(p++);
				num.b[2] = *(p++);
				num.b[1] = *(p++);
				num.b[0] = *(p++);
				lua_pushinteger(L, num.i32);
				++r;
			}
			break;
		case CSTRUCT_OP_SW_UINT32:
			{
				num.b[3] = *(p++);
				num.b[2] = *(p++);
				num.b[1] = *(p++);
				num.b[0] = *(p++);
				lua_pushinteger(L, num.u32);
				++r;
			}
			break;
		case CSTRUCT_OP_SW_INT64:
			{
				num.b[7] = *(p++);
				num.b[6] = *(p++);
				num.b[5] = *(p++);
				num.b[4] = *(p++);
				num.b[3] = *(p++);
				num.b[2] = *(p++);
				num.b[1] = *(p++);
				num.b[0] = *(p++);
				lua_pushinteger(L, num.i64);
				++r;
			}
			break;
		case CSTRUCT_OP_SW_UINT64:
			{
				num.b[7] = *(p++);
				num.b[6] = *(p++);
				num.b[5] = *(p++);
				num.b[4] = *(p++);
				num.b[3] = *(p++);
				num.b[2] = *(p++);
				num.b[1] = *(p++);
				num.b[0] = *(p++);
				lua_pushinteger(L, num.u64);
				++r;
			}
			break;
		case CSTRUCT_OP_SW_FLOAT32:
			{
				num.b[3] = *(p++);
				num.b[2] = *(p++);
				num.b[1] = *(p++);
				num.b[0] = *(p++);
				lua_pushnumber(L, num.f32);
				++r;
			}
			break;
		case CSTRUCT_OP_SW_FLOAT64:
			{
				num.b[7] = *(p++);
				num.b[6] = *(p++);
				num.b[5] = *(p++);
				num.b[4] = *(p++);
				num.b[3] = *(p++);
				num.b[2] = *(p++);
				num.b[1] = *(p++);
				num.b[0] = *(p++);
				lua_pushnumber(L, num.f64);
				++r;
			}
			break;
		default:
			assert(0);
		}
	}

	return r;
}

static int L_bufs_new(lua_State* L) {
	cbufs_t* self = (cbufs_t*)lua_newuserdata(L, sizeof(cbufs_t));
	cbufs_init(self);
	luaL_setmetatable(L, L_BUFS_META);
	return 1;
}

static int L_bufs_gc(lua_State* L) {
	cbufs_t* self = (cbufs_t*)luaL_checkudata(L, 1, L_BUFS_META);
	cbufs_fini(self);
	return 0;
}

static int L_bufs_len(lua_State* L) {
	cbufs_t* self = (cbufs_t*)luaL_checkudata(L, 1, L_BUFS_META);
	lua_pushinteger(L, self->length);
	return 1;
}

static int L_bufs_slice(lua_State* L) {
	cbufs_t* self = (cbufs_t*)luaL_checkudata(L, 1, L_BUFS_META);
	// TODO
	(void)self;
	return 0;
}

static int L_bufs_append(lua_State* L) {
	cbufs_t* self = (cbufs_t*)luaL_checkudata(L, 1, L_BUFS_META);
	cbuf_t* buf = (cbuf_t*)luaL_checkudata(L, 2, L_BUF_META);
	cbufs_push(self, buf, 0);
	lua_settop(L, 1);
	return 1;
}

static int L_bufs_prepend(lua_State* L) {
	cbufs_t* self = (cbufs_t*)luaL_checkudata(L, 1, L_BUFS_META);
	cbuf_t* buf = (cbuf_t*)luaL_checkudata(L, 2, L_BUF_META);
	cbufs_push_front(self, buf, 0);
	lua_settop(L, 1);
	return 1;
}

static int L_bufs_concat(lua_State* L) {
	cbufs_t* self = (cbufs_t*)luaL_checkudata(L, 1, L_BUFS_META);
	cbufs_t* other = (cbufs_t*)luaL_checkudata(L, 2, L_BUFS_META);
	cbufs_concat(self, other);
	lua_settop(L, 1);
	return 1;
}

static int L_bufs_skip(lua_State* L) {
	cbufs_t* self = (cbufs_t*)luaL_checkudata(L, 1, L_BUFS_META);
	ssize_t n = luaL_optinteger(L, 2, -1);
	n = cbufs_shift(self, n, NULL);
	lua_pushinteger(L, n);
	return 1;
}

static int L_bufs_peek(lua_State* L) {
	cbufs_t* self = (cbufs_t*)luaL_checkudata(L, 1, L_BUFS_META);
	ssize_t n = luaL_optinteger(L, 2, -1);
	cbuf_t* target = (cbuf_t*)lua_newuserdata(L, sizeof(cbuf_t));
	luaL_setmetatable(L, L_BUF_META);
	cbufs_peek(self, n, target);
	return 1;
}

static int L_bufs_shift(lua_State* L) {
	cbufs_t* self = (cbufs_t*)luaL_checkudata(L, 1, L_BUFS_META);
	ssize_t n = luaL_optinteger(L, 2, -1);
	cbufs_t* target = (cbufs_t*)lua_newuserdata(L, sizeof(cbufs_t));
	cbufs_init(target);
	luaL_setmetatable(L, L_BUFS_META);
	cbufs_shift(self, n, target);
	return 1;
}

static int L_bufs_truncate(lua_State* L) {
	cbufs_t* self = (cbufs_t*)luaL_checkudata(L, 1, L_BUFS_META);
	ssize_t n = luaL_optinteger(L, 2, 0);
	cbufs_truncate(self, n);
	return 0;
}

static int L_find(lua_State* L) {
	int ch;
	ssize_t n;
	void* self;

	if (lua_isnumber(L, 2)) {
		ch = lua_tointeger(L, 2);
	} else if (lua_isstring(L, 2)) {
		ch = *lua_tostring(L, 2);
	} else {
		return luaL_argerror(L, 2, "integer or string expected.");
	}

	if ((self = luaL_testudata(L, 1, L_BUFS_META)) != NULL) {
		n = cbufs_find((cbufs_t*)self, ch);
	} else if ((self = luaL_testudata(L, 1, L_BUF_META)) != NULL) {
		n = cbuf_find((cbuf_t*)self, ch);
	} else {
		return luaL_argerror(L, 1, "cbuf.buf or cbuf.bufs value expected.");
	}

	lua_pushinteger(L, n);
	return 1;
}

EXPORT int luaopen_cbuf(lua_State* L) {
	static luaL_Reg struct_meta[] = {
		{ "__len", L_struct_len },
	};

	static luaL_Reg buf_meta[] = {
		{ "__gc", L_buf_gc },
		{ "__len", L_buf_len },
		{ "__call", L_buf_slice },
		{ NULL, NULL }
	};

	static luaL_Reg bufs_meta[] = {
		{ "__gc", L_bufs_gc },
		{ "__len", L_bufs_len },
		{ "__call", L_bufs_slice },
		{ NULL, NULL }
	};

	static const luaL_Reg functions[] = {
		{ "struct", L_struct_new },

		{ "buf", L_buf_new },
		{ "base", L_buf_base },
		{ "tostring", L_buf_tostring },
		{ "pack", L_buf_pack },
		{ "unpack", L_buf_unpack },

		{ "bufs", L_bufs_new },
		{ "append", L_bufs_append },
		{ "prepend", L_bufs_prepend },
		{ "concat", L_bufs_concat },
		{ "peek", L_bufs_peek },
		{ "skip", L_bufs_skip },
		{ "shift", L_bufs_shift },
		{ "truncate", L_bufs_truncate },

		{ "find", L_find },

		{ NULL, NULL }
	};

	luaL_newmetatable(L, L_STRUCT_META);
	luaL_setfuncs(L, struct_meta, 0);
	luaL_newmetatable(L, L_BUF_META);
	luaL_setfuncs(L, buf_meta, 0);
	luaL_newmetatable(L, L_BUFS_META);
	luaL_setfuncs(L, bufs_meta, 0);
	luaL_newlib(L, functions);
	return 1;
}

