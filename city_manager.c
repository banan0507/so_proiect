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
            printf("REMOVE: district=%s id=%d role=%s user=%s\n", district, id, role, user);
        } else if (!strcmp(argv[i], "--update_threshold")) {
            char *district = argv[++i];
            int val = atoi(argv[++i]);
            printf("UPDATE_THRESHOLD: district=%s val=%d role=%s user=%s\n", district, val, role, user);
        } else if (!strcmp(argv[i], "--filter")) {
            char *district = argv[++i];
            printf("FILTER: district=%s role=%s user=%s\n", district, role, user);
        }
    }

    return 0;
}