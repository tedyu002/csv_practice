#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <stdint.h>
#include <ctype.h>

#include "mmap.h"
#include "token.h"
#include "type.h"
#include "val.h"
#include "config.h"

#define COL_CMP(col, val, val_len) (strlen(col) == val_len && strncmp(col, val, strlen(col)) == 0)

static int parse_header(config_t *config, const char *data);
static int parse_sort_headers(config_t *config, const char *field);
static int parse_formula(config_t *config, const char *src);
static void raw_formula_free(void *v);
static bool is_valid_header_name(config_t *config, const char *header);

int
config_parse(const char *path, config_t *config)
{
	void *config_mem = NULL;
	const char *config_ptr = NULL;
	size_t config_size;
	int save_errno = 0;
	char *sort_header = NULL;
	char *data = NULL;
	array_t raw_formulas;
	bool raw_formulas_init = false;

	if (path == NULL || config == NULL) {
		save_errno = EINVAL;
		goto end;
	}

	memset(config, 0, sizeof(*config));

	if (array_init(&config->formulas, sizeof(array_t), array_destroy_void) == -1) {
		save_errno = errno;
		goto end;
	}

	if (array_init(&raw_formulas, sizeof(char*), raw_formula_free) == -1) {
		save_errno = errno;
		goto end;
	}

	if (mmap_alloc(path, &config_mem, &config_size) == -1) {
		save_errno = errno;
		goto end;
	}

	config_ptr = (char*)config_mem;

	while (*config_ptr != '\0') {
		const char *line_end = NULL;
		const char *line_sep = NULL;
		size_t col_len = 0;
		int res = 0;

		line_end = strchr(config_ptr, '\n');
		line_sep = strchr(config_ptr, '=');

		if (line_end == NULL) {
			line_end = config_ptr + strlen(config_ptr);
		}

		if (line_sep == NULL || line_sep >= line_end) {
			save_errno = EINVAL;
			goto end;
		}

		col_len = line_sep - config_ptr;

		if (COL_CMP("CSV_INPUT", config_ptr, col_len)) {
			config_ptr = line_sep + 1;
			if (token_string_get(&config_ptr, &data) == -1) {
				save_errno = errno;
				fprintf(stderr, "Failed to parse CSV_INPUT '%s'.\n", data);
				goto end;
			}
			res = snprintf(config->input_file, sizeof(config->input_file), "%s", data);

			if (res < 0 || res > sizeof(config->input_file)) {
				save_errno = EINVAL;
				goto end;
			}
		}
		else if (COL_CMP("CSV_OUTPUT", config_ptr, col_len)) {
			config_ptr = line_sep + 1;
			if (token_string_get(&config_ptr, &data) == -1) {
				save_errno = errno;
				fprintf(stderr, "Failed to parse CSV_OUTPUT '%s'.\n", data);
				goto end;
			}
			if ((config->out_fp = fopen(data, "w")) == NULL) {
				save_errno = errno;
				fprintf(stderr, "Failed to open '%s'.", data);
				goto end;
			}
		}
		else if (COL_CMP("CSV_ERROR", config_ptr, col_len)) {
			config_ptr = line_sep + 1;
			if (token_string_get(&config_ptr, &data) == -1) {
				save_errno = errno;
				fprintf(stderr, "Failed to parse CSV_ERROR '%s'.\n", data);
				goto end;
			}
			if ((config->err_fp = fopen(data, "w")) == NULL) {
				save_errno = errno;
				fprintf(stderr, "Failed to open '%s'.\n", data);
				goto end;
			}
		}
		else if (COL_CMP("CSV_RESULT", config_ptr, col_len)) {
			config_ptr = line_sep + 1;
			if (token_string_get(&config_ptr, &data) == -1) {
				save_errno = errno;
				fprintf(stderr, "Failed to parse CSV_RESULT '%s'.\n", data);
				goto end;
			}
			if ((config->res_fp = fopen(data, "w")) == NULL) {
				save_errno = errno;
				fprintf(stderr, "Failed to open '%s'.", data);
				goto end;
			}
		}
		else if (COL_CMP("CSV_FORMULA", config_ptr, col_len)) {
			config_ptr = line_sep + 1;
			if (token_string_get(&config_ptr, &data) == -1) {
				save_errno = errno;
				fprintf(stderr, "Failed to parse CSV_FORMULA '%s'.\n", config_ptr);
				goto end;
			}

			if (array_add(&raw_formulas, &data) == -1) {
				save_errno = errno;
				goto end;
			}
			data = NULL;
		}
		else if (COL_CMP("HEADERS", config_ptr, col_len)) {
			config_ptr = line_sep + 1;
			if (token_string_get(&config_ptr, &data) == -1 ||
				 parse_header(config, data) == -1) {
				save_errno = errno;
				fprintf(stderr, "Failed to parse HEADERS '%s'.\n", data);
				goto end;
			}
		}
		else if (COL_CMP("SORT_HEADERS", config_ptr, col_len)) {
			config_ptr = line_sep + 1;
			if (token_string_get(&config_ptr, &data) == -1) {
				save_errno = errno;
				fprintf(stderr, "Failed to parse SORT_HEADERS '%s'.\n", data);
				goto end;
			}

			sort_header = data;
			data = NULL;
		}
		else if (COL_CMP("SORT_ORDER", config_ptr, col_len)) {
			config_ptr = line_sep + 1;
			if (token_string_get(&config_ptr, &data) == -1) {
				save_errno = errno;
				fprintf(stderr, "Failed to parse SORT_ORDER '%s'.\n", data);
				goto end;
			}

			if (strcmp(data, "ASC") == 0) {
				config->sort_order = ASC;
			}
			else if (strcmp(data, "DESC") == 0) {
				config->sort_order = DESC;
			}
			else {
				save_errno = EINVAL;
				fprintf(stderr, "Failed to parse SORT_ORDER '%s'.\n", data);
				goto end;
			}
		}
		else {
			config_ptr = *line_end == '\0' ? line_end : line_end + 1;
		}

		if (data != NULL) {
			free(data);
			data = NULL;
		}
	}

	if (config->header_num > 0) {
		const char *sh = sort_header == NULL ? "" : sort_header;

		if (parse_sort_headers(config, sh) == -1) {
			fprintf(stderr, "Failed to parse SORT_HEADERS '%s'.\n", sh);
			save_errno = errno;
			goto end;
		}

		if (config->sort_num > 0 && config->sort_order == NOSORT) {
			fprintf(stderr, "No SORT_HEADERS.\n");
			save_errno = EINVAL;
			goto end;
		}

		for (size_t i = 0; i < raw_formulas.len; ++i) {
			char **str_p = ARRAY_GET(&raw_formulas, char*, i);
			char *str = *str_p;
			if (parse_formula(config, str) == -1) {
				fprintf(stderr, "Failed to parse CSV_FORMULA '%s'.\n", str);
				save_errno = EINVAL;
				goto end;
			}
		}
	}
	else {
		fprintf(stderr, "No HEADERS\n");
		save_errno = EINVAL;
		goto end;
	}

	if (config->out_fp == NULL) {
		fprintf(stderr, "NO CSV_OUTPUT\n");
		save_errno = EINVAL;
		goto end;
	}
	if (config->err_fp == NULL) {
		fprintf(stderr, "NO CSV_ERROR\n");
		save_errno = EINVAL;
		goto end;
	}
	if (config->res_fp == NULL) {
		fprintf(stderr, "NO CSV_RESULT\n");
		save_errno = EINVAL;
		goto end;
	}

end:
	if (data != NULL) {
		free(data);
		data = NULL;
	}

	if (config_mem != NULL) {
		mmap_free(config_mem, config_size);
		config_mem = NULL;
	}

	if (sort_header != NULL) {
		free(sort_header);
		sort_header = NULL;
	}

	if (raw_formulas_init) {
		array_destroy(&raw_formulas);
		raw_formulas_init = true;
	}

	if (save_errno != 0) {
		if (config != NULL) {
			config_destroy(config);
		}
		errno = save_errno;
		return -1;
	}

	return 0;
}

void
config_destroy(config_t *config)
{
	if (config == NULL) {
		errno = EINVAL;
		return;
	}

	for (size_t i = 0; i < config->header_num; ++i) {
		if (config->header[i].name != NULL) {
			free(config->header[i].name);
			config->header[i].name = NULL;
		}
	}

	array_destroy(&config->formulas);

	if (config->out_fp != NULL) {
		fclose(config->out_fp);
		config->out_fp = NULL;
	}
	if (config->err_fp != NULL) {
		fclose(config->err_fp);
		config->err_fp = NULL;
	}

	if (config->res_fp != NULL) {
		fclose(config->res_fp);
		config->res_fp = NULL;
	}
}

static int
config_get_header_idx(config_t *config, const char *field, size_t *idx)
{
	if (config == NULL || field == NULL || idx == NULL) {
		errno = EINVAL;
		return -1;
	}

	for (size_t i = 0; i < config->header_num; ++i) {
		if (strcmp(config->header[i].name, field) == 0) {
			*idx = i;
			return 0;
		}
	}

	errno = ENOENT;
	return -1;
}

static int
parse_header(config_t *config, const char *data)
{
	char *ptr = NULL;
	char *sep = NULL;
	bool final = false;
	char *tmp_data = NULL;
	int save_errno = 0;

	if (config == NULL || data == NULL) {
		errno = EINVAL;
		return -1;
	}

	if ((tmp_data = strdup(data)) == NULL) {
		return -1;
	}

	ptr = tmp_data;
	while (!final) {
		char *save_ptr = NULL;
		char *name = NULL;
		char *type = NULL;

		if ((sep = strchr(ptr, ',')) != NULL) {
			*sep = '\0';
		}
		else {
			final = true;
		}

		if ((name = strtok_r(ptr, " ", &save_ptr)) == NULL) {
			save_errno = EINVAL;
			goto end;
		}

		if ((type = strtok_r(NULL, " ", &save_ptr)) == NULL) {
			save_errno = EINVAL;
			goto end;
		}

		if (strtok_r(NULL, " ", &save_ptr) != NULL) {
			save_errno = EINVAL;
			goto end;
		}

		if (!is_valid_header_name(config, name)) {
			save_errno = EINVAL;
			goto end;
		}

		if (type_parse(type, &config->header[config->header_num].type) == -1) {
			save_errno = errno;
			goto end;
		}

		if ((config->header[config->header_num].name = strdup(name)) == NULL) {
			save_errno = errno;
			goto end;
		}

		config->header_num++;

		ptr = sep + 1;
	}

end:
	if (tmp_data != NULL) {
		free(tmp_data);
		tmp_data = NULL;
	}

	if (save_errno != 0) {
		errno = save_errno;
		return -1;
	}
	return 0;
}

static int
parse_sort_headers(config_t *config, const char *field)
{
	const char *ptr = NULL;
	char *name = NULL;
	int save_errno = 0;
	bool has_next = false;

	bool is_set[CSV_HEADER_MAX] = {0};

	if (config == NULL || field == NULL) {
		errno = EINVAL;
		return -1;
	}

	ptr = field;

	while (*ptr != '\0') {
		size_t i = 0;
		char letter;

		if (name != NULL) {
			free(name);
			name = NULL;
		}

		if (token_column_string_get(&ptr, &name) == -1) {
			save_errno = errno;
			goto end;
		}

		for (i = 0; i < config->header_num; ++i) {
			if (strcmp(config->header[i].name, name) == 0) {
				if (is_set[i]) {
					save_errno = EINVAL;
					goto end;
				}
				is_set[i] = true;
				config->sort_header[config->sort_num] = i;
				config->sort_num++;
				break;
			}
		}
		if (i == config->header_num) {
			save_errno = EINVAL;
			goto end;
		}

		if (token_letter_get(&ptr, &letter) == -1) {
			has_next = false;
			break;
		}

		if (letter != ',') {
			save_errno = EINVAL;
			goto end;
		}
		has_next = true;
	}

	if (has_next) {
		save_errno = EINVAL;
		goto end;
	}

	for (size_t i = 0, j = config->sort_num; i < config->header_num; ++i) {
		if (!is_set[i]) {
			config->sort_header[j] = i;
			j++;
		}
	}

end:
	if (name != NULL) {
		free(name);
		name = NULL;
	}

	if (save_errno != 0) {
		errno = save_errno;
		return -1;
	}

	return 0;
}

static int
parse_formula(config_t *config, const char *src)
{
	const char *ptr = src;
	char *str = NULL;

	array_t formula;
	array_t formula_op;
	bool formula_valid = false;
	bool formula_op_valid = false;
	bool is_print_get = false;

	int save_errno = 0;

	if (config == NULL || src == NULL) {
		errno = EINVAL;
		return -1;
	}

	if (array_init(&formula, sizeof(formula_element_t), NULL) == -1) {
		save_errno = errno;
		goto end;
	}
	formula_valid = true;


	if (array_init(&formula_op, sizeof(formula_element_t), NULL) == -1) {
		save_errno = errno;
		goto end;
	}
	formula_op_valid = true;

	while (true) {
		long val = 0;
		char *end_ptr = NULL;
		formula_element_t formula_element;

		formula_element.is_op = false;
		if (token_formula_get(&ptr, &str) == -1) {
			save_errno = errno;
			goto end;
		}
		if (str == NULL) {
			save_errno = EINVAL;
			goto end;
		}

		if (!is_print_get) {
			if (strcmp(str, "Print") == 0) {
				is_print_get = true;
				free(str);
				str = NULL;
				continue;
			}
			else {
				save_errno = EINVAL;
				goto end;
			}
		}
		if (str[0] == '*' || str[0] == '+' || str[0] == '[' || str[0] == ']') {
			save_errno = EINVAL;
			goto end;
		}

		if (config_get_header_idx(config, str, &formula_element.val.operand.val.variable.col) == -1) {
			val = 0;
			errno = 0;
			val = strtol(str, &end_ptr, 10);
			if ((val == LONG_MIN || val == LONG_MAX) && errno == ERANGE) {
				save_errno = errno;
				goto end;
			}
			if (val < INT_MIN || INT_MAX < val) {
				save_errno = ERANGE;
				goto end;
			}
			formula_element.is_op = false;
			formula_element.val.operand.is_constant = true;
			formula_element.val.operand.val.constant.type.type = INTEGER;
			formula_element.val.operand.val.constant.val.int_v = val;

			free(str);
			str = NULL;
		}
		else {
			free(str);
			str = NULL;

			formula_element.val.operand.is_constant = false;
			if (token_formula_get(&ptr, &str) == -1) {
				save_errno = errno;
				goto end;
			}
			if (str == NULL || str[0] != '[') {
				save_errno = EINVAL;
				goto end;
			}
			free(str);
			str = NULL;

			if (token_formula_get(&ptr, &str) == -1) {
				save_errno = errno;
				goto end;
			}
			if (str == NULL) {
				save_errno = EINVAL;
				goto end;
			}
			errno = 0;
			val = strtol(str, &end_ptr, 10);
			if ((val == LONG_MIN || val == LONG_MAX) && errno == ERANGE) {
				save_errno = errno;
				goto end;
			}
			if (*end_ptr != '\0' || val < 0) {
				save_errno = EINVAL;
				goto end;
			}
			free(str);
			str = NULL;

			formula_element.val.operand.val.variable.row_num = val;

			if (token_formula_get(&ptr, &str) == -1) {
				save_errno = errno;
				goto end;
			}
			if (str == NULL || str[0] != ']') {
				save_errno = EINVAL;
				goto end;
			}
			free(str);
			str = NULL;
		}

		if (array_add(&formula, &formula_element) == -1) {
			save_errno = errno;
			goto end;
		}

		if (token_formula_get(&ptr, &str) == -1) {
			save_errno = errno;
			goto end;
		}

		if (str == NULL) {
			break;
		}
		if (str[0] == '+') {
			formula_element.is_op = true;
			formula_element.val.op = ADD;
		}
		else if (str[0] == '*') {
			formula_element.is_op = true;
			formula_element.val.op = MULTIPLE;
		}
		else {
			save_errno = EINVAL;
			goto end;
		}
		free(str);
		str = NULL;

		while (formula_op.len > 0) {
			formula_element_t *op = ARRAY_GET(&formula_op, formula_element_t, formula_op.len - 1);
			if (op->val.op == formula_element.val.op || (op->val.op == MULTIPLE && formula_element.val.op == ADD)) {
				if (array_add(&formula, op) == -1) {
					save_errno = errno;
					goto end;
				}

				if (array_del_last(&formula_op, false) == -1) {
					save_errno = errno;
					goto end;
				}
			}
			else {
				break;
			}
		}

		if (array_add(&formula_op, &formula_element) == -1) {
			save_errno = errno;
			goto end;
		}
	}

	while (formula_op.len > 0) {
		formula_element_t *op = ARRAY_GET(&formula_op, formula_element_t, formula_op.len - 1);
		if (array_add(&formula, op) == -1) {
			save_errno = errno;
			goto end;
		}
		if (array_del_last(&formula_op, false) == -1) {
			save_errno = errno;
			goto end;
		}
	}

	if (array_add(&config->formulas, &formula) == -1) {
		save_errno = errno;
		goto end;
	}
	formula_valid = false;

end:
	if (formula_valid) {
		array_destroy(&formula);
	}

	if (formula_op_valid) {
		array_destroy(&formula_op);
	}

	if (str != NULL) {
		free(str);
		str = NULL;
	}

	if (save_errno != 0) {
		errno = save_errno;
		return -1;
	}

	return 0;
}

static bool
is_valid_header_name(config_t *config, const char *header)
{
	const char *ptr = header;

	if (config == NULL || header == NULL) {
		return false;
	}

	if (*ptr == '\0') {
		return false;
	}

	if (isdigit(*ptr)) {
		return false;
	}

	while (*ptr != '\0') {
		switch(*ptr) {
			case '*':
			case '+':
			case '[':
			case ']':
			case '-':
				return false;
		}

		if (isspace(*ptr)) {
			return false;
		}

		ptr++;
	}

	for (size_t i = 0; i < config->header_num; ++i) {
		if (strcmp(config->header[i].name, header) == 0) {
			return false;
		}
	}

	return true;
}

static void
raw_formula_free(void *p)
{
	char **str = (char**)p;
	char *real_str = *str;

	if (real_str != NULL) {
		free(real_str);
	}
}
