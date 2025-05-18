#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>    // fork, getpid
#include <signal.h>    // sigaction, kill
#include <fcntl.h>
#include <sys/wait.h>
#include <errno.h>


#define CMD_FILE "monitorCommands.txt" // hub writes commands to .txt
//putea fi si un "/tmp/treasure_command.txt" - temporar

#define MAX_CMD 256 //user input max size

pid_t monitor_pid = -1; // to send signals
int monitor_alive = 0;  // flag ->1 ruleaza, 0 oprit

// handler activ cand monitor moare
void sigchld_handler(int signum) {
    int status;
    waitpid(monitor_pid, &status, 0); //asteapta
    printf("Monitor process terminated with code %d!\n", WEXITSTATUS(status));
    monitor_alive = 0; // monitor oprit
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

    monitor_pid = fork(); // create child and verifies
    if (monitor_pid == -1) {
        perror("Error: Failed to fork monitor!");
        exit(1);
    }

    if (monitor_pid == 0) {//cod fiu
        //printf("Child: trying to execl ./monitor...\n");

        /*
        int null_fd = open("/dev/null", O_RDONLY);
        if (null_fd >= 0) {
          dup2(null_fd, STDIN_FILENO); // redir stdin
          close(null_fd);
      } */

        execl("./monitor", "monitor", NULL); // changes the child's code to ./monitor
        // inlocuieste procesul cu program

        perror("Failed to start monitor from child process!");
        exit(1);
    }
    else {
        monitor_alive = 1;
        printf("Monitor started with PID %d\n", monitor_pid);
    }
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
        printf("hub : "); // prompt
        fflush(stdout); //afis imediata

        if (!fgets(input, sizeof(input), stdin)) break; // input from user 
        input[strcspn(input, "\n")] = 0; // to delete the newline character

        if (strcmp(input, "exit") == 0) {
            if (monitor_alive) {
                // nu permite iesirea, cere oprirea monitorului ca sa nu ajunga in starea de zombie
                printf("Warning: Monitor is still running! Usethe command <stop_monitor> first!\n");
            }
            else {
                break;
            }
        }
        else if (strcmp(input, "start_monitor") == 0) {
            start_monitor();
        }
        else if (strcmp(input, "stop_monitor") == 0) {
            write_command("stop");
            send_signal(SIGUSR2); // writes stop in file then sends SIGUSR2 to stop monitor
        }
        else if (strncmp(input, "list_hunts", 10) == 0 ||
            strncmp(input, "list_treasures", 14) == 0 ||
            strncmp(input, "view_treasure", 13) == 0)
        {
            // strncmp pt ca pot avea argumente dupa ele
            write_command(input);
            send_signal(SIGUSR1); // notifies the monitor to read
            // printf("Command sent to monitor.\n");
        }
        else {
            printf("Entered an unknown command!\n");
        }
    }

    return 0;
}
