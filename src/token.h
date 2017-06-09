#ifndef __TZUTE_TOKEN_H
#define __TZUTE_TOKEN_H

#include <stdbool.h>
#include <time.h>

#include "type.h"

extern int token_string_get(const char **src, char **string);
extern int token_column_string_get(const char **src, char **string);
extern int token_letter_get(const char **src, char *letter);
extern int token_to_line_feed(const char **src);
extern int token_formula_get(const char **src, char **string);

#endif
