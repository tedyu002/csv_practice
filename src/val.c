#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdio.h>
#include <errno.h>

#include "val.h"

int
val_parse(const char *src, type_t *type, val_t *val)
{
	long val_long;
	double val_double;
	char *end_ptr = NULL;
	struct tm tm;

	if (src == NULL || type == NULL || val == NULL) {
		errno = EINVAL;
		return -1;
	}

	val->type = *type;

	switch (type->type) {
		case INTEGER:
			val_long = strtol(src, &end_ptr, 10);
			if (*end_ptr != '\0') {
				errno = EINVAL;
				return -1;
			}
			val->val.int_v = (int)val_long;
			break;

		case BOOL:
			if (strcmp(src, "TRUE") == 0) {
				val->val.bool_v = true;
			}
			else if (strcmp(src, "FALSE") == 0) {
				val->val.bool_v = false;
			}
			else {
				errno = EINVAL;
				return -1;
			}

			break;

		case DOUBLE:
			val_double = strtod(src, &end_ptr);
			if (*end_ptr != '\0') {
				errno = EINVAL;
				return -1;
			}
			val->val.double_v = val_double;
			break;

		case CHAR:
			if (strlen(src) > type->len) {
				errno = EINVAL;
				return -1;
			}

			val->val.str_v.str = malloc(type->len + 1);
			if (val->val.str_v.str == NULL) {
				return -1;
			}

			snprintf(val->val.str_v.str, type->len + 1, "%*s", (int)type->len, src);
			break;

		case VARCHAR:
			if (strlen(src) > type->len) {
				errno = EINVAL;
				return -1;
			}

			val->val.str_v.str = strdup(src);
			if (val->val.str_v.str == NULL) {
				return -1;
			}
			break;

		case DATETIME:
			end_ptr = strptime(src, "%Y/%m/%d %H:%M:%S", &tm);
			if (end_ptr == NULL || *end_ptr != '\0') {
				errno = EINVAL;
				return -1;
			}
			val->val.datetime_v = mktime(&tm);
			break;
	}

	return 0;
}

void
val_destroy(val_t *val)
{
	if (val == NULL) {
		return;
	}

	if (val->type.type == VARCHAR || val->type.type == CHAR) {
		if (val->val.str_v.str != NULL) {
			free(val->val.str_v.str);
			val->val.str_v.str = NULL;
		}
	}
}

int
val_clone(const val_t *src_val, val_t *target_val)
{
	if (src_val == NULL || target_val == NULL) {
		errno = EINVAL;
		return -1;
	}

	if (src_val->type.type == VARCHAR || src_val->type.type == CHAR) {
		target_val->type = src_val->type;

		if ((target_val->val.str_v.str = strdup(src_val->val.str_v.str)) == NULL) {
			return -1;
		}
		target_val->val.str_v.len = src_val->val.str_v.len;
	}
	else {
		memcpy(target_val, src_val, sizeof(val_t));
	}

	return 0;
}

void
val_destroy_void(void *val)
{
	val_destroy((val_t*)val);
}

int
val_op_less(const val_t *a, const val_t *b, bool *res)
{
	if (a == NULL || b == NULL || res == NULL) {
		errno = EINVAL;
		return -1;
	}

	if (a->type.type != b->type.type) {
		errno = EINVAL;
		return -1;
	}

	switch (a->type.type) {
		case INTEGER:
			*res = a->val.int_v < b->val.int_v;
			break;
		case BOOL:
			*res = a->val.bool_v < b->val.bool_v;
			break;
		case DOUBLE:
			*res = a->val.double_v < b->val.double_v;
			break;
		case DATETIME:
			*res = difftime(a->val.datetime_v, b->val.datetime_v) < 0;
			break;
		case CHAR:
		case VARCHAR:
			*res = strcmp(a->val.str_v.str, b->val.str_v.str) < 0;
			break;
	}

	return 0;
}
