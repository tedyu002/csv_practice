#include <errno.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdio.h>

#include "limits.h"
#include "type.h"

int
type_parse(const char *src, type_t *type)
{
	if (src == NULL || type == NULL) {
		errno = EINVAL;
		return -1;
	}

	type->len = 0;
	if (strcmp("INTEGER", src) == 0) {
		type->type = INTEGER;
	}
	else if (strcmp("BOOL", src) == 0) {
		type->type = BOOL;
	}
	else if (strcmp("DOUBLE", src) == 0) {
		type->type = DOUBLE;
	}
	else if (strcmp("DATETIME", src) == 0) {
		type->type = DATETIME;
	}
	else {
		const char *end = NULL;
		char *to_l_end_ptr = NULL;
		size_t len_limit = 0;

		if (strncmp("VARCHAR", src, strlen("VARCHAR")) == 0) {
			type->type = VARCHAR;
			src += strlen("VARCHAR");
			len_limit = VARCHAR_MAX_LEN;
		}
		else if (strncmp("CHAR", src, strlen("CHAR")) == 0) {
			type->type = CHAR;
			src += strlen("CHAR");
			len_limit = CHAR_MAX_LEN;
		}
		else {
			errno = EINVAL;
			return -1;
		}

		if (*src != '(') {
			errno = EINVAL;
			return -1;
		}

		src++;
		end = src;

		while (true) {
			if (*end == '\0') {
				errno = EINVAL;
				return -1;
			}
			else if (!isdigit(*end)) {
				long val = 0;

				if (*end != ')') {
					errno = EINVAL;
					return -1;
				}

				val = strtol(src, &to_l_end_ptr, 10);
				if (val < 0 || val > len_limit) {
					errno = ERANGE;
					return -1;
				}

				if (end != to_l_end_ptr) {
					errno = EINVAL;
					return -1;
				}

				type->len = val;

				break;
			}
			end++;
		}
	}

	return 0;
}
