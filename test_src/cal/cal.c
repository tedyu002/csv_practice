#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "mmap.h"
#include "csv.h"

int
main(int argc, char *argv[])
{
	config_t config;
	void *ptr = NULL;
	const char *ptr_char = NULL;
	size_t size = 0;
	val_t *csv = NULL;
	size_t csv_num = 0;
	val_t val;

	memset(&config, 0, sizeof(config));

	if (config_parse(argv[1], &config) == -1) {
		printf("config '%s' parse error\n", argv[1]);
		return 1;
	}

	if (mmap_alloc(argv[2], &ptr, &size) == -1) {
		printf("Failed to map row '%s'.\n", argv[2]);
		return 1;
	}

	ptr_char = (char*)ptr;


	if (csv_parse(&config, &ptr_char, &csv, &csv_num) == -1) {
		printf("Failed to parse row '%s'.\n", (char*)ptr);
		return 1;
	}

	for (size_t i = 0; i < config.formulas.len; ++i) {
		array_t *formula = ARRAY_GET(&config.formulas, array_t, i);

		if (csv_cal(&config, csv, csv_num, formula, &val) == -1) {
			printf("Failed to cal result.\n");
		}

		switch (val.type.type) {
			case INTEGER:
				printf("%d", val.val.int_v);
				break;
			case BOOL:
				printf("%d", val.val.bool_v);
				break;
			case DOUBLE:
					printf("%f", val.val.double_v);
				break;
			case CHAR:
				printf("%s", val.val.str_v.str);
				break;
			case VARCHAR:
				printf("%s", val.val.str_v.str);
				break;
			case DATETIME:
				printf("%ld", val.val.datetime_v);
				break;
		}
		putchar('\n');

		val_destroy(&val);
	}

	return 0;
}
