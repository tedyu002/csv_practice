#ifndef __TZUTE_OP_H
#define __TZUTE_OP_H

#include "val.h"

typedef enum {
	ADD,
	MULTIPLE,
	OPERATOR_NUM,
} operator_t;

typedef int (*op_two_t)(const val_t *a, const val_t *b, val_t *t);

extern op_two_t type_operators[TYPE_NUM][OPERATOR_NUM];

#endif
