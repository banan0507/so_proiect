#ifndef SECONDARY_FUNCTIONS_H
#define SECONDARY_FUNCTIONS_H

#include "city_operations.h"
#include <time.h>
#include <stdlib.h>
#include <errno.h>

void create_district(const char *district);
void write_log(const char *district, const char *role, const char *user, const char *action);
void create_symlink(const char *district);
void mode_to_string(mode_t mode, char *str);
int parse_condition(const char *input, char *field, char *op, char *value);
int match_condition(Report *r, const char *field, const char *op, const char *value); 

#endif