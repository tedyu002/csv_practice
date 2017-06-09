#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "token.h"

const char *data[] = {
	"  12345  ",
	"12345",
	"  12345   \n",
	"12345",
	"   \"12345 67890   \"",
	"12345 67890   ",
	"  \"line1\nline2\nline3\"  ",
	"line1line2line3",
	"  \"line1\nline2\nline3\"  \n",
	"line1line2line3",
	"  \"\"\"line1\"\"\n\"\"line2\"\"\n\"\"line3\"\"\"  \n",
	"\"line1\"\"line2\"\"line3\"",
};

int
main(int argc, char *argv[])
{
	size_t failed = 0;

	for (size_t i = 0; i < sizeof(data) / sizeof(*data); i+= 2) {
		char *res = NULL;
		const char *src = data[i];

		printf("----------------------------\n");
		printf("'%s': ", data[i]);
		if (token_column_string_get(&src, &res) == -1) {
			printf("Failed\n");
			failed++;
		}
		else if (strcmp(data[i + 1], res) != 0) {
			printf("Failed\n'%s'", res);
			free(res);
			failed++;
		}
		else {
			printf("PASSED\n");
		}
	}

	printf("\n\n\n\nFailed: %zu\n", failed);
	return 0;
}
