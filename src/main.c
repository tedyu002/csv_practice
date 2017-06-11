#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <time.h>

#include "mmap.h"
#include "csv.h"

static int usage();
static int get_csv(config_t *config, val_t **csv, size_t *csv_num);
static int output_csv(config_t *config, val_t *csv, size_t csv_num);
static int output_formulas(config_t *config, val_t *csv, size_t csv_num);
static int output_val(FILE *fp, const val_t *val);

int
main(int argc, char *argv[])
{
	config_t config;
	bool config_init = false;

	val_t *csv = NULL;
	size_t csv_num = 0;
	int ex = 0;

	memset(&config, 0, sizeof(config));

	if (argc != 2) {
		usage(argv[0]);
	}

	if (config_parse(argv[1], &config) == -1) {
		fprintf(stderr, "config '%s' parse error\n", argv[1]);
		ex = 1;
		goto end;
	}
	config_init = true;

	if (get_csv(&config, &csv, &csv_num) == -1) {
		fprintf(stderr, "failed to parse csv\n");
		ex = 1;
		goto end;
	}

	if (csv_sort(&config, csv, csv_num) == -1) {
		fprintf(stderr, "Failed to sort csv.\n");
		ex = 1;
		goto end;
	}

	if (output_csv(&config, csv, csv_num) == -1) {
		fprintf(stderr, "Failed to output csv.\n");
	}

	if (output_formulas(&config, csv, csv_num) == -1) {
		fprintf(stderr, "Failed to output formulas.\n");
	}

end:
	if (csv != NULL) {
		csv_destroy(&config, csv, csv_num);
		free(csv);
		csv = NULL;
	}

	if (config_init) {
		config_destroy(&config);
		config_init = false;
	}

	return ex;
}

static
int usage(const char *prog)
{
	fprintf(stderr, "%s conf_file\n", prog);
	exit(1);
}

static int
get_csv(config_t *config, val_t **csv, size_t *csv_num)
{
	void *csv_input_void = NULL;
	size_t csv_input_size = 0;
	const char *csv_input = NULL;
	int save_errno = 0;

	if (mmap_alloc(config->input_file, &csv_input_void, &csv_input_size) == -1) {
		errno = save_errno;
		fprintf(stderr, "Failed to mmap csv file '%s'.\n", config->input_file);
		goto end;
	}

	csv_input = (char*)csv_input_void;

	if (csv_parse(config, &csv_input, csv, csv_num) == -1) {
		errno = save_errno;
		goto end;
	}

end:
	if (csv_input_void != NULL) {
		mmap_free(csv_input_void, csv_input_size);
		csv_input_void = NULL;
	}

	if (save_errno != 0) {
		errno = save_errno;
		return -1;
	}
	return 0;
}

static int
output_csv(config_t *config, val_t *csv, size_t csv_num)
{
	for (size_t i = 0; i < csv_num; i ++) {
		val_t *row = csv + i * config->header_num;
		bool is_first = true;

		for (size_t j = 0; j < config->header_num; ++j) {
			val_t *val = row + config->sort_header[j];

			if (!is_first) {
				fputc(',', config->out_fp);
			}
			output_val(config->out_fp, val);
			is_first = false;
		}

		fputc('\n', config->out_fp);
	}

	return 0;
}

static int
output_formulas(config_t *config, val_t *csv, size_t csv_num)
{
	for (size_t i = 0; i < config->formulas.len; ++i) {
		array_t *formula = ARRAY_GET(&config->formulas, array_t, i);
		val_t val;

		if (csv_cal(config, csv, csv_num, formula, &val) == -1) {
			continue;
		}

		output_val(config->res_fp, &val);
		fputc('\n', config->res_fp);

		val_destroy(&val);
	}

	return 0;
}

static int
output_val(FILE *fp, const val_t *val)
{
	const char *p = NULL;
	struct tm tm;

	switch (val->type.type) {
		case INTEGER:
			fprintf(fp, "%d", val->val.int_v);
			break;
		case BOOL:
			fprintf(fp, "%s", val->val.bool_v ? "TRUE" : "FALSE");
			break;
		case DOUBLE:
			fprintf(fp, "%.2f", val->val.double_v);
			break;
		case CHAR:
		case VARCHAR:
			p = val->val.str_v.str;

			fputc('"', fp);
			while (*p!= '\0') {
				if (*p == '"') {
					fputs("\"\"", fp);
				}
				else {
					fputc(*p, fp);
				}
				p++;
			}
			fputc('"', fp);
			break;
		case DATETIME:
			localtime_r(&val->val.datetime_v, &tm);
			fprintf(fp, "%d/%02d/%02d %02d:%02d:%02d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
							tm.tm_hour, tm.tm_min, tm.tm_sec);
			break;
	}

	return 0;
}
