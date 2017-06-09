#ifndef __TZUTE_TYPE_H
#define __TZUTE_TYPE_H

#include <stdbool.h>
#include <time.h>
#include <stdlib.h>

enum type {
	INTEGER,
	BOOL,
	DOUBLE,
	CHAR,
	VARCHAR,
	DATETIME,
	TYPE_NUM,
};

typedef struct {
	char *str;
	size_t len;
} string_t;

typedef struct {
	int type;
	size_t len;
} type_t;

extern int type_parse(const char *src, type_t *type);

#endif
