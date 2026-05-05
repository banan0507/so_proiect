#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>

#define PID_FILE ".monitor_pid"

/*
 * Handler pentru SIGUSR1 - apelat cand city_manager adauga un raport nou.
 * Afiseaza un mesaj cu timestamp-ul curent.
 * 
 * IMPORTANT: in signal handlers folosim doar functii async-signal-safe
 * (write() in loc de printf(), time() pentru timestamp)
 */
void handle_sigusr1(int sig) {
    time_t now = time(NULL);
    char *ts = ctime(&now);
    ts[strlen(ts)-1] = '\0';
    printf("[%s] Monitor: new report added!\n", ts);
    fflush(stdout);  // flush ca sa apara imediat in terminal
}

/*
 * Handler pentru SIGINT - apelat cand utilizatorul apasa Ctrl+C.
 * Sterge fisierul .monitor_pid si iese din program.
 */
void handle_sigint(int sig) {
    printf("\nMonitor: SIGINT received, shutting down...\n");
    fflush(stdout);

    // stergem fisierul .monitor_pid
    if (unlink(PID_FILE) == -1) {
        perror("unlink .monitor_pid");
    } else {
        printf("Monitor: .monitor_pid deleted\n");
    }

    exit(0);
}

int main() {
    /*
     * 1. Scrie PID-ul in .monitor_pid
     * 
     * getpid() returneaza PID-ul procesului curent.
     * Scriem PID-ul ca text in fisier ca sa il poata citi city_manager.
     * O_TRUNC = suprascrie fisierul daca exista deja
     */
    pid_t my_pid = getpid();

    int fd = open(PID_FILE, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) {
        perror("open .monitor_pid");
        exit(1);
    }

    char pid_str[32];
    int len = snprintf(pid_str, sizeof(pid_str), "%d\n", my_pid);
    write(fd, pid_str, len);
    close(fd);

    printf("Monitor started with PID %d\n", my_pid);
    printf("Waiting for signals... (Ctrl+C to stop)\n");
    fflush(stdout);

    /*
     * 2. Configureaza handler-ele de semnale cu sigaction()
     * 
     * sigaction() e mai sigur decat signal() pentru ca:
     *   - comportamentul e bine definit pe toate platformele
     *   - permite configurarea mai detaliata (sa_flags)
     * 
     * struct sigaction contine:
     *   sa_handler = functia handler
     *   sa_mask    = semnale blocate in timpul handler-ului
     *   sa_flags   = optiuni extra (0 = default)
     */
    struct sigaction sa_usr1;
    memset(&sa_usr1, 0, sizeof(sa_usr1));
    sa_usr1.sa_handler = handle_sigusr1;
    sigemptyset(&sa_usr1.sa_mask);  // nu blocam niciun semnal extra
    sa_usr1.sa_flags = 0;

    if (sigaction(SIGUSR1, &sa_usr1, NULL) == -1) {
        perror("sigaction SIGUSR1");
        exit(1);
    }

    struct sigaction sa_int;
    memset(&sa_int, 0, sizeof(sa_int));
    sa_int.sa_handler = handle_sigint;
    sigemptyset(&sa_int.sa_mask);
    sa_int.sa_flags = 0;

    if (sigaction(SIGINT, &sa_int, NULL) == -1) {
        perror("sigaction SIGINT");
        exit(1);
    }

    /*
     * 3. Bucla infinita - asteptam semnale
     * 
     * pause() suspenda procesul pana vine un semnal.
     * Dupa ce handlerul e executat, pause() returneaza
     * si bucla while continua - asteptam urmatorul semnal.
     */
    while (1) {
        pause();
    }

    return 0;
}