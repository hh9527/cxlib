#ifndef __CX_BASE_H__
#define __CX_BASE_H__

#ifdef _WIN32
# include <windows.h>
#endif

#include <sys/types.h>
#include <stddef.h>
#include <stdint.h>

#ifndef CX_API
# define CX_API extern
#endif

#ifdef CX_WITH_UV
# include "uv.h"
# define cx_buf_t uv_buf_t
#else
typedef struct {
#ifdef _WIN32
	ULONG len;
	char* base;
#else
	char* base;
	size_t len;
#endif
} cx_buf_t;
#endif

#define CX_NEW(malloc, type, xlen) (type*)malloc(sizeof(type) + xlen)
#define CX_NEW2(malloc, type, member, xlen) (type*)malloc(offsetof(type, member) + xlen)
#define CX_GET_SELF(ptr, type, member) ((type*)((char*)((void*)(ptr)) - offsetof(type, member)))

#endif

