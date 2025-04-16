#ifndef __MEMORY_HEAD_H__
#define __MEMORY_HEAD_H__

void *nio_mem_malloc(size_t size);
void nio_mem_free(void *ptr);
void *nio_mem_calloc(size_t nmemb, size_t size);
void *nio_mem_realloc(void *ptr, size_t size);
void nio_mem_stat(void);
char *nio_mem_strdup(const char *s);

#endif
