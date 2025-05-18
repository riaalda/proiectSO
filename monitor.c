// reacts to signals, reads from command.txt, runs commands with treasure_manager

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <string.h>
#include <sys/wait.h>

#define CMD_FILE "monitorCommands.txt" //hub writes commands to .txt
#define MAX_CMD 256

int stop_requested = 0; // if 1, then monitor stops
int command_ready = 0; // if 1,there is a new command

void handle_sigusr1(int sig)
{ //process a command
    command_ready = 1;
}

void handle_sigusr2(int sig)
{ // to stop
    stop_requested = 1;
}

void setup_signal_handlers()
{
    struct sigaction sa_usr1 = { 0 };
    sa_usr1.sa_handler = handle_sigusr1;
    sa_usr1.sa_flags = SA_RESTART;
    sigaction(SIGUSR1, &sa_usr1, NULL);

    struct sigaction sa_usr2 = { 0 };
    sa_usr2.sa_handler = handle_sigusr2;
    sa_usr2.sa_flags = SA_RESTART;
    sigaction(SIGUSR2, &sa_usr2, NULL);
}

void execute_command()
{
    char buffer[MAX_CMD] = { 0 };
    int fd = open(CMD_FILE, O_RDONLY);

    if (fd == -1) {
        perror("[Monitor] Could not open command file");
        return;
    }

    int n = read(fd, buffer, sizeof(buffer) - 1);
    if (n <= 0) {
        perror("[Monitor] Could not read command");
        close(fd);
        return;
    }
    buffer[n] = '\0'; // validate the string
    close(fd);

    printf("[Monitor] Command received: %s\n", buffer);


    if (strncmp(buffer, "list_treasures", 14) == 0)
    {
        char hunt_id[64];
        sscanf(buffer, "list_treasures %s", hunt_id);

        pid_t pid = fork();
        if (pid == 0)
        {
            execlp("./treasure_manager", "treasure_manager", "--list", hunt_id, NULL);
            // apel spre manager pt optiunea --list <hunt_id>
            perror("execution failed");
            exit(1);
        }
        else if (pid < 0) {
            perror("fork failed!\n");
        }
    }
    else if (strncmp(buffer, "view_treasure", 13) == 0)
    {
        char hunt_id[64];
        int tid;

        if (sscanf(buffer, "view_treasure %s %d", hunt_id, &tid) != 2) {
            printf("[Monitor] Invalid usage: view_treasure requires <hunt_id> and <treasure_id>\n");
            return;
        }

        char id_str[16];
        snprintf(id_str, sizeof(id_str), "%d", tid);
        //extract the hunt name and id

        pid_t pid = fork();
        if (pid == 0)
        {
            execlp("./treasure_manager", "treasure_manager", "--view", hunt_id, id_str, NULL);
            perror("exec failed");
            exit(1);
        }
        else if (pid < 0) { perror("Error: fork failed!\n"); }
    }

    else if (strncmp(buffer, "list_hunts", 10) == 0) {
        pid_t pid = fork();
        if (pid == 0) {
            execlp("./treasure_manager", "treasure_manager", "--hunts", NULL);
            perror("exec failed");
            exit(1);
        }
        else if (pid < 0) {
            perror("Error: fork failed");
        }
    }

    else if (strncmp(buffer, "stop", 4) == 0)
    {
        printf("[Monitor] Stop command received.\n");
        stop_requested = 1; // set the flag
    }

    else {
        printf("[Monitor] Unknown command: %s !\n", buffer);
    }
}

int main()
{
    setup_signal_handlers();

    printf("[Monitor] Running... PID: %d\n", getpid()); // pid for monitor process

    while (!stop_requested) //wait for signals
    {
        pause();  // block the process and wait for signal
        if (command_ready) { // daca e sigusr1, ready==1, execute command
            command_ready = 0;
            execute_command();
            while (waitpid(-1, NULL, WNOHANG) > 0);//procese copil ce s au terminat
        }
    }

    printf("[Monitor] Stopping...\n");
    usleep(2000);  // 2sec
    printf("[Monitor] Exit done!\n");
    return 0;
}
