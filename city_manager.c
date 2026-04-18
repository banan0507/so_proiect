#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <errno.h>

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

void create_district(const char *district) {
    // 1. Creaza directorul
    if (mkdir(district, 0750) == -1 && errno != EEXIST) {
        perror("mkdir");
        exit(1);
    }

    // 2. Creaza reports.dat
    char path[256];
    snprintf(path, sizeof(path), "%s/reports.dat", district);
    int fd = open(path, O_CREAT | O_WRONLY, 0664);
    if (fd == -1) { perror("open reports.dat"); exit(1); }
    close(fd);
    chmod(path, 0664);

    // 3. Creaza district.cfg
    snprintf(path, sizeof(path), "%s/district.cfg", district);
    fd = open(path, O_CREAT | O_WRONLY, 0640);
    if (fd == -1) { perror("open district.cfg"); exit(1); }
    // scriem threshold default = 1
    write(fd, "threshold=1\n", 12);
    close(fd);
    chmod(path, 0640);

    // 4. Creaza logged_district
    snprintf(path, sizeof(path), "%s/logged_district", district);
    fd = open(path, O_CREAT | O_WRONLY, 0644);
    if (fd == -1) { perror("open logged_district"); exit(1); }
    close(fd);
    chmod(path, 0644);
}

void write_log(const char *district, const char *role, const char *user, const char *action) {
    char path[256];
    snprintf(path, sizeof(path), "%s/logged_district", district);

    // verifica permisiuni - doar managerul poate scrie
    struct stat st;
    stat(path, &st);
    if (!strcmp(role, "inspector")) {
        printf("Error: inspector cannot write to log\n");
        return;
    }

    int fd = open(path, O_WRONLY | O_APPEND);
    if (fd == -1) { perror("open log"); return; }

    time_t now = time(NULL);
    char *timestr = ctime(&now);
    timestr[strlen(timestr)-1] = '\0'; // scoate \n din ctime

    char entry[512];
    int len = snprintf(entry, sizeof(entry), "[%s] role=%s user=%s action=%s\n",
                       timestr, role, user, action);
    write(fd, entry, len);
    close(fd);
}

void create_symlink(const char *district) {
    char link_name[256];
    char target[256];

    snprintf(link_name, sizeof(link_name), "active_reports-%s", district);
    snprintf(target, sizeof(target), "%s/reports.dat", district);

    // sterge symlink-ul vechi daca exista
    unlink(link_name);

    if (symlink(target, link_name) == -1) {
        perror("symlink");
    }
}

void add(const char *district, const char *role, const char *user) {
    // 1. Creaza directorul si fisierele
    create_district(district);
    create_symlink(district);

    // 2. Deschide reports.dat ca sa calculam urmatorul ID
    char path[256];
    snprintf(path, sizeof(path), "%s/reports.dat", district);

    int fd = open(path, O_RDWR | O_APPEND);
    if (fd == -1) { perror("open reports.dat"); return; }

    // calculeaza cate rapoarte exista deja
    struct stat st;
    stat(path, &st);
    int num_reports = st.st_size / sizeof(Report);

    // 3. Completeaza raportul
    Report r;
    memset(&r, 0, sizeof(Report)); // initializeaza cu 0

    r.id = num_reports + 1;
    strncpy(r.inspector, user, sizeof(r.inspector) - 1);
    r.timestamp = time(NULL);
    r.severity = 1; // default

    printf("Latitude: ");
    scanf("%lf", &r.latitude);
    printf("Longitude: ");
    scanf("%lf", &r.longitude);
    printf("Category (road/lighting/flooding): ");
    scanf("%31s", r.category);
    printf("Severity (1=minor, 2=moderate, 3=critical): ");
    scanf("%d", &r.severity);
    printf("Description: ");
    // citim tot restul liniei
    getchar(); // consuma \n ramas
    fgets(r.description, sizeof(r.description), stdin);
    r.description[strcspn(r.description, "\n")] = '\0'; // scoate \n

    // 4. Scrie raportul in fisier
    write(fd, &r, sizeof(Report));
    close(fd);

    // 5. Scrie in log (doar managerul)
    if (!strcmp(role, "manager")) {
        write_log(district, role, user, "add_report");
    }

    printf("Report #%d added successfully to district '%s'\n", r.id, district);
}

void mode_to_string(mode_t mode, char *str) {
    // owner
    str[0] = (mode & S_IRUSR) ? 'r' : '-';
    str[1] = (mode & S_IWUSR) ? 'w' : '-';
    str[2] = (mode & S_IXUSR) ? 'x' : '-';
    // group
    str[3] = (mode & S_IRGRP) ? 'r' : '-';
    str[4] = (mode & S_IWGRP) ? 'w' : '-';
    str[5] = (mode & S_IXGRP) ? 'x' : '-';
    // others
    str[6] = (mode & S_IROTH) ? 'r' : '-';
    str[7] = (mode & S_IWOTH) ? 'w' : '-';
    str[8] = (mode & S_IXOTH) ? 'x' : '-';
    str[9] = '\0';
}

void list(const char *district, const char *role, const char *user) {
    char path[256];
    snprintf(path, sizeof(path), "%s/reports.dat", district);

    // 1. Verifica daca fisierul exista
    struct stat st;
    if (stat(path, &st) == -1) {
        printf("Error: district '%s' not found\n", district);
        return;
    }

    // 2. Afiseaza info despre fisier
    char perm_str[10];
    mode_to_string(st.st_mode, perm_str);

    char *mod_time = ctime(&st.st_mtime);
    mod_time[strlen(mod_time)-1] = '\0';

    printf("=== File Info ===\n");
    printf("Permissions : %s\n", perm_str);
    printf("Size        : %ld bytes\n", st.st_size);
    printf("Last modified: %s\n", mod_time);
    printf("=================\n\n");

    // 3. Verifica daca sunt rapoarte
    int num_reports = st.st_size / sizeof(Report);
    if (num_reports == 0) {
        printf("No reports found in district '%s'\n", district);
        return;
    }

    // 4. Deschide fisierul si citeste rapoartele
    int fd = open(path, O_RDONLY);
    if (fd == -1) { perror("open reports.dat"); return; }

    printf("=== Reports in district '%s' ===\n", district);

    Report r;
    while (read(fd, &r, sizeof(Report)) == sizeof(Report)) {
        char *ts = ctime(&r.timestamp);
        ts[strlen(ts)-1] = '\0';

        printf("-----------------------------\n");
        printf("ID         : %d\n", r.id);
        printf("Inspector  : %s\n", r.inspector);
        printf("Category   : %s\n", r.category);
        printf("Severity   : %d\n", r.severity);
        printf("GPS        : (%.4f, %.4f)\n", r.latitude, r.longitude);
        printf("Timestamp  : %s\n", ts);
        printf("Description: %s\n", r.description);
    }
    printf("-----------------------------\n");
    printf("Total: %d report(s)\n", num_reports);

    close(fd);
}

void view(const char *district, int report_id, const char *role, const char *user) {
    char path[256];
    snprintf(path, sizeof(path), "%s/reports.dat", district);

    // verifica daca fisierul exista
    struct stat st;
    if (stat(path, &st) == -1) {
        printf("Error: district '%s' not found\n", district);
        return;
    }

    int fd = open(path, O_RDONLY);
    if (fd == -1) { perror("open reports.dat"); return; }

    Report r;
    int found = 0;

    while (read(fd, &r, sizeof(Report)) == sizeof(Report)) {
        if (r.id == report_id) {
            found = 1;
            char *ts = ctime(&r.timestamp);
            ts[strlen(ts)-1] = '\0';

            printf("=== Report #%d ===\n", r.id);
            printf("Inspector  : %s\n", r.inspector);
            printf("Category   : %s\n", r.category);
            printf("Severity   : %d\n", r.severity);
            printf("GPS        : (%.4f, %.4f)\n", r.latitude, r.longitude);
            printf("Timestamp  : %s\n", ts);
            printf("Description: %s\n", r.description);
            break;
        }
    }

    if (!found) {
        printf("Error: report #%d not found in district '%s'\n", report_id, district);
    }

    close(fd);
}

void remove_report(const char *district, int report_id, const char *role) {
    // 1. Doar managerul poate sterge
    if (strcmp(role, "manager") != 0) {
        printf("Error: only manager can remove reports\n");
        return;
    }

    char path[256];
    snprintf(path, sizeof(path), "%s/reports.dat", district);

    // 2. Verifica permisiunile
    struct stat st;
    if (stat(path, &st) == -1) {
        printf("Error: district '%s' not found\n", district);
        return;
    }

    int num_reports = st.st_size / sizeof(Report);

    // 3. Deschide fisierul
    int fd = open(path, O_RDWR);
    if (fd == -1) { perror("open reports.dat"); return; }

    // 4. Gaseste pozitia raportului de sters
    int found = -1;
    Report r;

    for (int i = 0; i < num_reports; i++) {
        lseek(fd, i * sizeof(Report), SEEK_SET);
        read(fd, &r, sizeof(Report));
        if (r.id == report_id) {
            found = i;
            break;
        }
    }

    if (found == -1) {
        printf("Error: report #%d not found\n", report_id);
        close(fd);
        return;
    }

    // 5. Muta toate rapoartele de dupa found cu o pozitie mai in fata
    for (int i = found + 1; i < num_reports; i++) {
        // citeste raportul de la pozitia i
        lseek(fd, i * sizeof(Report), SEEK_SET);
        read(fd, &r, sizeof(Report));

        // scrie-l la pozitia i-1
        lseek(fd, (i - 1) * sizeof(Report), SEEK_SET);
        write(fd, &r, sizeof(Report));
    }

    // 6. Micsoreaza fisierul cu un raport
    ftruncate(fd, (num_reports - 1) * sizeof(Report));

    close(fd);
    printf("Report #%d removed successfully from district '%s'\n", report_id, district);
}

void update_threshold(const char *district, int value, const char *role) {
    // 1. Doar managerul poate actualiza
    if (strcmp(role, "manager") != 0) {
        printf("Error: only manager can update threshold\n");
        return;
    }

    char path[256];
    snprintf(path, sizeof(path), "%s/district.cfg", district);

    // 2. Verifica permisiunile - trebuie sa fie exact 640
    struct stat st;
    if (stat(path, &st) == -1) {
        printf("Error: district '%s' not found\n", district);
        return;
    }

    // extrage doar bitii de permisiuni
    mode_t perms = st.st_mode & 0777;
    if (perms != 0640) {
        printf("Error: district.cfg permissions have been changed (expected 640, got %o)\n", perms);
        return;
    }

    // 3. Deschide fisierul si scrie noua valoare
    int fd = open(path, O_WRONLY | O_TRUNC);
    if (fd == -1) { perror("open district.cfg"); return; }

    char buf[64];
    int len = snprintf(buf, sizeof(buf), "threshold=%d\n", value);
    write(fd, buf, len);
    close(fd);

    printf("Threshold updated to %d in district '%s'\n", value, district);
}

int parse_condition(const char *input, char *field, char *op, char *value) {
    // facem o copie ca sa nu modificam input-ul original
    char copy[256];
    strncpy(copy, input, sizeof(copy) - 1);
    copy[255] = '\0';

    // cautam primul ':'
    char *first = strchr(copy, ':');
    if (first == NULL) return 0;
    *first = '\0'; // taiem string-ul aici

    // cautam al doilea ':'
    char *second = strchr(first + 1, ':');
    if (second == NULL) return 0;
    *second = '\0'; // taiem din nou

    // copiem cele 3 parti
    strncpy(field, copy, 31);
    strncpy(op, first + 1, 3);
    strncpy(value, second + 1, 63);

    field[31] = '\0';
    op[3] = '\0';
    value[63] = '\0';

    return 1;
}

int match_condition(Report *r, const char *field, const char *op, const char *value) {
    // helper macro pentru comparatii numerice
    #define COMPARE(a, b) ( \
        strcmp(op, "==") == 0 ? (a) == (b) : \
        strcmp(op, "!=") == 0 ? (a) != (b) : \
        strcmp(op, "<")  == 0 ? (a) <  (b) : \
        strcmp(op, "<=") == 0 ? (a) <= (b) : \
        strcmp(op, ">")  == 0 ? (a) >  (b) : \
        strcmp(op, ">=") == 0 ? (a) >= (b) : 0)

    if (strcmp(field, "severity") == 0) {
        int val = atoi(value);
        return COMPARE(r->severity, val);
    }

    if (strcmp(field, "timestamp") == 0) {
        time_t val = (time_t)atol(value);
        return COMPARE(r->timestamp, val);
    }

    if (strcmp(field, "category") == 0) {
        int cmp = strcmp(r->category, value);
        return COMPARE(cmp, 0);
    }

    if (strcmp(field, "inspector") == 0) {
        int cmp = strcmp(r->inspector, value);
        return COMPARE(cmp, 0);
    }

    printf("Error: unknown field '%s'\n", field);
    return 0;

    #undef COMPARE
}

void filter(const char *district, const char *role, const char *user, 
            char **conditions, int num_conditions) {
    char path[256];
    snprintf(path, sizeof(path), "%s/reports.dat", district);

    // verifica daca fisierul exista
    struct stat st;
    if (stat(path, &st) == -1) {
        printf("Error: district '%s' not found\n", district);
        return;
    }

    int fd = open(path, O_RDONLY);
    if (fd == -1) { perror("open reports.dat"); return; }

    int num_reports = st.st_size / sizeof(Report);
    if (num_reports == 0) {
        printf("No reports found in district '%s'\n", district);
        close(fd);
        return;
    }

    printf("=== Filtered Reports in district '%s' ===\n", district);
    int found = 0;
    Report r;

    while (read(fd, &r, sizeof(Report)) == sizeof(Report)) {
        int match = 1;

        // verifica toate conditiile
        for (int i = 0; i < num_conditions; i++) {
            char field[32], op[4], value[64];

            if (!parse_condition(conditions[i], field, op, value)) {
                printf("Error: invalid condition '%s'\n", conditions[i]);
                match = 0;
                break;
            }

            if (!match_condition(&r, field, op, value)) {
                match = 0;
                break;
            }
        }

        if (match) {
            found++;
            char *ts = ctime(&r.timestamp);
            ts[strlen(ts)-1] = '\0';

            printf("-----------------------------\n");
            printf("ID         : %d\n", r.id);
            printf("Inspector  : %s\n", r.inspector);
            printf("Category   : %s\n", r.category);
            printf("Severity   : %d\n", r.severity);
            printf("GPS        : (%.4f, %.4f)\n", r.latitude, r.longitude);
            printf("Timestamp  : %s\n", ts);
            printf("Description: %s\n", r.description);
        }
    }

    if (found == 0) {
        printf("No reports match the given conditions\n");
    } else {
        printf("-----------------------------\n");
        printf("Total matching: %d report(s)\n", found);
    }

    close(fd);
}

int main(int argc, char** argv) {
    char *role = NULL;
    char *user = NULL;

    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--role")) {
            role = argv[++i];
        } else if (!strcmp(argv[i], "--user")) {
            user = argv[++i];
        } else if (!strcmp(argv[i], "--add")) {
            char *district = argv[++i];
            add(district, role, user);
        } else if (!strcmp(argv[i], "--list")) {
            char *district = argv[++i];
            list(district, role, user);
        } else if (!strcmp(argv[i], "--view")) {
            char *district = argv[++i];
            int id = atoi(argv[++i]);
            view(district, id, role, user);
        } else if (!strcmp(argv[i], "--remove_report")) {
            char *district = argv[++i];
            int id = atoi(argv[++i]);
            remove_report(district, id, role);
        } else if (!strcmp(argv[i], "--update_threshold")) {
            char *district = argv[++i];
            int val = atoi(argv[++i]);
            update_threshold(district, val, role);
        } else if (!strcmp(argv[i], "--filter")) {
            char *district = argv[++i];
            // toate argumentele ramase sunt conditii
            char **conditions = &argv[i+1];
            int num_conditions = argc - i - 1;
            i = argc; // am consumat toate argumentele
            filter(district, role, user, conditions, num_conditions);
        }
    }

    return 0;
}