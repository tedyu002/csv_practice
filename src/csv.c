#include <errno.h>
#include <string.h>
#include <assert.h>

#include "config.h"
#include "token.h"
#include "csv.h"

typedef struct sort_data{
	size_t row_id;
	val_t *row;
} sort_data_t;

static int row_cmp(const void *v1, const void *v2, void *config_void);

int
csv_parse(config_t *config, const char **src, val_t **csv, size_t *csv_num)
{
	val_t *tmp_csv = NULL;
	size_t tmp_csv_num = 0;
	size_t tmp_csv_size = 32;
	const char *tmp_src = NULL;
	int save_errno = 0;
	val_t *res_row = NULL;

	tmp_src = *src;

	if (config == NULL || src == NULL || csv == NULL || csv_num == NULL) {
		return -1;
	}

	if ((tmp_csv = (val_t*)malloc(sizeof(val_t) * config->header_num * tmp_csv_size)) == NULL) {
		return -1;
	}

	while (true) {
		const char *err = NULL;

		if (*tmp_src == '\0') {
			break;
		}

		if (csv_row_parse(config, &tmp_src, &err, &res_row) == -1) {
			const char *line_end = err;
			if (token_to_line_feed(&line_end) == -1) {
				save_errno = errno;
				goto end;
			}
			fwrite(tmp_src, line_end - tmp_src, 1, config->err_fp);
			tmp_src = line_end;
			continue;
		}

		if (token_to_line_feed(&tmp_src) == -1) {
			save_errno = errno;
			goto end;
		}

		if (tmp_csv_num > tmp_csv_size) {
			tmp_csv_size *= 2;
			val_t *new_csv = realloc(tmp_csv, tmp_csv_size * config->header_num * sizeof(val_t));

			if (new_csv == NULL) {
				save_errno = errno;
				goto end;
			}
			tmp_csv = new_csv;
		}

		memcpy(tmp_csv + tmp_csv_num * config->header_num, res_row, config->header_num * sizeof(val_t));
		tmp_csv_num++;
		free(res_row);
		res_row = NULL;
	}

	*csv = tmp_csv;
	*csv_num = tmp_csv_num;
	tmp_csv = NULL;

end:
	if (res_row != NULL) {
		for (size_t i = 0; i < config->header_num; ++i) {
			val_destroy(&res_row[i]);
		}
		free(res_row);
		res_row = NULL;
	}

	if (tmp_csv != NULL) {
		for (size_t i = 0; i < tmp_csv_num; ++i) {
			val_destroy(&tmp_csv[i]);
		}
		free(tmp_csv);
		tmp_csv = NULL;
	}

	if (save_errno != 0) {
		errno = save_errno;
		return -1;
	}

	return 0;
}

int
csv_row_parse(config_t *config, const char **src, const char **err, val_t **row)
{
	val_t *tmp_row = NULL;
	int cur_col_num = 0;
	int save_errno = 0;
	const char *tmp_src = NULL;
	char *col_str = NULL;

	if (config == NULL || src == NULL || err == NULL || row == NULL) {
		errno = EINVAL;
		return -1;
	}

	tmp_row = (val_t*)calloc(config->header_num, sizeof(val_t));
	if (tmp_row == NULL) {
		return -1;
	}

	tmp_src = *src;
	while (cur_col_num < config->header_num) {
		if (token_column_string_get(&tmp_src, &col_str) == -1) {
			save_errno = EINVAL;
			goto end;
		}

		if (val_parse(col_str, &config->header[cur_col_num].type, &tmp_row[cur_col_num]) == -1) {
			save_errno = errno;
			goto end;
		}
		free(col_str);
		col_str = NULL;
		cur_col_num++;

		if (cur_col_num < config->header_num) {
			char letter;

			if (token_letter_get(&tmp_src, &letter) == -1) {
				save_errno = errno;
				goto end;
			}

			if (letter != ',') {
				save_errno = EINVAL;
				goto end;
			}
		}
	}

	*src = tmp_src;
	*row = tmp_row;
	tmp_row = NULL;

end:
	if (col_str != NULL) {
		free(col_str);
		col_str = NULL;
	}

	if (tmp_row != NULL) {
		for (size_t i = 0; i < cur_col_num; ++i) {
			val_destroy(&tmp_row[i]);
		}
		free(tmp_row);
	}

	if (save_errno != 0) {
		*err = tmp_src;
		errno = save_errno;
		return -1;
	}

	*err = NULL;
	return 0;
}

int
csv_sort(config_t *config, val_t *csv, size_t csv_num)
{
	val_t *tmp_row = NULL;
	size_t *target_idx = NULL;
	size_t row_size = 0;
	int save_errno = 0;

	if (config == NULL || csv == NULL) {
		errno = EINVAL;
		return -1;
	}

	sort_data_t *sort_data = NULL;
	if ((sort_data = (sort_data_t*)malloc(sizeof(sort_data_t) * csv_num)) == NULL) {
		save_errno = errno;
		goto end;
	}

	for (size_t i = 0; i < csv_num; ++i) {
		sort_data[i].row_id = i;
		sort_data[i].row = csv + i * config->header_num;
	}

	qsort_r(sort_data, csv_num, sizeof(sort_data_t), row_cmp, config);

	if ((target_idx = (size_t*)malloc(sizeof(size_t) * csv_num)) == NULL) {
		save_errno = errno;
		goto end;
	}

	for (size_t i = 0; i < csv_num; ++i) {
		target_idx[sort_data[i].row_id] = i;
	}

	row_size = config->header_num * sizeof(val_t);
	if ((tmp_row = (val_t*)malloc(row_size)) == NULL) {
		save_errno = errno;
		goto end;
	}

	for (size_t i = 0; i < csv_num; ++i) {
		while (i != target_idx[i]) {
			size_t target = target_idx[i];
			size_t tmp_target_idx = 0;

			memcpy(tmp_row, csv + config->header_num * target, row_size);
			memcpy(csv + config->header_num * target, csv + config->header_num * i, row_size);
			memcpy(csv + config->header_num * i, tmp_row, row_size);

			tmp_target_idx = target_idx[i];
			target_idx[i] = target_idx[target];
			target_idx[target] = tmp_target_idx;
		}
	}

end:
	if (sort_data != NULL) {
		free(sort_data);
		sort_data = NULL;
	}

	if (target_idx != NULL) {
		free(target_idx);
		target_idx = NULL;
	}

	if (tmp_row != NULL) {
		free(tmp_row);
		tmp_row = NULL;
	}

	if (save_errno != 0) {
		errno = save_errno;
		return -1;
	}

	return 0;
}

static int
row_cmp(const void *v1, const void *v2, void *config_void)
{
	config_t *config = (config_t*)config_void;
	const sort_data_t *d_1 = (const sort_data_t*)v1;
	const sort_data_t *d_2 = (const sort_data_t*)v2;

	const val_t *row_1 = (const val_t*)d_1->row;
	const val_t *row_2 = (const val_t*)d_2->row;

	for (size_t i = 0; i < config->sort_num; ++i) {
		int sort_col = config->sort_header[i];
		const val_t *col_1 = row_1 + sort_col;
		const val_t *col_2 = row_2 + sort_col;
		bool less = false;

		assert(val_op_less(col_1, col_2, &less) == 0);
		if (less) {
			if (config->sort_order == ASC) {
				return -1;
			}
			else {
				return 1;
			}
		}

		assert(val_op_less(col_2, col_1, &less) == 0);
		if (less) {
			if (config->sort_order == ASC) {
				return 1;
			}
			else {
				return -1;
			}
		}
	}

	if (config->sort_order == ASC) {
		if (d_1->row_id < d_2->row_id) {
			return -1;
		}
		else {
			return 1;
		}
	}
	else {
		if (d_1->row_id < d_2->row_id) {
			return 1;
		}
		else {
			return -1;
		}
	}

	return 0;
}

int
csv_cal(config_t *config, val_t *csv, size_t csv_num, array_t *formula, val_t *res)
{
	array_t result;
	bool result_valid = false;
	int save_errno = 0;
	val_t cal_err_val = {.type = {.type = INTEGER}, .val = {.int_v = -1}};

	if (array_init(&result, sizeof(val_t), val_destroy_void) == -1) {
		return -1;
	}
	result_valid = true;

	for (size_t i = 0; i < formula->len; ++i) {
		formula_element_t *formula_element = ARRAY_GET(formula, formula_element_t, i);
		if (formula_element->is_op) {
			val_t *v1 = NULL;
			val_t *v2 = NULL;
			val_t vt;
			val_t *st = &vt;

			if (result.len < 2) {
				save_errno = EINVAL;
				goto end;
			}

			v1 = ARRAY_GET(&result, val_t, result.len - 2);
			v2 = ARRAY_GET(&result, val_t, result.len - 1);

			if (type_operators[v1->type.type][formula_element->val.op](v1, v2, &vt) == -1) {
				st = &cal_err_val;
			}

			if (array_del_last(&result, true) == -1) {
				save_errno = errno;
				goto end;
			}
			if (array_del_last(&result, true) == -1) {
				save_errno = errno;
				goto end;
			}

			if (array_add(&result, st) == -1) {
				save_errno = errno;
				goto end;
			}
		}
		else {
			if (formula_element->val.operand.is_constant) {
				if (array_add(&result, &formula_element->val.operand.val.constant) == -1) {
					save_errno = errno;
					goto end;
				}
			}
			else {
				size_t col = formula_element->val.operand.val.variable.col;
				size_t row = formula_element->val.operand.val.variable.row_num;

				val_t *src_val = csv + row * config->header_num + col;
				val_t clone_val;

				if (val_clone(src_val, &clone_val) == -1) {
					save_errno = errno;
					goto end;
				}

				if (array_add(&result, &clone_val) == -1) {
					save_errno = errno;
					goto end;
				}
			}
		}
	}

	if (result.len != 1) {
		save_errno = EINVAL;
		goto end;
	}

	*res = *ARRAY_GET(&result, val_t, 0);
	if (array_del_last(&result, false) == -1) {
		save_errno = errno;
		goto end;
	}

end:
	if (result_valid) {
		array_destroy(&result);
	}

	if (save_errno != 0) {
		errno = save_errno;
		return -1;
	}

	return 0;
}
