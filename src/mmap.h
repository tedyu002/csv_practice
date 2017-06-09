#ifndef __TZUTE_MMAP_H
#define __TZUTE_MMAP_H

#include <stddef.h>

extern int mmap_alloc(const char *path, void **ptr, size_t *size);
extern int mmap_free(void *ptr, size_t size);

#endif
