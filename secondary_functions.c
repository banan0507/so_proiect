#include "secondary_functions.h"

/*
 * create_district() - Creaza structura de fisiere pentru un district nou
 * 
 * Creeaza:
 *   - directorul districtului (permisiuni 750)
 *   - reports.dat             (permisiuni 664) - fisierul binar cu rapoarte
 *   - district.cfg            (permisiuni 640) - fisierul de configurare
 *   - logged_district         (permisiuni 644) - jurnalul de operatii
 */
void create_district(const char *district) {

    /* 
     * mkdir() creaza directorul cu permisiunile 0750:
     *   7 = rwx pentru owner (manager)
     *   5 = r-x pentru group (inspector - poate citi si intra, dar nu scrie)
     *   0 = --- pentru others (nimeni altcineva)
     * 
     * Daca directorul exista deja (EEXIST), nu e o eroare - continuam normal.
     * Orice alta eroare e fatala si oprim programul cu exit(1).
     */
    if (mkdir(district, 0750) == -1 && errno != EEXIST) {
        perror("mkdir");
        exit(1);
    }

    char path[256];

    /* 
     * snprintf construieste calea completa: ex "downtown/reports.dat"
     * sizeof(path) limiteaza lungimea ca sa nu depasim buffer-ul
     * 
     * O_CREAT = creeaza fisierul daca nu exista
     * O_WRONLY = deschide doar pentru scriere
     * 0664 = rw-rw-r-- (owner si group pot citi/scrie, others doar citesc)
     */
    snprintf(path, sizeof(path), "%s/reports.dat", district);
    int fd = open(path, O_CREAT | O_WRONLY, 0664);
    if (fd == -1) { perror("open reports.dat"); exit(1); }
    close(fd);
    /* chmod seteaza explicit permisiunile - garanteaza ca sunt corecte
     * chiar daca umask-ul sistemului ar fi modificat permisiunile la open() */
    chmod(path, 0664);

    /*
     * district.cfg are permisiuni 0640:
     *   6 = rw- pentru owner (manager poate citi si scrie)
     *   4 = r-- pentru group (inspector poate doar citi)
     *   0 = --- pentru others
     * 
     * Scriem valoarea default a threshold-ului: "threshold=1"
     */
    snprintf(path, sizeof(path), "%s/district.cfg", district);
    fd = open(path, O_CREAT | O_WRONLY, 0640);
    if (fd == -1) { perror("open district.cfg"); exit(1); }
    write(fd, "threshold=1\n", 12);  // 12 = lungimea exacta a stringului
    close(fd);
    chmod(path, 0640);

    /*
     * logged_district are permisiuni 0644:
     *   6 = rw- pentru owner (manager poate citi si scrie)
     *   4 = r-- pentru group (inspector poate doar citi)
     *   4 = r-- pentru others (toti pot citi)
     */
    snprintf(path, sizeof(path), "%s/logged_district", district);
    fd = open(path, O_CREAT | O_WRONLY, 0644);
    if (fd == -1) { perror("open logged_district"); exit(1); }
    close(fd);
    chmod(path, 0644);
}

/*
 * write_log() - Scrie o intrare in jurnalul districtului
 * 
 * Doar managerul poate scrie in log.
 * Fiecare intrare contine: timestamp, rol, user, actiune
 * ex: "[Sat Apr 18 12:00:00 2026] role=manager user=alice action=add_report"
 */
void write_log(const char *district, const char *role, const char *user, const char *action) {
    char path[256];
    snprintf(path, sizeof(path), "%s/logged_district", district);

    /*
     * Verificam permisiunile - inspectorul nu poate scrie in log.
     * Conform tabelului din cerinta, logged_district are permisiunile 644:
     * doar owner-ul (managerul) poate scrie.
     */
    struct stat st;
    stat(path, &st);
    if (!strcmp(role, "inspector")) {
        printf("Error: inspector cannot write to log\n");
        return;
    }

    /*
     * O_WRONLY = doar scriere
     * O_APPEND = adauga la sfarsitul fisierului (nu suprascrie)
     */
    int fd = open(path, O_WRONLY | O_APPEND);
    if (fd == -1) { perror("open log"); return; }

    /*
     * time(NULL) returneaza timpul curent in secunde de la 1 Ian 1970
     * ctime() il converteste in format human-readable:
     *   ex: "Sat Apr 18 12:00:00 2026\n"
     * Scoatem \n-ul de la final cu timestr[strlen-1] = '\0'
     */
    time_t now = time(NULL);
    char *timestr = ctime(&now);
    timestr[strlen(timestr)-1] = '\0';

    /* construim linia de log si o scriem in fisier */
    char entry[512];
    int len = snprintf(entry, sizeof(entry), "[%s] role=%s user=%s action=%s\n",
                       timestr, role, user, action);
    write(fd, entry, len);
    close(fd);
}

/*
 * create_symlink() - Creaza un symlink catre reports.dat al districtului
 * 
 * Creeaza: active_reports-<district> -> <district>/reports.dat
 * ex:      active_reports-downtown   -> downtown/reports.dat
 * 
 * unlink() sterge symlink-ul vechi daca exista, ca sa nu dea eroare
 * symlink() creeaza legatura simbolica noua
 */
void create_symlink(const char *district) {
    char link_name[256];
    char target[256];

    snprintf(link_name, sizeof(link_name), "active_reports-%s", district);
    snprintf(target, sizeof(target), "%s/reports.dat", district);

    /* sterge symlink-ul vechi daca exista */
    unlink(link_name);

    if (symlink(target, link_name) == -1) {
        perror("symlink");
    }
}

/*
 * mode_to_string() - Converteste permisiunile dintr-un numar in format simbolic
 * 
 * ex: 0664 -> "rw-rw-r--"
 * 
 * st_mode contine permisiunile ca biti. Folosim operatorul & (AND pe biti)
 * cu macrourile S_IRUSR, S_IWUSR, etc. ca sa verificam fiecare bit individual.
 * 
 * Macrourile sunt:
 *   S_IRUSR = bitul de citire pentru owner
 *   S_IWUSR = bitul de scriere pentru owner
 *   S_IXUSR = bitul de executie pentru owner
 *   S_IRGRP = bitul de citire pentru group
 *   S_IWGRP = bitul de scriere pentru group
 *   S_IXGRP = bitul de executie pentru group
 *   S_IROTH = bitul de citire pentru others
 *   S_IWOTH = bitul de scriere pentru others
 *   S_IXOTH = bitul de executie pentru others
 */
void mode_to_string(mode_t mode, char *str) {
    /* owner (primele 3 caractere) */
    str[0] = (mode & S_IRUSR) ? 'r' : '-';
    str[1] = (mode & S_IWUSR) ? 'w' : '-';
    str[2] = (mode & S_IXUSR) ? 'x' : '-';
    /* group (urmatoarele 3 caractere) */
    str[3] = (mode & S_IRGRP) ? 'r' : '-';
    str[4] = (mode & S_IWGRP) ? 'w' : '-';
    str[5] = (mode & S_IXGRP) ? 'x' : '-';
    /* others (ultimele 3 caractere) */
    str[6] = (mode & S_IROTH) ? 'r' : '-';
    str[7] = (mode & S_IWOTH) ? 'w' : '-';
    str[8] = (mode & S_IXOTH) ? 'x' : '-';
    /* terminator de string */
    str[9] = '\0';
}

/* ================================================================
 * Urmatoarele 2 functii au fost generate cu ajutorul AI (Claude).
 * Vezi ai_usage.md pentru detalii despre prompturi si modificari.
 * ================================================================ */

/*
 * parse_condition() - Imparte o conditie "field:operator:value" in 3 parti
 * 
 * ex: "severity:>=:2" -> field="severity", op=">=", value="2"
 * 
 * Returneaza 1 daca parsarea a reusit, 0 daca formatul e invalid.
 */
int parse_condition(const char *input, char *field, char *op, char *value) {
    /*
     * Facem o copie locala a input-ului ca sa nu modificam string-ul original.
     * strncpy copiaza maxim sizeof(copy)-1 caractere, deci e sigur.
     */
    char copy[256];
    strncpy(copy, input, sizeof(copy) - 1);
    copy[255] = '\0';

    /*
     * strchr(copy, ':') cauta primul ':' in string si returneaza
     * un pointer la acea pozitie (sau NULL daca nu gaseste).
     * 
     * *first = '\0' pune un terminator de string la pozitia ':',
     * practic "taind" string-ul in doua:
     *   inainte: "severity:>=:2"
     *   dupa:    copy="severity"  |  first+1=">=:2"
     */
    char *first = strchr(copy, ':');
    if (first == NULL) return 0;
    *first = '\0';

    /*
     * Cautam al doilea ':' incepand de la first+1 (dupa primul ':')
     * Dupa taiere:
     *   first+1 = ">="  |  second+1 = "2"
     */
    char *second = strchr(first + 1, ':');
    if (second == NULL) return 0;
    *second = '\0';

    /* copiem cele 3 parti in parametrii de output */
    strncpy(field, copy, 31);       // ex: "severity"
    strncpy(op, first + 1, 3);      // ex: ">="
    strncpy(value, second + 1, 63); // ex: "2"

    /* adaugam terminatori expliciti ca masura de siguranta */
    field[31] = '\0';
    op[3] = '\0';
    value[63] = '\0';

    return 1;
}

/*
 * match_condition() - Verifica daca un raport satisface o conditie
 * 
 * Returneaza 1 daca raportul satisface conditia, 0 altfel.
 * 
 * Suporta campurile: severity, timestamp, category, inspector
 * Suporta operatorii: ==, !=, <, <=, >, >=
 */
int match_condition(Report *r, const char *field, const char *op, const char *value) {
    /*
     * COMPARE este un macro care face comparatia corecta in functie de operator.
     * In loc sa scriem 6 if-uri de fiecare data, folosim macro-ul o singura data.
     * 
     * ex: COMPARE(r->severity, 2) cu op=">=" va returna r->severity >= 2
     * 
     * Operatorul ternar ?: functioneaza asa:
     *   conditie ? valoare_daca_true : valoare_daca_false
     * Sunt inlantuite pentru a verifica fiecare operator posibil.
     */
    #define COMPARE(a, b) ( \
        strcmp(op, "==") == 0 ? (a) == (b) : \
        strcmp(op, "!=") == 0 ? (a) != (b) : \
        strcmp(op, "<")  == 0 ? (a) <  (b) : \
        strcmp(op, "<=") == 0 ? (a) <= (b) : \
        strcmp(op, ">")  == 0 ? (a) >  (b) : \
        strcmp(op, ">=") == 0 ? (a) >= (b) : 0)

    if (strcmp(field, "severity") == 0) {
        /*
         * atoi() converteste stringul "2" in numarul intreg 2
         * dupa comparam cu r->severity folosind macro-ul COMPARE
         */
        int val = atoi(value);
        return COMPARE(r->severity, val);
    }

    if (strcmp(field, "timestamp") == 0) {
        /*
         * atol() converteste stringul in long (time_t e de tip long)
         * timestamp-ul e numarul de secunde de la 1 Ian 1970
         */
        time_t val = (time_t)atol(value);
        return COMPARE(r->timestamp, val);
    }

    if (strcmp(field, "category") == 0) {
        /*
         * Pentru stringuri nu putem compara direct cu ==
         * strcmp() returneaza:
         *   0  daca stringurile sunt egale
         *   <0 daca primul e mai mic alfabetic
         *   >0 daca primul e mai mare alfabetic
         * Asa ca comparam rezultatul lui strcmp cu 0
         */
        int cmp = strcmp(r->category, value);
        return COMPARE(cmp, 0);
    }

    if (strcmp(field, "inspector") == 0) {
        /* similar cu category, comparam stringuri prin strcmp */
        int cmp = strcmp(r->inspector, value);
        return COMPARE(cmp, 0);
    }

    printf("Error: unknown field '%s'\n", field);
    return 0;

    #undef COMPARE  /* eliberam macro-ul dupa folosire - buna practica */
}