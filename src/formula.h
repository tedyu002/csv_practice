#ifndef __FORMULA_H
#define __FORMULA_H

#include <stddef.h>
#include <stdbool.h>

#include "type.h"
#include "val.h"
#include "op.h"

typedef struct {
	bool is_constant;
	union{
		struct {
			size_t col;
			size_t row_num;
		} variable;
		val_t constant;
	} val;
} operand_t;

typedef struct formula_element {
	bool is_op;
	union {
		operand_t operand;
		operator_t op;
	} val;
} formula_element_t;

#endif
