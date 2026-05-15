#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

typedef struct {
    int id;
    char inspector[64];
    double latitude;
    double longitude;
    char category[32];
    int severity;
    long timestamp;
    char description[256];
} Report;

typedef struct {
    char inspector[64];
    int total_severity;
    int num_reports;
} InspectorScore;

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: ./scorer <district>\n");
        exit(1);
    }

    const char *district = argv[1];

    /* construim calea catre reports.dat */
    char path[256];
    snprintf(path, sizeof(path), "%s/reports.dat", district);

    /* verificam ca districtul exista */
    struct stat st;
    if (stat(path, &st) == -1) {
        fprintf(stderr, "Error: district '%s' not found\n", district);
        exit(1);
    }

    /* deschidem reports.dat */
    int fd = open(path, O_RDONLY);
    if (fd == -1) {
        perror("open reports.dat");
        exit(1);
    }

    /*
     * Citim toate rapoartele si calculam scorul pentru fiecare inspector.
     * Folosim un array de InspectorScore pentru a tine evidenta.
     */
    InspectorScore scores[256];
    int num_inspectors = 0;
    Report r;

    while (read(fd, &r, sizeof(Report)) == sizeof(Report)) {
        /* cautam inspectorul in array */
        int found = -1;
        for (int i = 0; i < num_inspectors; i++) {
            if (strcmp(scores[i].inspector, r.inspector) == 0) {
                found = i;
                break;
            }
        }

        if (found == -1) {
            /* inspector nou - il adaugam in array */
            strncpy(scores[num_inspectors].inspector, r.inspector, 63);
            scores[num_inspectors].total_severity = r.severity;
            scores[num_inspectors].num_reports = 1;
            num_inspectors++;
        } else {
            /* inspector existent - actualizam scorul */
            scores[found].total_severity += r.severity;
            scores[found].num_reports++;
        }
    }
    close(fd);

    /* afisam rezultatele - merg in pipe prin dup2 din city_hub */
    printf("District: %s\n", district);
    if (num_inspectors == 0) {
        printf("  No reports found\n");
    } else {
        for (int i = 0; i < num_inspectors; i++) {
            printf("  Inspector: %-20s Score: %d (reports: %d)\n",
                   scores[i].inspector,
                   scores[i].total_severity,
                   scores[i].num_reports);
        }
    }
    printf("---\n");  // marcaj de final pentru city_hub

    return 0;
}