#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>    // fork, getpid
#include <signal.h>    // sigaction, kill
#include <fcntl.h>
#include <sys/wait.h>
#include <errno.h>
#include <dirent.h>


#define CMD_FILE "monitorCommands.txt" // hub writes commands to .txt
//putea fi si un "/tmp/treasure_command.txt" - temporar

#define MAX_CMD 256 //user input max size

pid_t monitor_pid = -1; // to send signals
int monitor_alive = 0;  // flag ->1 ruleaza, 0 oprit

int monitor_pipe[2]; // 0 read, 1 write
int waiting_monitor_exit = 0; // sa blochez comenzi pt stop_monitor


// handler activ cand monitor moare
void sigchld_handler(int signum) {
    int status;
    //waitpid(monitor_pid, &status, 0); //asteapta

    pid_t pid = waitpid(-1, &status, WNOHANG);

    if (pid == monitor_pid) {
        printf("Monitor process terminated with code %d!\n", WEXITSTATUS(status));
        monitor_alive = 0;
        waiting_monitor_exit = 0;
    }
}


// write command to file, monitor citeste
void write_command(const char* command) {
    int fd = open(CMD_FILE, O_WRONLY | O_CREAT | O_TRUNC, 0666); // opens file, empties file
    if (fd == -1) {
        perror("Could not open command file");
        return;
    }
    write(fd, command, strlen(command));
    close(fd);
}

// start the monitor process fork + execl
void start_monitor() {
    if (monitor_alive) {
        printf("Monitor already running!\n");
        return;
    }

    if (pipe(monitor_pipe) == -1) {
        perror("Error at creating a pipe for monitor!");
        exit(1);
    }

    monitor_pid = fork(); // create child and verifies
    if (monitor_pid == -1) {
        perror("Error: Failed to fork monitor!");
        exit(1);
    }

    if (monitor_pid == 0) {//cod fiu(monitor)

        close(monitor_pipe[0]); // inchide capat citire
        dup2(monitor_pipe[1], STDOUT_FILENO); // stdout merge in pipe
        close(monitor_pipe[1]);

        execl("./monitor", "monitor", NULL); // changes the child's code to ./monitor
        // inlocuieste procesul cu program
        perror("Monitor execution failed!\n");
        exit(1);

    }
    else {
        close(monitor_pipe[1]); // inchide capat scriere
        monitor_alive = 1;
        printf("Monitor started with PID %d\n", monitor_pid);
    }
}

void read_monitor_output() { //cit date din pipe[0], afis pe ecran
    if (!monitor_alive) {
        printf(">>>>>>>>>>>>>>> Output from monitor: <<<<<<<<<<<<<<<\n(none — monitor is not running)\n\n");
        return;
    }


    char buffer[256];
    int n;
    printf(">>>>>>>>>>>>>>> Output from monitor: <<<<<<<<<<<<<<<\n");
    while ((n = read(monitor_pipe[0], buffer, sizeof(buffer) - 1)) > 0) {
        buffer[n] = '\0';
        printf("%s", buffer);
        if (n < sizeof(buffer) - 1) break; // iesire
    }
    printf("\n");
}


// sends signals to monitor (process)
void send_signal(int sig) {
    if (!monitor_alive) {
        printf("Monitor is not running! Enter the command <start_monitor> first!\n");
        return;
    }
    kill(monitor_pid, sig);// trm semnalul sig la  monitor_pid
}

int main() {
    char input[MAX_CMD];

    struct sigaction sa; // for sigchld, to see when the monitor dies
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask); // setez masca sa nu blocheze alte semnale dc ruleaza sigchld
    sa.sa_flags = 0;
    sigaction(SIGCHLD, &sa, NULL);
    // monitor moare, sigchld e trimis la hub

    while (1) {
        usleep(2000);
        printf("hub : "); // prompt
        fflush(stdout); //afis imediata

        if (!fgets(input, sizeof(input), stdin)) break; // input from user
        input[strcspn(input, "\n")] = 0; // to delete the newline character

        if (waiting_monitor_exit) {
            printf("Monitor is shutting down... Please wait!\n");
            continue;
        } // blocare pana monitor se opreste

        else if (strcmp(input, "exit") == 0) {
            if (monitor_alive) {
                if (waiting_monitor_exit) {
                    int status;
                    while (waitpid(monitor_pid, &status, WNOHANG) == 0) {
                        printf("Waiting for monitor to shut down...\n");
                        sleep(1);
                    }
                    printf("Monitor process terminated with code %d!\n", WEXITSTATUS(status));
                    monitor_alive = 0;
                    waiting_monitor_exit = 0;
                }
                else {
                    printf("Warning: Monitor is still running! Use the command <stop_monitor> first!\n");
                    continue;
                }
            }
            break;
        }

        else if (strcmp(input, "start_monitor") == 0) {
            start_monitor();
        }
        else if (strcmp(input, "stop_monitor") == 0) {
            if (!monitor_alive) {
                printf("Monitor is not running! Enter the command <start_monitor> first!\n");
                continue;
            }

            write_command("stop");
            send_signal(SIGUSR2); // writes stop in file then sends SIGUSR2 to stop monitor
            read_monitor_output();
            waiting_monitor_exit = 1; // blocheaza comenzi
        }
        else if (strncmp(input, "list_hunts", 10) == 0 ||
            strncmp(input, "list_treasures", 14) == 0 ||
            strncmp(input, "view_treasure", 13) == 0) // strncmp pt ca pot avea argumente dupa ele
        {

            write_command(input);
            send_signal(SIGUSR1); // notifies the monitor to read
            // printf("Command sent to monitor.\n");
            read_monitor_output();
        }

        else if (strcmp(input, "calculate_score") == 0) {
            DIR* dir = opendir(".");
            if (!dir) {
                perror("Failed to open current directory!\n");
                continue;
            }

            struct dirent* entry;
            while ((entry = readdir(dir)) != NULL) { //cit urm elem din ac folder
                if (entry->d_type == DT_DIR &&
                    strcmp(entry->d_name, ".") != 0 &&
                    strcmp(entry->d_name, "..") != 0) {

                    int pfd[2];
                    if (pipe(pfd) == -1) {
                        perror("Pipe failed!\n");
                        continue;
                    }

                    pid_t pid = fork();
                    if (pid == -1) {
                        perror("Fork failed!\n");
                        close(pfd[0]); close(pfd[1]);
                        continue;
                    }

                    if (pid == 0) { // copil
                        close(pfd[0]);
                        dup2(pfd[1], STDOUT_FILENO);
                        close(pfd[1]);

                        execl("./calculate_score", "calculate_score", entry->d_name, NULL);
                        perror("Exec failed!\n");
                        exit(1);
                    }
                    else { // parinte

                        close(pfd[1]);
                        char buffer[256];
                        int n;
                        printf("\n>>> Score output for hunt: %s\n", entry->d_name);
                        while ((n = read(pfd[0], buffer, sizeof(buffer) - 1)) > 0)
                        {
                            buffer[n] = '\0';
                            printf("%s", buffer);
                        }

                        close(pfd[0]);
                        waitpid(pid, NULL, 0);
                    }
                }
            }
            closedir(dir);
        }

        else {
            printf("Entered an unknown command!\n");
        }

    }

    return 0;
}