#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#define USERNAME_SIZE 50
#define CLUE_SIZE 256
#define MAX_USERS 100

typedef struct {
    int id;
    char username[USERNAME_SIZE];
    float latitude;
    float longitude;
    int value;
    char clue[CLUE_SIZE];
} Treasure;

typedef struct {
    char username[USERNAME_SIZE];
    int score;
} UserScore;

int main(int argc, char* argv[]) {
    if (argc != 2) {
        printf("Usage: %s <hunt_id>\n", argv[0]);
        return 1;
    }

    char filePath[256];
    snprintf(filePath, sizeof(filePath), "%s/%s_treasures.dat", argv[1], argv[1]);

    int fd = open(filePath, O_RDONLY);
    if (fd == -1) {
        perror("Failed to open treasure file!\n");
        return 1;
    }

    UserScore users[MAX_USERS];
    int userCount = 0;

    Treasure t;
    while (read(fd, &t, sizeof(Treasure)) == sizeof(Treasure)) {
        int found = 0;
        for (int i = 0; i < userCount; i++) {
            if (strcmp(users[i].username, t.username) == 0) {
                users[i].score += t.value;
                found = 1;
                break;
            }
        }
        if (!found && userCount < MAX_USERS) {
            strcpy(users[userCount].username, t.username);
            users[userCount].score = t.value;
            userCount++;
        }
    }

    close(fd);

    printf("Scores for hunt '%s':\n", argv[1]);
    for (int i = 0; i < userCount; i++) {
        printf(" - %s: %d\n", users[i].username, users[i].score);
    }

    return 0;
}