#ifndef __TZUTE_CONFIG_H
#define __TZUTE_CONFIG_H

#include <linux/limits.h>
#include <stdio.h>

#include "limits.h"
#include "type.h"
#include "container.h"
#include "formula.h"

enum SORT_ORDER {
	NOSORT,
	ASC,
	DESC,
};

typedef struct header {
	char *name;
	type_t type;
} header_t;

typedef struct config {
	char input_file[PATH_MAX];
	FILE *out_fp;
	FILE *err_fp;
	FILE *res_fp;
	header_t header[CSV_HEADER_MAX];
	size_t header_num;
	int sort_header[CSV_HEADER_MAX];
	size_t sort_num;
	int sort_order;
	array_t formulas;
} config_t;

extern int config_parse(const char *path, config_t *config);
extern void config_destroy(config_t *config);

#endif
