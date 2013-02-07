#include <lauxlib.h>

#include "cbuf.h"

#define L_CBUF_META "cbuf.cbuf"
#define L_CBUFS_META "cbuf.cbufs"

static int L_cbuf_new(lua_State* L) {
	if (lua_isnumber(L, 1)) {
		cbuf_t* self = (cbuf_t*)lua_newuserdata(L, sizeof(cbuf_t));
		ssize_t l = (ssize_t)lua_tointeger(L, 1);
		cbuf_init2(self, l);
	} else if (lua_isstring(L, 1)) {
		cbuf_t* self = (cbuf_t*)lua_newuserdata(L, sizeof(cbuf_t));
		size_t l;
		const char* s = luaL_tolstring(L, 1, &l);
		cbuf_init(self, s, (ssize_t)l);
	} else {
		luaL_argerror(L, 1, "Integer or string value expected.");
	}

	luaL_setmetatable(L, L_CBUF_META);
	return 1;
}

static int L_cbuf_gc(lua_State* L) {
	cbuf_t* self = (cbuf_t*)luaL_checkudata(L, 1, L_CBUF_META);
	cbuf_fini(self);
	return 0;
}

static int L_cbuf_len(lua_State* L) {
	cbuf_t* self = (cbuf_t*)luaL_checkudata(L, 1, L_CBUF_META);
	lua_pushinteger(L, self->end - self->start);
	return 1;
}

static int L_cbuf_base(lua_State* L) {
	cbuf_t* self = (cbuf_t*)luaL_checkudata(L, 1, L_CBUF_META);
	lua_pushlightuserdata(L, cbuf_base(self));
	return 1;
}

static int L_cbuf_slice(lua_State* L) {
	cbuf_t* self = (cbuf_t*)luaL_checkudata(L, 1, L_CBUF_META);
	int length = self->end - self->start;
	int start = luaL_optint(L, 1, 0);
	int end = luaL_optint(L, 2, length);
	if (start == 0 && end == length) {
		lua_settop(L, 1);
	} else {
		cbuf_t* obj = (cbuf_t*)lua_newuserdata(L, sizeof(cbuf_t));
		*obj = cbuf_slice(self, start, end, 0);
		luaL_setmetatable(L, L_CBUF_META);
	}

	return 1;
}

static int L_cbuf_find(lua_State* L) {
	cbuf_t* self = (cbuf_t*)luaL_checkudata(L, 1, L_CBUF_META);
	int ch;
	ssize_t n;
	if (lua_isnumber(L, 2))
		ch = lua_tointeger(L, 2);
	else if (lua_isstring(L, 2))
		ch = *lua_tostring(L, 2);
	else
		luaL_argerror(L, 2, "Integer or string expected.");
	n = cbuf_find(self, ch);
	lua_pushinteger(L, n);
	return 1;
}

static int L_init_cbuf(lua_State* L) {
	static luaL_Reg methods[] = {
		{ "__gc", L_cbuf_gc },
		{ "__len", L_cbuf_len },
		{ "base", L_cbuf_base },
		{ "slice", L_cbuf_slice },
		{ "find", L_cbuf_find },
		{ NULL, NULL }
	};

	luaL_newmetatable(L, L_CBUF_META);
	luaL_setfuncs(L, methods, 0);
	return 1;
}

static int L_cbufs_new(lua_State* L) {
	cbufs_t* self = (cbufs_t*)lua_newuserdata(L, sizeof(cbufs_t));
	cbufs_init(self);
	luaL_setmetatable(L, L_CBUFS_META);
	return 1;
}

static int L_cbufs_gc(lua_State* L) {
	cbufs_t* self = (cbufs_t*)luaL_checkudata(L, 1, L_CBUFS_META);
	cbufs_fini(self);
	return 0;
}

static int L_cbufs_len(lua_State* L) {
	cbufs_t* self = (cbufs_t*)luaL_checkudata(L, 1, L_CBUFS_META);
	lua_pushinteger(L, self->length);
	return 1;
}

static int L_cbufs_find(lua_State* L) {
	cbufs_t* self = (cbufs_t*)luaL_checkudata(L, 1, L_CBUFS_META);
	int ch;
	ssize_t n;
	if (lua_isnumber(L, 2))
		ch = lua_tointeger(L, 2);
	else if (lua_isstring(L, 2))
		ch = *lua_tostring(L, 2);
	else
		luaL_argerror(L, 2, "Integer or string expected.");
	n = cbufs_find(self, ch);
	lua_pushinteger(L, n);
	return 1;
}

static int L_init_cbufs(lua_State* L) {
	static luaL_Reg methods[] = {
		{ "__gc", L_cbufs_gc },
		{ "__len", L_cbufs_len },
		{ "find", L_cbufs_find },
		{ NULL, NULL }
	};

	luaL_newmetatable(L, L_CBUFS_META);
	luaL_setfuncs(L, methods, 0);
	return 1;
}

extern int luaopen_cbuf(lua_State* L) {
	static const luaL_Reg methods[] = {
		{ "newbuf", L_cbuf_new },
		{ "newbufs", L_cbufs_new },
		{ NULL, NULL }
	};

	// fprintf(stderr, "top=%d", lua_gettop(L));
	L_init_cbuf(L);
	// fprintf(stderr, "top1=%d", lua_gettop(L));
	L_init_cbufs(L);
	// fprintf(stderr, "top2=%d", lua_gettop(L));
	luaL_newlib(L, methods);
	// fprintf(stderr, "top3=%d", lua_gettop(L));
	return 1;
}

