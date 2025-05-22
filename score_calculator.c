#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#define USERNAME_SIZE 50
#define CLUE_SIZE 256

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

UserScore scores[100];
int score_count = 0;

void add_score(const char* username, int value) {
    for (int i = 0; i < score_count; i++) {
        if (strcmp(scores[i].username, username) == 0) {
            scores[i].score += value;
            return;
        }
    }
    strcpy(scores[score_count].username, username);
    scores[score_count].score = value;
    score_count++;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <hunt_id>\n", argv[0]);
        return 1;
    }

    char filepath[256];
    snprintf(filepath, sizeof(filepath), "%s/%s_treasures.dat", argv[1], argv[1]);

    int fd = open(filepath, O_RDONLY);
    if (fd == -1) {
        perror("Failed to open treasure file");
        return 1;
    }

    Treasure t;
    while (read(fd, &t, sizeof(Treasure)) == sizeof(Treasure)) {
        add_score(t.username, t.value);
    }

    close(fd);

    for (int i = 0; i < score_count; i++) {
        printf("%s: %d\n", scores[i].username, scores[i].score);
    }

    return 0;
}
