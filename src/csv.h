#ifndef __TZUTE_CSV_H
#define __TZUTE_CSV_H

#include "config.h"
#include "val.h"

extern int csv_parse(config_t *config, const char **src, val_t **csv, size_t *csv_num);
extern int csv_cal(config_t *config, val_t *csv, size_t csv_num, array_t *formula, val_t *res);
extern int csv_sort(config_t *config, val_t *csv, size_t csv_num);
extern int csv_row_parse(config_t *config, const char **src, const char **err, val_t **row);

#endif
