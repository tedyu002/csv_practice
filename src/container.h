#ifndef __TZUTE_CONTAINER_H
#define __TZUTE_CONTAINER_H

#include <stdbool.h>
#include <stddef.h>

typedef struct {
	void *data;
	size_t ele_size;
	size_t len;
	size_t size;
	void (*dealloc)(void *data);
} array_t;

#define ARRAY_GET(array, type, idx) (type*)((char*)(array)->data + sizeof(type) * (idx))

int array_init(array_t *array, size_t ele_size, void (*dealloc)(void *data));
int array_add(array_t *array, const void *data);
int array_del_last(array_t *array, bool dealloc);
void array_destroy(array_t *array);
void array_destroy_void(void *array);

#endif
