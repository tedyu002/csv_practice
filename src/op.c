#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdio.h>
#include <errno.h>

#include "op.h"

static int op_integer_add(const val_t *a, const val_t *b, val_t *t);
static int op_integer_multi(const val_t *a, const val_t *b, val_t *t);

static int op_bool_add(const val_t *a, const val_t *b, val_t *t);
static int op_bool_multi(const val_t *a, const val_t *b, val_t *t);

static int op_double_add(const val_t *a, const val_t *b, val_t *t);
static int op_double_multi(const val_t *a, const val_t *b, val_t *t);

static int op_str_add(const val_t *a, const val_t *b, val_t *t);
static int op_str_multi(const val_t *a, const val_t *b, val_t *t);

static int op_datetime_add(const val_t *a, const val_t *b, val_t *t);
static int op_datetime_multi(const val_t *a, const val_t *b, val_t *t);

op_two_t type_operators[TYPE_NUM][OPERATOR_NUM] = {
	{op_integer_add, op_integer_multi},
	{op_bool_add, op_bool_multi},
	{op_double_add, op_double_multi},
	{op_str_add, op_str_multi},
	{op_str_add, op_str_multi},
	{op_datetime_add, op_datetime_multi},
};

static int
op_integer_add(const val_t *a, const val_t *b, val_t *t)
{
	FILE *fp = NULL;
	char *buf = NULL;
	size_t buf_len = 0;
	int save_errno = 0;

	if (a == NULL || b == NULL || t == NULL) {
		errno = EINVAL;
		return -1;
	}

	if (a->type.type != INTEGER) {
		errno = EINVAL;
		return -1;
	}

	switch (b->type.type) {
		case INTEGER:
			t->type.type = INTEGER;
			t->val.int_v = a->val.int_v + b->val.int_v;
			break;
		case BOOL:
			t->type.type = INTEGER;
			t->val.int_v = a->val.int_v + (b->val.bool_v ? 1 : 0);
			break;
		case DOUBLE:
			t->type.type = DOUBLE;
			t->val.double_v = a->val.int_v + b->val.double_v;
			break;
		case CHAR:
		case VARCHAR:
			fp = open_memstream(&buf, &buf_len);
			if (fp == NULL) {
				save_errno = errno;
				goto end;
			}
			if (fprintf(fp, "%d%s", a->val.int_v, b->val.str_v.str) < 0) {
				save_errno = errno;
				goto end;
			}
			if (fclose(fp) == EOF) {
				save_errno = errno;
				goto end;
			}
			fp = NULL;

			t->type.type = VARCHAR;
			t->val.str_v.str = buf;
			t->val.str_v.len = buf_len;
			buf = NULL;
			break;
		case DATETIME:
			save_errno = EINVAL;
			goto end;
	}

end:
	if (fp != NULL) {
		fclose(fp);
		fp = NULL;
	}

	if (buf != NULL) {
		free(buf);
		buf = NULL;
	}

	if (save_errno != 0) {
		errno = save_errno;
		return -1;
	}
	return 0;
}

static int
op_integer_multi(const val_t *a, const val_t *b, val_t *t)
{
	FILE *fp = NULL;
	char *buf = NULL;
	size_t buf_len = 0;
	int save_errno = 0;

	if (a == NULL || b == NULL || t == NULL) {
		errno = EINVAL;
		return -1;
	}

	if (a->type.type != INTEGER) {
		errno = EINVAL;
		return -1;
	}

	switch (b->type.type) {
		case INTEGER:
			t->type.type = INTEGER;
			t->val.int_v = a->val.int_v * b->val.int_v;
			break;
		case BOOL:
			t->type.type = INTEGER;
			t->val.int_v = a->val.int_v * (b->val.bool_v ? 1 : 0);
			break;
		case DOUBLE:
			t->type.type = DOUBLE;
			t->val.double_v = a->val.int_v * b->val.double_v;
			break;
		case CHAR:
		case VARCHAR:
			fp = open_memstream(&buf, &buf_len);
			if (fp == NULL) {
				save_errno = errno;
				goto end;
			}
			for (int i = 0 ; i < a->val.int_v; ++i) {
				if (fprintf(fp, "%s", b->val.str_v.str) < 0) {
					save_errno = errno;
					goto end;
				}
			}
			if (fclose(fp) == EOF) {
				save_errno = errno;
				goto end;
			}
			fp = NULL;

			t->type.type = VARCHAR;
			t->val.str_v.str = buf;
			t->val.str_v.len = buf_len;
			buf = NULL;
			break;
		case DATETIME:
			save_errno = EINVAL;
			goto end;
	}

end:
	if (fp != NULL) {
		fclose(fp);
		fp = NULL;
	}

	if (buf != NULL) {
		free(buf);
		buf = NULL;
	}

	if (save_errno != 0) {
		errno = save_errno;
		return -1;
	}
	return 0;
}

static int
op_bool_add(const val_t *a, const val_t *b, val_t *t)
{
	if (a == NULL || b == NULL || t == NULL) {
		errno = EINVAL;
		return -1;
	}

	if (a->type.type != BOOL) {
		errno = EINVAL;
		return -1;
	}

	val_t tmp_val;
	tmp_val.type.type = INTEGER;
	tmp_val.val.int_v = a->val.bool_v ? 1 : 0;

	return op_integer_add(&tmp_val, b, t);
}

static int
op_bool_multi(const val_t *a, const val_t *b, val_t *t)
{
	if (a == NULL || b == NULL || t == NULL) {
		errno = EINVAL;
		return -1;
	}

	if (a->type.type != BOOL) {
		errno = EINVAL;
		return -1;
	}

	val_t tmp_val;
	tmp_val.type.type = INTEGER;
	tmp_val.val.int_v = a->val.bool_v ? 1 : 0;

	return op_integer_multi(&tmp_val, b, t);
}

static int
op_double_add(const val_t *a, const val_t *b, val_t *t)
{
	FILE *fp = NULL;
	char *buf = NULL;
	size_t buf_len = 0;
	int save_errno = 0;

	if (a == NULL || b == NULL || t == NULL) {
		errno = EINVAL;
		return -1;
	}

	if (a->type.type != DOUBLE) {
		errno = EINVAL;
		return -1;
	}

	switch (b->type.type) {
		case INTEGER:
			t->type.type = DOUBLE;
			t->val.double_v = a->val.double_v + b->val.int_v;
			break;
		case BOOL:
			t->type.type = DOUBLE;
			t->val.double_v = a->val.double_v + (b->val.bool_v ? 1 : 0);
			break;
		case DOUBLE:
			t->type.type = DOUBLE;
			t->val.double_v = a->val.double_v + b->val.double_v;
			break;
		case CHAR:
		case VARCHAR:
			fp = open_memstream(&buf, &buf_len);
			if (fp == NULL) {
				save_errno = errno;
				goto end;
			}
			if (fprintf(fp, "%.2f%s", a->val.double_v, b->val.str_v.str) < 0) {
				save_errno = errno;
				goto end;
			}
			if (fclose(fp) == EOF) {
				save_errno = errno;
				goto end;
			}
			fp = NULL;

			t->type.type = VARCHAR;
			t->val.str_v.str = buf;
			t->val.str_v.len = buf_len;
			buf = NULL;
			break;
		case DATETIME:
			save_errno = EINVAL;
			goto end;
	}

end:
	if (fp != NULL) {
		fclose(fp);
		fp = NULL;
	}

	if (buf != NULL) {
		free(buf);
		buf = NULL;
	}

	if (save_errno != 0) {
		errno = save_errno;
		return -1;
	}
	return 0;
}

static int
op_double_multi(const val_t *a, const val_t *b, val_t *t)
{
	int save_errno = 0;

	if (a == NULL || b == NULL || t == NULL) {
		errno = EINVAL;
		return -1;
	}

	if (a->type.type != DOUBLE) {
		errno = EINVAL;
		return -1;
	}

	switch (b->type.type) {
		case INTEGER:
			t->type.type = DOUBLE;
			t->val.double_v = a->val.double_v * b->val.int_v;
			break;
		case BOOL:
			t->type.type = DOUBLE;
			t->val.double_v = a->val.double_v * (b->val.bool_v ? 1 : 0);
			break;
		case DOUBLE:
			t->type.type = DOUBLE;
			t->val.double_v = a->val.double_v * b->val.double_v;
			break;
		case CHAR:
		case VARCHAR:
			save_errno = EINVAL;
			goto end;
		case DATETIME:
			save_errno = EINVAL;
			goto end;
	}

end:
	if (save_errno != 0) {
		errno = save_errno;
		return -1;
	}

	return 0;
}

static int
op_str_add(const val_t *a, const val_t *b, val_t *t)
{
	FILE *fp = NULL;
	char *buf = NULL;
	size_t buf_len = 0;
	int save_errno = 0;
	int res = 0;

	if (a == NULL || b == NULL || t == NULL) {
		errno = EINVAL;
		return -1;
	}

	if (a->type.type != VARCHAR && a->type.type != CHAR) {
		errno = EINVAL;
		return -1;
	}

	fp = open_memstream(&buf, &buf_len);
	if (fp == NULL) {
		save_errno = errno;
		goto end;
	}

	t->type.type = VARCHAR;

	switch (b->type.type) {
		case INTEGER:
			res = fprintf(fp, "%s%d", a->val.str_v.str, b->val.int_v);
			break;
		case BOOL:
			res = fprintf(fp, "%s%d", a->val.str_v.str, b->val.bool_v ? 1 : 0);
			break;
		case DOUBLE:
			res = fprintf(fp, "%s%.2f", a->val.str_v.str, b->val.double_v);
			break;
		case CHAR:
		case VARCHAR:
			res = fprintf(fp, "%s%s", a->val.str_v.str, b->val.str_v.str);
			break;
		case DATETIME:
			save_errno = EINVAL;
			goto end;
	}

	if (res < 0) {
		save_errno = errno;
		goto end;
	}

	if (fclose(fp) == EOF) {
		save_errno = errno;
		goto end;
	}
	fp = NULL;

	t->val.str_v.str = buf;
	t->val.str_v.len = buf_len;
	buf = NULL;

end:
	if (fp != NULL) {
		fclose(fp);
		fp = NULL;
	}

	if (buf != NULL) {
		free(buf);
		buf = NULL;
	}

	if (save_errno != 0) {
		errno = save_errno;
		return -1;
	}
	return 0;
}

static int
op_str_multi(const val_t *a, const val_t *b, val_t *t)
{

	if (a == NULL || b == NULL || t == NULL) {
		errno = EINVAL;
		return -1;
	}

	if (a->type.type != VARCHAR && a->type.type != CHAR) {
		errno = EINVAL;
		return -1;
	}

	if (b->type.type == INTEGER) {
		return op_integer_multi(b, a, t);
	}
	else if (b->type.type == BOOL) {
		return op_bool_multi(b, a, t);
	}
	else {
		errno = EINVAL;
		return -1;
	}
}

static int
op_datetime_add(const val_t *a, const val_t *b, val_t *t)
{
	errno = EINVAL;
	return -1;
}

static int
op_datetime_multi(const val_t *a, const val_t *b, val_t *t)
{
	errno = EINVAL;
	return -1;
}
