#include <errno.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "token.h"


int
token_string_get(const char **src, char **string)
{
	const char *beg = NULL;
	const char *line_end = NULL;
	const char *str_end = NULL;
	char *tmp_str = NULL;
	size_t tmp_str_len = 0;

	if (src == NULL || string == NULL) {
		errno = EINVAL;
		return -1;
	}

	beg = *src;
	while (*beg != '\0' && *beg != '\n' && *beg == ' ') {
		beg++;
	}

	line_end = beg;
	while (*line_end != '\0' && *line_end != '\n') {
		line_end++;
	}

	str_end = line_end;
	while (beg < str_end && isspace(*(str_end - 1))) {
		str_end--;
	}

	tmp_str_len = str_end - beg;

	if ((tmp_str = malloc((tmp_str_len + 1) * sizeof(char))) == NULL) {
		return -1;
	}

	strncpy(tmp_str, beg, tmp_str_len);
	tmp_str[tmp_str_len] = '\0';

	*string = tmp_str;
	*src = line_end + ((*line_end == '\0') ? 0 : 1);

	return 0;
}

int
token_column_string_get(const char **src, char **string)
{
	const char *beg = NULL;
	const char *col_end = NULL;
	const char *str_end = NULL;
	bool quoted = false;

	if (src == NULL || string == NULL) {
		errno = EINVAL;
		return -1;
	}

	beg = *src;
	while (*beg != '\0' && *beg != '\n' && *beg != ',' && *beg == ' ') {
		beg++;
	}

	if (*beg == '\0' || *beg == '\n' || *beg == ',') {
		char *tmp_str = NULL;
		if ((tmp_str = strdup("")) == NULL) {
			return -1;
		}
		*string = tmp_str;
		*src = beg;
		return 0;
	}

	if (*beg == '"') {
		quoted = true;
		beg++;
	}

	if (quoted) {
		str_end = beg;
		FILE *target_fp = NULL;
		size_t target_size = 0;
		char *target_buf = NULL;

		target_fp = open_memstream(&target_buf, &target_size);
		if (target_fp == NULL) {
			return -1;
		}

		while (*str_end != '\0') {
			if (*str_end == '"') {
				if (*(str_end + 1) == '"') {
					fputc('"', target_fp);
					str_end += 2;
				}
				else {
					quoted = false;
					break;
				}
			}
			else if (*str_end == '\n') {
				str_end++;
			}
			else {
				fputc(*str_end, target_fp);
				str_end++;
			}
		}

		fclose(target_fp);

		if (quoted) {
			*src = str_end;
			errno = EINVAL;
			free(target_buf);
			return -1;
		}

		*string = target_buf;
		*src = str_end + 1;

		return 0;
	}
	else {
		char *tmp_str = NULL;
		size_t tmp_str_len = 0;

		col_end = beg;
		while (*col_end != '\0') {
			if (*col_end == ',' || *col_end == '\n') {
				break;
			}
			col_end++;
		}

		str_end = col_end;

		while (beg < str_end && *(str_end - 1) == ' ') {
			str_end--;
		}

		tmp_str_len = str_end - beg;

		if ((tmp_str = malloc((tmp_str_len + 1) * sizeof(char))) == NULL) {
			return -1;
		}

		strncpy(tmp_str, beg, tmp_str_len);
		tmp_str[tmp_str_len] = '\0';

		*string = tmp_str;
		*src = col_end;
	}

	return 0;
}

int
token_formula_get(const char **src, char **string)
{
	const char *beg = NULL;
	const char *str_end = NULL;
	char *tmp_str = NULL;
	size_t tmp_str_len = 0;

	if (src == NULL || string == NULL) {
		errno = EINVAL;
		return -1;
	}

	beg = *src;
	while (*beg != '\0' && *beg == ' ') {
		beg++;
	}

	if (*beg == '\0') {
		*string = NULL;
		return 0;
	}

	str_end = beg;
	while (!(*str_end == '*' || *str_end == '+' || *str_end == '-' ||
				*str_end == '[' || *str_end == ']' ||
				*str_end == ' ' || *str_end == '\0')) {
		str_end++;
	}

	if (str_end == beg && *beg != '\0') {
		str_end++;
	}

	tmp_str_len = str_end - beg;

	if ((tmp_str = malloc((tmp_str_len + 1) * sizeof(char))) == NULL) {
		return -1;
	}

	strncpy(tmp_str, beg, tmp_str_len);
	tmp_str[tmp_str_len] = '\0';

	*string = tmp_str;
	*src = str_end;

	return 0;
}

int
token_letter_get(const char **src, char *letter)
{
	const char *tmp_src = NULL;

	if (src == NULL || letter == NULL) {
		errno = EINVAL;
		return -1;
	}

	tmp_src = *src;

	while (*tmp_src != '\0') {
		if (*tmp_src == '\n') {
			errno = EINVAL;
			return -1;
		}
		else if (isspace(*tmp_src)) {
			tmp_src++;
		}
		else {
			*letter = *tmp_src;
			*src = tmp_src + 1;
			return 0;
		}
	}

	errno = EINVAL;
	return -1;
}

int
token_to_line_feed(const char **src)
{
	const char *tmp_src = NULL;

	if (src == NULL) {
		errno = EINVAL;
		return -1;
	}

	tmp_src = *src;

	while (!(*tmp_src == '\0' || *tmp_src == '\n')) {
		tmp_src++;
	}
	if (*tmp_src == '\n') {
		tmp_src++;
	}

	*src = tmp_src;
	return 0;
}
