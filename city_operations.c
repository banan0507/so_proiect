#include "city_operations.h"
#include "secondary_functions.h"
#include <sys/wait.h>

/*
 * add() - Adauga un raport nou in district
 *
 * Pasi:
 *   1. Creaza directorul si fisierele daca nu exista
 *   2. Creaza symlink-ul active_reports-<district>
 *   3. Calculeaza urmatorul ID
 *   4. Citeste datele raportului de la utilizator
 *   5. Scrie raportul in fisierul binar reports.dat
 *   6. Scrie in log (doar managerul)
 */
void add(const char *district, const char *role, const char *user) {

    /*
     * create_district() creaza folderul si cele 3 fisiere cu permisiunile corecte.
     * Daca exista deja, nu face nimic (errno == EEXIST e ignorat).
     * create_symlink() creaza active_reports-<district> -> <district>/reports.dat
     */
    create_district(district);
    create_symlink(district);

    /* construim calea catre reports.dat: ex "downtown/reports.dat" */
    char path[256];
    snprintf(path, sizeof(path), "%s/reports.dat", district);

    /*
     * O_RDWR = deschide pentru citire SI scriere (avem nevoie de ambele)
     * O_APPEND = orice write() se face la sfarsitul fisierului automat
     */
    int fd = open(path, O_RDWR | O_APPEND);
    if (fd == -1) { perror("open reports.dat"); return; }

    /*
     * Calculam cate rapoarte exista deja in fisier.
     * stat() umple structura st cu informatii despre fisier, inclusiv st_size.
     * Impartim dimensiunea fisierului la dimensiunea unui raport (sizeof(Report))
     * ca sa stim cate rapoarte sunt stocate.
     * ex: daca fisierul are 784 bytes si sizeof(Report) = 392 -> 2 rapoarte
     */
    struct stat st;
    stat(path, &st);
    int num_reports = st.st_size / sizeof(Report);

    /*
     * Initializam structura Report cu 0 pe toti bytes.
     * memset(&r, 0, sizeof(Report)) e important ca sa nu ramanem cu
     * date random in memorie in campurile necompletate.
     */
    Report r;
    memset(&r, 0, sizeof(Report));

    /* ID-ul urmator e numarul de rapoarte existente + 1 */
    r.id = num_reports + 1;

    /*
     * strncpy copiaza maxim sizeof(r.inspector)-1 caractere
     * ca sa nu depasim buffer-ul (buffer overflow)
     */
    strncpy(r.inspector, user, sizeof(r.inspector) - 1);

    /* time(NULL) returneaza timpul curent in secunde de la 1 Ian 1970 */
    r.timestamp = time(NULL);

    /* citim datele de la utilizator */
    printf("Latitude: ");
    scanf("%lf", &r.latitude);   // %lf citeste un double

    printf("Longitude: ");
    scanf("%lf", &r.longitude);

    printf("Category (road/lighting/flooding): ");
    scanf("%31s", r.category);   // %31s citeste maxim 31 caractere (+ '\0')

    printf("Severity (1=minor, 2=moderate, 3=critical): ");
    scanf("%d", &r.severity);

    printf("Description: ");
    /*
     * getchar() consuma '\n'-ul ramas in buffer dupa scanf("%d").
     * Fara el, fgets() ar citi imediat '\n' si ar returna un string gol.
     */
    getchar();
    fgets(r.description, sizeof(r.description), stdin);
    /* strcspn gaseste pozitia primului '\n' si il inlocuieste cu '\0' */
    r.description[strcspn(r.description, "\n")] = '\0';

    /*
     * write() scrie toata structura Report ca bytes raw in fisier.
     * &r = adresa structurii, sizeof(Report) = numarul de bytes de scris.
     * Asta e ce inseamna fisier binar - nu scrie text, scrie bytes direct.
     */
    write(fd, &r, sizeof(Report));
    close(fd);

    /* doar managerul scrie in log */
    if (!strcmp(role, "manager")) {
        write_log(district, role, user, "add_report");
    }

    printf("Report #%d added successfully to district '%s'\n", r.id, district);
}

/*
 * list() - Listeaza toate rapoartele dintr-un district
 *
 * Afiseaza mai intai informatii despre fisier (permisiuni, dimensiune, data),
 * dupa afiseaza fiecare raport din reports.dat.
 */
void list(const char *district, const char *role, const char *user) {
    char path[256];
    snprintf(path, sizeof(path), "%s/reports.dat", district);

    /*
     * stat() verifica daca fisierul exista si ii citeste metadatele.
     * Returneaza -1 daca fisierul nu exista (districtul nu a fost creat).
     */
    struct stat st;
    if (stat(path, &st) == -1) {
        printf("Error: district '%s' not found\n", district);
        return;
    }

    /*
     * mode_to_string() converteste st.st_mode (un numar) in format simbolic.
     * ex: 0664 -> "rw-rw-r--"
     * st.st_mtime = timpul ultimei modificari a fisierului
     * ctime() il converteste in format human-readable
     */
    char perm_str[10];
    mode_to_string(st.st_mode, perm_str);

    char *mod_time = ctime(&st.st_mtime);
    mod_time[strlen(mod_time)-1] = '\0';  // scoate \n de la final

    printf("=== File Info ===\n");
    printf("Permissions : %s\n", perm_str);
    printf("Size        : %ld bytes\n", st.st_size);
    printf("Last modified: %s\n", mod_time);
    printf("=================\n\n");

    /* verificam daca sunt rapoarte in fisier */
    int num_reports = st.st_size / sizeof(Report);
    if (num_reports == 0) {
        printf("No reports found in district '%s'\n", district);
        return;
    }

    /*
     * O_RDONLY = deschide fisierul doar pentru citire
     * read() citeste exact sizeof(Report) bytes la fiecare apel,
     * avansand automat pozitia in fisier.
     * Bucla while continua cat timp read() returneaza sizeof(Report) bytes
     * (la sfarsitul fisierului returneaza 0 si bucla se opreste).
     */
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

/*
 * view() - Afiseaza un raport specific dupa ID
 *
 * Parcurge fisierul raport cu raport pana gaseste ID-ul cautat.
 */
void view(const char *district, int report_id, const char *role, const char *user) {
    char path[256];
    snprintf(path, sizeof(path), "%s/reports.dat", district);

    struct stat st;
    if (stat(path, &st) == -1) {
        printf("Error: district '%s' not found\n", district);
        return;
    }

    int fd = open(path, O_RDONLY);
    if (fd == -1) { perror("open reports.dat"); return; }

    Report r;
    int found = 0;

    /*
     * Citim raport cu raport pana gasim ID-ul cautat.
     * Cand il gasim, setam found=1 si iesim din bucla cu break.
     */
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

/*
 * remove_report() - Sterge un raport din fisierul binar
 *
 * Doar managerul poate sterge rapoarte.
 *
 * Algoritmul:
 *   1. Gaseste pozitia raportului de sters (indice i)
 *   2. Muta toate rapoartele de dupa el cu o pozitie mai in fata
 *      (raportul i+1 merge la pozitia i, i+2 la i+1, etc.)
 *   3. Micsoreaza fisierul cu sizeof(Report) bytes folosind ftruncate()
 *
 * Vizualizare:
 *   Inainte: [R1][R2][R3][R4]   (vrem sa stergem R2, found=1)
 *   Pas 2:   [R1][R3][R4][R4]   (R3 a suprascris R2, R4 a suprascris R3)
 *   Pas 3:   [R1][R3][R4]       (ftruncate taie ultimul R4 duplicat)
 */
void remove_report(const char *district, int report_id, const char *role) {

    /* verificam rolul - doar managerul poate sterge */
    if (strcmp(role, "manager") != 0) {
        printf("Error: only manager can remove reports\n");
        return;
    }

    char path[256];
    snprintf(path, sizeof(path), "%s/reports.dat", district);

    struct stat st;
    if (stat(path, &st) == -1) {
        printf("Error: district '%s' not found\n", district);
        return;
    }

    int num_reports = st.st_size / sizeof(Report);

    /* O_RDWR = avem nevoie de citire SI scriere */
    int fd = open(path, O_RDWR);
    if (fd == -1) { perror("open reports.dat"); return; }

    /*
     * Cautam raportul cu ID-ul dat.
     * lseek(fd, i * sizeof(Report), SEEK_SET) muta "cursorul" in fisier
     * la pozitia raportului i (SEEK_SET = de la inceputul fisierului).
     * found retine indicele pozitiei raportului gasit.
     */
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

    /*
     * Mutam fiecare raport de dupa found cu o pozitie mai in fata.
     * Pentru fiecare raport la pozitia i:
     *   - lseek la pozitia i si citim raportul
     *   - lseek la pozitia i-1 si scriem raportul acolo
     */
    for (int i = found + 1; i < num_reports; i++) {
        lseek(fd, i * sizeof(Report), SEEK_SET);
        read(fd, &r, sizeof(Report));

        lseek(fd, (i - 1) * sizeof(Report), SEEK_SET);
        write(fd, &r, sizeof(Report));
    }

    /*
     * ftruncate() taie fisierul la dimensiunea specificata.
     * Dupa ce am mutat toate rapoartele, ultimul raport e duplicat.
     * ftruncate il elimina reducand dimensiunea cu sizeof(Report).
     */
    ftruncate(fd, (num_reports - 1) * sizeof(Report));

    close(fd);
    printf("Report #%d removed successfully from district '%s'\n", report_id, district);
}

/*
 * update_threshold() - Actualizeaza valoarea threshold in district.cfg
 *
 * Doar managerul poate actualiza threshold-ul.
 * Verifica ca permisiunile pe district.cfg sunt exact 640 inainte de scriere.
 * Daca cineva a modificat permisiunile, refuza operatia.
 */
void update_threshold(const char *district, int value, const char *role) {

    /* verificam rolul */
    if (strcmp(role, "manager") != 0) {
        printf("Error: only manager can update threshold\n");
        return;
    }

    char path[256];
    snprintf(path, sizeof(path), "%s/district.cfg", district);

    struct stat st;
    if (stat(path, &st) == -1) {
        printf("Error: district '%s' not found\n", district);
        return;
    }

    /*
     * st.st_mode & 0777 extrage doar bitii de permisiuni
     * (elimina bitii de tip fisier care sunt in st_mode).
     * Verificam ca sunt exact 0640 (rw-r-----).
     * Daca nu, cineva a modificat permisiunile si refuzam operatia.
     */
    mode_t perms = st.st_mode & 0777;
    if (perms != 0640) {
        printf("Error: district.cfg permissions have been changed (expected 640, got %o)\n", perms);
        return;
    }

    /*
     * O_WRONLY = doar scriere
     * O_TRUNC = sterge continutul vechi al fisierului la deschidere
     * Scriem noua valoare a threshold-ului.
     */
    int fd = open(path, O_WRONLY | O_TRUNC);
    if (fd == -1) { perror("open district.cfg"); return; }

    char buf[64];
    int len = snprintf(buf, sizeof(buf), "threshold=%d\n", value);
    write(fd, buf, len);
    close(fd);

    printf("Threshold updated to %d in district '%s'\n", value, district);
}

/*
 * filter() - Filtreaza rapoartele dupa una sau mai multe conditii
 *
 * Conditiile sunt de forma "field:operator:value"
 * ex: "severity:>=:2" sau "category:==:road"
 *
 * Toate conditiile trebuie satisfacute (AND implicit).
 * Foloseste parse_condition() si match_condition() din secondary_functions.c
 *
 * ATENTIE: in terminal, conditiile cu operatori <, >, >=, <=
 * trebuie puse in ghilimele: "severity:>=:2"
 */
void filter(const char *district, const char *role, const char *user,
            char **conditions, int num_conditions) {
    char path[256];
    snprintf(path, sizeof(path), "%s/reports.dat", district);

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

    /*
     * Citim fiecare raport din fisier.
     * Pentru fiecare raport, verificam TOATE conditiile (AND).
     * Daca orice conditie nu e satisfacuta, match devine 0 si
     * raportul nu e afisat.
     */
    while (read(fd, &r, sizeof(Report)) == sizeof(Report)) {
        int match = 1;

        for (int i = 0; i < num_conditions; i++) {
            char field[32], op[4], value[64];

            /*
             * parse_condition() imparte "severity:>=:2" in:
             *   field = "severity"
             *   op    = ">="
             *   value = "2"
             * Returneaza 0 daca formatul e invalid.
             */
            if (!parse_condition(conditions[i], field, op, value)) {
                printf("Error: invalid condition '%s'\n", conditions[i]);
                match = 0;
                break;
            }

            /*
             * match_condition() verifica daca raportul r satisface conditia.
             * Returneaza 1 daca da, 0 daca nu.
             * Daca nu satisface, setam match=0 si iesim din bucla conditiilor.
             */
            if (!match_condition(&r, field, op, value)) {
                match = 0;
                break;
            }
        }

        /* daca toate conditiile sunt satisfacute, afisam raportul */
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

void remove_district(const char *district, const char *role) {
    // 1. Doar managerul poate sterge districte
    if (strcmp(role, "manager") != 0) {
        printf("Error: only manager can remove districts\n");
        return;
    }

    // 2. Verifica ca districtul exista
    struct stat st;
    if (stat(district, &st) == -1) {
        printf("Error: district '%s' not found\n", district);
        return;
    }

    // 3. Creaza procesul copil
    pid_t pid = fork();

    if (pid == -1) {
        // fork() a esuat
        perror("fork");
        return;
    }

    if (pid == 0) {
        // ====== PROCESUL COPIL ======
        // execvp() inlocuieste procesul curent cu "rm -rf <district>"
        // argv pentru rm: { "rm", "-rf", "<district>", NULL }
        char *args[] = { "rm", "-rf", (char *)district, NULL };
        execvp("rm", args);

        // daca execvp() returneaza, inseamna ca a esuat
        perror("execvp");
        exit(1);
    }

    // ====== PROCESUL PARINTE ======
    // waitpid() asteapta sa termine copilul
    // al doilea argument e statusul de iesire al copilului
    int status;
    waitpid(pid, &status, 0);

    if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
        printf("District '%s' deleted successfully\n", district);
    } else {
        printf("Error: failed to delete district '%s'\n", district);
        return;
    }

    // 4. Sterge symlink-ul active_reports-<district>
    char link_name[256];
    snprintf(link_name, sizeof(link_name), "active_reports-%s", district);
    if (unlink(link_name) == -1) {
        printf("Warning: could not delete symlink '%s'\n", link_name);
    } else {
        printf("Symlink '%s' deleted successfully\n", link_name);
    }
}