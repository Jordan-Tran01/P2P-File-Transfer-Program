#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <config.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>

int parse_config(const char *filename, Config *config) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Failed to open file");
        return 1;
    }

    char line[256];
    while (fgets(line, sizeof(line), file)) {
        if (sscanf(line, "directory:%255s", config->directory) == 1) continue;
        if (sscanf(line, "max_peers:%d", &config->max_peers) == 1) continue;
        if (sscanf(line, "port:%hu", &config->port) == 1) continue;
    }

    DIR* dir = opendir(config->directory);
    if (dir) {
        closedir(dir);
    } else {
        if (mkdir(config->directory, 0777) == -1) {
            // Failed to create directory
            perror("Error creating directory");
            return 3;
        }
    }

    if (config->max_peers < 1 || config->max_peers > 2048) {
        fprintf(stderr, "Invalid number of peers\n");
        return 4;
    }

    if (config->port <= 1024 || config->port > 65535) {
        fprintf(stderr, "Unacceptable port range\n");
        return 5;
    }

    fclose(file);
    return 0;
}