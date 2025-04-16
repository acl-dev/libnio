#include "stdafx.h"
#include "nio_memory.h"

//#define DEBUG_MEM
#ifdef DEBUG_MEM
static __thread unsigned long long __nmalloc  = 0;
static __thread unsigned long long __ncalloc  = 0;
static __thread unsigned long long __nstrdup  = 0;
static __thread unsigned long long __nrealloc = 0;
static __thread unsigned long long __nfree    = 0;
#endif

void *nio_mem_malloc(size_t size)
{
#ifdef DEBUG_MEM
	__nmalloc++;
#endif
	return malloc(size);
}

void nio_mem_free(void *ptr)
{
#ifdef DEBUG_MEM
	__nfree++;
#endif
	free(ptr);
}

void *nio_mem_calloc(size_t nmemb, size_t size)
{
#ifdef DEBUG_MEM
	__ncalloc++;
#endif
	return calloc(nmemb, size);
}

void *nio_mem_realloc(void *ptr, size_t size)
{
#ifdef DEBUG_MEM
	__nrealloc++;
#endif
	return realloc(ptr, size);
}

char *niio_mem_strdup(const char *s)
{
#ifdef DEBUG_MEM
	__nstrdup++;
#endif
#if defined(_WIN32) || defined(_WIN64)
	return _strdup(s);
#else
	return strdup(s);
#endif
}

void nio_mem_stat(void)
{
#ifdef DEBUG_MEM
	printf("malloc=%llu, calloc=%llu, strdup=%llu, realloc=%llu, "
		"free=%llu, diff=%llu\r\n", __nmalloc, __ncalloc, __nstrdup,
		__nrealloc, __nfree, __nmalloc + __ncalloc + __nstrdup - __nfree);
#endif
}
