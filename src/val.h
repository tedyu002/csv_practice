#ifndef __TZUTE_VAL_H
#define __TZUTE_VAL_H

#include <stdbool.h>
#include <time.h>
#include <stdlib.h>

#include "type.h"

typedef struct {
	type_t type;
	union {
		int int_v;
		bool bool_v;
		double double_v;
		string_t str_v;
		time_t datetime_v;
	} val;
} val_t;

extern void val_destroy(val_t *val);
extern void val_destroy_void(void *val);
extern int val_parse(const char *src, type_t *type, val_t *val);
extern int val_clone(const val_t *src_val, val_t *target_val);

extern int val_op_less(const val_t *a, const val_t *b, bool *res);

#endif
