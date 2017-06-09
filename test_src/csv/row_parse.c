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
	val_t *row = NULL;

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

	if (csv_row_parse(&config, &ptr_char, &row) == -1) {
		printf("Failed to parse row '%s'.\n", (char*)ptr);
		return 1;
	}

	for (size_t i = 0; i < config.header_num; ++i) {
		printf("%s: ", config.header[i].name);

		switch (config.header[i].type.type) {
			case INTEGER:
				printf("%d", row[i].val.int_v);
				break;
			case BOOL:
				printf("%d", row[i].val.bool_v);
				break;
			case DOUBLE:
				printf("%f", row[i].val.double_v);
				break;
			case CHAR:
				printf("%s", row[i].val.str_v.str);
				break;
			case VARCHAR:
				printf("%s", row[i].val.str_v.str);
				break;
			case DATETIME:
				printf("%ld", row[i].val.datetime_v);
				break;
		}
		putchar('\n');
	}

	return 0;
}
