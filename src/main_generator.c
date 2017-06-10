#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <time.h>

#include "mmap.h"
#include "csv.h"

static int usage();
static int gen_csv(config_t *config, val_t **csv, size_t csv_num);
static int output_csv(config_t *config, val_t *csv, size_t csv_num);
static int output_val(FILE *fp, const val_t *val);

int
main(int argc, char *argv[])
{
	config_t config;
	bool config_init = false;

	val_t *csv = NULL;
	int ex = 0;
	size_t csv_num = 0;

	memset(&config, 0, sizeof(config));

	if (argc != 3) {
		usage(argv[0]);
	}

	csv_num = atoi(argv[2]);

	if (config_parse(argv[1], &config) == -1) {
		printf("config '%s' parse error\n", argv[1]);
		ex = 1;
		goto end;
	}
	config_init = true;

	if (gen_csv(&config, &csv, csv_num) == -1) {
		printf("Failed to gen csv\n");
		ex = 1;
		goto end;
	}

	if (output_csv(&config, csv, csv_num) == -1) {
		printf("Failed to output csv.\n");
	}

end:
	if (csv != NULL) {
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
	fprintf(stderr, "%s conf_file row_num\n", prog);
	exit(1);
}

static int
gen_csv(config_t *config, val_t **csv, size_t row_num)
{
	srand(time(NULL));

	char printable_str[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz\"!@#$%^&*()_+=-,./\\";

	*csv = malloc(sizeof(val_t) * config->header_num * row_num);
	if (csv == NULL) {
		return -1;
	}

	for (size_t i = 0; i < row_num; ++i) {
		for (size_t j = 0; j < config->header_num; ++j) {
			val_t *val = *csv + i * config->header_num + j;
			val->type = config->header[j].type;

			switch(config->header[j].type.type) {
				case INTEGER:
					val->val.int_v = rand() * (rand() % 2 == 0 ? 1 : -1);
					break;
				case BOOL:
					val->val.bool_v = rand() % 2 == 0 ? true : false;
					break;
				case DOUBLE:
					val->val.double_v = rand() * (rand() / 100.0) * (rand() % 2 == 0 ? 1 : -1);
					break;
				case CHAR:
				case VARCHAR:
					val->val.str_v.len = rand() % val->type.len;
					val->val.str_v.str = malloc(sizeof(char) * (val->type.len + 1));
					if (val->val.str_v.str == NULL) {
						return -1;
					}
					for (size_t k = 0; k < val->type.len; ++k) {
						val->val.str_v.str[k] = printable_str[rand() % sizeof(printable_str)];
					}
					val->val.str_v.str[val->val.str_v.len] = '\0';
					break;
				case DATETIME:
					val->val.datetime_v = rand();
					break;
			}
		}
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
