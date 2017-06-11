#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "container.h"

int
array_init(array_t *array, size_t ele_size, void (*dealloc)(void *data))
{
	if (array == NULL || ele_size == 0) {
		errno = EINVAL;
		return -1;
	}

	array->data = NULL;
	array->ele_size = ele_size;
	array->len = 0;
	array->size = 32;
	array->dealloc = dealloc;

	if ((array->data = malloc(ele_size * array->size)) == NULL) {
		return -1;
	}

	return 0;
}

int
array_add(array_t *array, const void *data)
{
	if (array == NULL || data == NULL) {
		errno = EINVAL;
		return -1;
	}

	while (array->len >= array->size) {
		void *new_data = NULL;

		array->size *= 2;
		if ((new_data = realloc(array->data, array->ele_size * array->size)) == NULL) {
			return -1;
		}

		array->data = new_data;
	}

	memcpy((char*)array->data + array->len * array->ele_size, data, array->ele_size);
	array->len++;

	return 0;
}

int
array_del_last(array_t *array, bool dealloc)
{
	if (array == NULL) {
		errno = EINVAL;
		return -1;
	}

	if (array->len == 0) {
		errno = EINVAL;
		return -1;
	}

	array->len -= 1;

	if (dealloc && array->dealloc != NULL) {
		array->dealloc((char*)array->data + array->ele_size * array->len);
	}

	return 0;
}

void
array_destroy(array_t *array)
{
	if (array == NULL) {
		return;
	}

	if (array->dealloc != NULL) {
		for (size_t i = 0; i < array->len; ++i) {
			array->dealloc((char*)array->data + array->ele_size * i);
		}
	}

	if (array->data != NULL) {
		free(array->data);
		array->data = NULL;
	}
}

void
array_destroy_void(void *array)
{
	if (array != NULL) {
		array_destroy((array_t*)array);
	}
}
