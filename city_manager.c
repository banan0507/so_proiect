#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <errno.h>
#include "city_operations.h"      // functiile principale: add, list, view, etc.
#include "secondary_functions.h"  // functiile helper: create_district, write_log, etc.
 
int main(int argc, char** argv) {
    /*
     * argc = numarul total de argumente (inclusiv numele programului)
     * argv = array de stringuri cu argumentele
     * 
     * Exemplu: ./city_manager --role manager --user alice --add downtown
     * argc = 6
     * argv[0] = "./city_manager"
     * argv[1] = "--role"
     * argv[2] = "manager"
     * argv[3] = "--user"
     * argv[4] = "alice"
     * argv[5] = "--add"
     * argv[6] = "downtown"
     */
 
    char *role = NULL;  // va fi "manager" sau "inspector"
    char *user = NULL;  // va fi numele utilizatorului (ex: "alice")
 
    /*
     * Parcurgem toate argumentele unul cate unul.
     * Incepem de la i=1 pentru ca argv[0] e numele programului.
     * Folosim ++i in interiorul conditiilor ca sa "sarim" peste
     * argumentul urmator (valoarea flagului curent).
     */
    for (int i = 1; i < argc; i++) {
 
        if (!strcmp(argv[i], "--role")) {
            /* 
             * !strcmp returneaza 1 (true) cand stringurile sunt EGALE
             * (strcmp returneaza 0 cand sunt egale, iar !0 = 1)
             * ++i trece la urmatorul argument si il salveaza in role
             * ex: daca argv[i] = "--role", atunci argv[++i] = "manager"
             */
            role = argv[++i];
 
        } else if (!strcmp(argv[i], "--user")) {
            /* similar cu --role, salveaza numele utilizatorului */
            user = argv[++i];
 
        } else if (!strcmp(argv[i], "--add")) {
            /* 
             * argv[++i] = numele districtului (ex: "downtown")
             * apeleaza functia add() din city_operations.c
             */
            char *district = argv[++i];
            add(district, role, user);
 
        } else if (!strcmp(argv[i], "--list")) {
            /* listeaza toate rapoartele din district */
            char *district = argv[++i];
            list(district, role, user);
 
        } else if (!strcmp(argv[i], "--view")) {
            /* 
             * are nevoie de 2 argumente: district si id
             * atoi() converteste stringul "17" in numarul intreg 17
             */
            char *district = argv[++i];
            int id = atoi(argv[++i]);
            view(district, id, role, user);
 
        } else if (!strcmp(argv[i], "--remove_report")) {
            /* 
             * similar cu --view, primeste district si id
             * doar managerul poate sterge (verificat in functie)
             */
            char *district = argv[++i];
            int id = atoi(argv[++i]);
            remove_report(district, id, role);
 
        } else if (!strcmp(argv[i], "--update_threshold")) {
            /* 
             * primeste district si noua valoare threshold
             * doar managerul poate actualiza (verificat in functie)
             */
            char *district = argv[++i];
            int val = atoi(argv[++i]);
            update_threshold(district, val, role);
 
        } else if (!strcmp(argv[i], "--filter")) {
            /*
             * --filter e special: dupa numele districtului urmeaza
             * oricati termeni de filtrare (conditii)
             * 
             * Exemplu: --filter downtown "severity:>=:2" "category:==:road"
             * district    = "downtown"
             * conditions  = { "severity:>=:2", "category:==:road" }
             * num_conditions = 2
             * 
             * &argv[i+1] = pointer la primul element dupa district
             * argc - i - 1 = numarul de conditii ramase
             * i = argc seteaza i la final ca sa opreasca bucla for
             *   (am consumat deja toate argumentele)
             * 
             * ATENTIE: in terminal, conditiile cu operatori <, >, >=, <=
             * trebuie puse in ghilimele ca sa nu fie interpretate de shell
             * ex: "severity:>=:2" nu severity:>=:2
             */
            char *district = argv[++i];
            char **conditions = &argv[i+1];
            int num_conditions = argc - i - 1;
            i = argc;
            filter(district, role, user, conditions, num_conditions);
        }
    }
 
    return 0;
}