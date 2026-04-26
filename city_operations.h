#ifndef CITY_OPERATIONS_H
#define CITY_OPERATIONS_H

#include <time.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

typedef struct {
    int id;
    char inspector[64];
    double latitude;
    double longitude;
    char category[32];
    int severity;
    time_t timestamp;
    char description[256];
} Report;

void add(const char *district, const char *role, const char *user);
void list(const char *district, const char *role, const char *user);
void view(const char *district, int report_id, const char *role, const char *user);
void remove_report(const char *district, int report_id, const char *role);
void update_threshold(const char *district, int value, const char *role);
void filter(const char *district, const char *role, const char *user, char **conditions, int num_conditions);

#endif