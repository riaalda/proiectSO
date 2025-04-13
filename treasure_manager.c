#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include <sys/types.h>
#include <dirent.h>
#include <time.h>

#define USERNAME_SIZE 50
#define CLUE_SIZE 256
#define PATH_SIZE 256

typedef struct {
    int id;
    char username[USERNAME_SIZE];
    float latitude;
    float longitude;
    int value;
    char clue[CLUE_SIZE];
} Treasure;

void printTreasure(Treasure *t) {
    printf("Treasure ID: %d\n", t->id);
    printf("Username: %s\n", t->username);
    printf("Coordinates: latitude=%.2f longitude=%.2f\n", t->latitude, t->longitude);
    printf("Value: %d\n", t->value);
    printf("Clue: %s\n", t->clue);
    printf("-----------------------------------\n");
}

// log activity
void addLog(const char *huntID, const char *message) {
    char logPath[PATH_SIZE];
    snprintf(logPath, sizeof(logPath), "%s/logged_hunt.log", huntID);

    FILE *logFile = fopen(logPath, "a");
    if (!logFile) {
        perror("Could not open log file");
        return;
    }

    fprintf(logFile, "%s\n", message);
    fclose(logFile);
}

int addTreasure(Treasure *t, const char *huntID) {
    char dirPath[PATH_SIZE], treasureFile[PATH_SIZE];
    snprintf(dirPath, sizeof(dirPath), "%s", huntID); // creates the folder huntID if it does NOT exist yet
    snprintf(treasureFile, sizeof(treasureFile), "%s/%s_treasures.dat", huntID, huntID); // writes the treasure in the file treasures.dat (binary file)

    if (mkdir(dirPath, 0777) == -1 && errno != EEXIST) {
        perror("mkdir failed!\n");
        return 0;
    } // verification for creating a folder

    int fd = open(treasureFile, O_WRONLY | O_CREAT | O_APPEND, 0777);
    if (fd == -1) {
        perror("Error while opening the treasure File!\n");
        return 0;
    }

    printf("Enter Treasure ID: ");
    scanf("%d", &t->id);
    getchar(); 

    printf("Enter Username: ");
    fgets(t->username, USERNAME_SIZE, stdin);
    t->username[strcspn(t->username, "\n")] = 0;

    printf("Enter Latitude: ");
    scanf("%f", &t->latitude);
    printf("Enter Longitude: ");
    scanf("%f", &t->longitude);
    getchar();

    printf("Enter Clue: ");
    fgets(t->clue, CLUE_SIZE, stdin);
    t->clue[strcspn(t->clue, "\n")] = 0;

    printf("Enter Value: ");
    scanf("%d", &t->value);

    // creates the file treasure
    if (write(fd, t, sizeof(Treasure)) != sizeof(Treasure)) {
        perror("Failed to write treasure to file");
        close(fd);
        return 0;
    }

    close(fd);
    printf("Treasure added successfully!\n");


    // call log function
    char logEntry[PATH_SIZE];
    snprintf(logEntry, sizeof(logEntry), "Added treasure with the ID %d in the hunt (ID : %s) by user %s", t->id, huntID, t->username);
    addLog(huntID, logEntry);


    return 1;
}

void viewTreasure(const char *huntID, int id) {

  // verification if the hunt file exists and if the treasure (with the treasure id) belongs to the specified hunt
  
    char treasureFile[PATH_SIZE];
    snprintf(treasureFile, sizeof(treasureFile), "%s/%s_treasures.dat", huntID, huntID);

    int fd = open(treasureFile, O_RDONLY);
    if (fd == -1) {
        perror("Error at opening the treasure file!\n");
        return;
    }

    Treasure t;
    while (read(fd, &t, sizeof(Treasure)) == sizeof(Treasure)) {
        if (t.id == id) {

	  // call log function
	  char logEntry[PATH_SIZE];
	  snprintf(logEntry, sizeof(logEntry), "Viewed treasure ID %d from hunt %s", id, huntID);
	  addLog(huntID, logEntry);

	  printTreasure(&t);
	  close(fd);
	  return;
        }
    }

    printf("Treasure with the following id: %d is not found!\n", id);

    close(fd);
}

void listTreasures(const char *huntID) {
    char treasureFile[PATH_SIZE];
    snprintf(treasureFile, sizeof(treasureFile), "%s/%s_treasures.dat", huntID, huntID);

    // Get file metadata
    struct stat fileStat;
    if (stat(treasureFile, &fileStat) == -1) {
        perror("Error retrieving treasure file info");
        return;
    }

    printf("Hunt: %s\n", huntID);
    printf("File size: %ld bytes\n", fileStat.st_size);
    printf("Last modified: %s\n", ctime(&fileStat.st_mtime));  // ctime adds \n automatically

    int fd = open(treasureFile, O_RDONLY);
    if (fd == -1) {
        perror("Error while opening the treasure file");
        return;
    }

    Treasure t;
    int found = 0;
    while (read(fd, &t, sizeof(Treasure)) == sizeof(Treasure)) {
        printTreasure(&t);
        found = 1;
    }

    if (!found) {
        printf("No treasures found in the following hunt %s!\n", huntID);
    } else {
        // call log function
        char logEntry[PATH_SIZE];
        snprintf(logEntry, sizeof(logEntry), "Listed all the treasures from the hunt (ID : %s)", huntID);
        addLog(huntID, logEntry);
    }

    close(fd);
}


void removeTreasure(const char *huntID, int id) {
    char treasureFile[PATH_SIZE], tempFile[PATH_SIZE];
    snprintf(treasureFile, sizeof(treasureFile), "%s/%s_treasures.dat", huntID, huntID);
    snprintf(tempFile, sizeof(tempFile), "%s/temp.dat", huntID);

    int fd = open(treasureFile, O_RDONLY);
    if (fd == -1) {
        perror("Error while opening the treasure file!\n");
        return;
    }

    int tempFd = open(tempFile, O_WRONLY | O_CREAT | O_TRUNC, 0777); // temporary file that will replace the original one 
    if (tempFd == -1) {
        perror("Error while opening the temporary file!\n");
        close(fd);
        return;
    }

    Treasure t;
    int removed = 0;
    while (read(fd, &t, sizeof(Treasure)) == sizeof(Treasure)) {
        if (t.id != id) {
            write(tempFd, &t, sizeof(Treasure));
        } else {
            removed = 1; //treasure removed successfully

	    // call log function
	    char logEntry[PATH_SIZE];
	    snprintf(logEntry, sizeof(logEntry), "Removed treasure ID %d from the hunt %s", id, huntID);
	    addLog(huntID, logEntry);
        }
    }

    close(fd);
    close(tempFd);

    if (removed) {
        rename(tempFile, treasureFile);
        printf("Treasure with ID %d removed successfully!\n", id);
    } else {
        remove(tempFile);
        printf("Treasure with ID %d not found!\n", id);
    }

   
}

void removeHunt(const char *huntID) {
    DIR *dir = NULL;
    char dirPath[PATH_SIZE];
    char filePath[2 * PATH_SIZE];
    char loggedHuntPath[PATH_SIZE];

    snprintf(dirPath, sizeof(dirPath), "%s", huntID); // folder
    snprintf(loggedHuntPath, sizeof(loggedHuntPath), "logged_hunt-%s.dat", huntID); // log file

    if ((dir = opendir(dirPath)) == NULL) {
        perror("The directory does not exist or could not be opened --RemoveHunt()\n");
        exit(1);
    }

    if (remove(loggedHuntPath) != 0 && errno != ENOENT) {
    perror("Error at removing/finding the logged_hunt Path --RemoveHunt()");
    exit(1);
}

    struct dirent *entry = NULL;

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        snprintf(filePath, sizeof(filePath), "%s/%s", dirPath, entry->d_name);
        remove(filePath);
    }

    if (closedir(dir) == -1) {
        perror("Error at closing the dir --RemoveHunt()\n");
        exit(1);
    }

    if (rmdir(dirPath) == -1) {
        perror("Error at removing the directory --RemoveHunt()\n");
        exit(1);
    }

    printf("Hunt '%s' was removed succesfully!\n", huntID);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        perror("Wrong number of arguments given!\nCorrect format that should be given: treasure_manager <flag>\nPossible flags:\n--add <hunt id>\n--list <hunt id>\n--view <hunt id> <id>\n--remove_treasure <hunt id> <id>\n--remove_hunt <hunt id>\n");
        exit(1);
    }

    Treasure *treasure = NULL;

    if (strcmp(argv[1], "--add") == 0) {
        if (argc != 3) {
            perror("You should enter a hunt name\n");
            exit(1);
        }
        if (treasure == NULL) {
            if ((treasure = (Treasure *)malloc(sizeof(Treasure))) == NULL) {
                perror("Memory allocation failure for -treasure-\n");
                exit(1);
            }
        }

        addTreasure(treasure, argv[2]);
    }
    else if (strcmp(argv[1], "--list") == 0) {
        if (argc != 3) {
            perror("You should enter a hunt name\n");
            exit(1);
        }
        listTreasures(argv[2]);
    }
    else if (strcmp(argv[1], "--view") == 0) {
        if (argc != 4) {
            perror("You should enter a hunt name and a specific treasure you want to see\n");
            exit(1);
        }
        int id = atoi(argv[3]);
        viewTreasure(argv[2], id);
    }
    else if (strcmp(argv[1], "--remove_treasure") == 0) {
        if (argc != 4) {
            perror("You should enter a hunt name and the treasure ID to remove\n");
            exit(1);
        }
        int id = atoi(argv[3]);
        removeTreasure(argv[2], id);
    }
    else if (strcmp(argv[1], "--remove_hunt") == 0) {
        if (argc != 3) {
            perror("You should enter the name of a hunt you want to remove\n");
            exit(1);
        }
        removeHunt(argv[2]);
    }
    else {
        printf("You've entered an unknown command!\n");
    }

    return 0;
}