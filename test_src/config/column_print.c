#include <stdio.h>

#include "config.h"

int
main(int argc, char *argv[])
{
	config_t config;

	if (config_parse(argv[1], &config) == -1) {
		printf("Failed to parse csv file\n");
		return 1;
	}

	printf("%s=%s\n", "CSV_INPUT", config.input_file);
	printf("%s=%s\n", "CSV_OUTPUT", config.output_file);
	printf("%s=%s\n", "CSV_ERROR", config.err_file);
	printf("%s=%s\n", "CSV_RESULT", config.res_file);
	printf("%s=\n", "HEADERS");
	for (size_t i = 0; i < config.header_num; ++i) {
		printf("\t%s %d:%zu\n", config.header[i].name, (int)config.header[i].type.type, config.header[i].type.len);
	}

	printf("%s=", "SORT_HEADERS");
	for (size_t i = 0; i < config.sort_num; ++i) {
		printf(" %s", config.header[config.sort_header[i]].name);
	}
	printf("\n");

	printf("%s=%s\n", "SORT_ORDER", config.sort_order == ASC ? "ASC" : config.sort_order == DESC ? "DESC" : "NOSORT");

	for (size_t i = 0; i < config.formulas.len; ++i) {
		array_t *formula = ARRAY_GET(&config.formulas, array_t, i);

		printf("%s=%s\n", "CSV_FORMULA", "");
		for (size_t j = 0; j < formula->len; j++) {
			formula_element_t *formula_element = ARRAY_GET(formula, formula_element_t, j);

			if (formula_element->is_op) {
				printf("(op:%d)", formula_element->val.op);
			}
			else {
				if (!formula_element->val.operand.is_constant) {
					printf("(%zu:%zu)", formula_element->val.operand.val.variable.col,
											  formula_element->val.operand.val.variable.row_num);
				}
				else {
					printf("(%d)", formula_element->val.operand.val.constant.val.int_v);
				}
			}
		}
		printf("\n------------------------------------------\n");
	}

	config_destroy(&config);

	return 0;
}
