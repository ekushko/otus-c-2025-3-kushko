#include "scoreboard.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define FILENAME "scoreboard.txt"
#define SCOREBOARD_LEN 10
#define DELIM "\t\t"

char* names[SCOREBOARD_LEN];
int   scores[SCOREBOARD_LEN];

void scoreboard_load() {
    FILE* fp = fopen(FILENAME, "r");

    if (fp == NULL) {
        return;
    }

    char*   line = NULL;
    size_t  len = 0;

    for (int i = 0; i < SCOREBOARD_LEN; ++i) {
        if (getline(&line, &len, fp) == -1) {
            break;
        }
        int place = 0;
        char*  name = NULL;
        int    score = -1;

        char* token = strtok(line, DELIM);
        if (token != NULL) {
            place = atoi(token) - 1;
        }
        token = strtok(NULL, DELIM);
        if (token != NULL) {
            size_t name_len = strlen(token);
            name = malloc(name_len + 1);
            memcpy(name, token, name_len);
            name[name_len] = '\0';
        }
        token = strtok(NULL, DELIM);
        if (token != NULL) {
            score = atoi(token);
        }

        if (place < SCOREBOARD_LEN && name != NULL && score >= 0) {
            scores[place] = score;

            size_t name_len = strlen(name);
            names[place] = malloc(name_len + 1);
            memcpy(names[place], name, name_len);
            names[place][name_len] = '\0';
            free(name);
        }
    }

    fclose(fp);
}

void scoreboard_save() {
    FILE* fp = fopen(FILENAME, "w");

    if (fp == NULL) {
        perror(FILENAME);
        return;
    }

    for (int i = 0; i < SCOREBOARD_LEN; ++i) {
        fprintf(fp, "%d%s%s%s%d\n", i + 1, DELIM, names[i], DELIM, scores[i]);
    }

    fclose(fp);
}

void scoreboard_insert(const char* name, int score) {
    int place = -1;
    for (int i = 0; i < SCOREBOARD_LEN; ++i) {
        if (score >= scores[i]) {
            place = i;
            break;
        }
    }
    if (place >= 0) {
        if (names[SCOREBOARD_LEN - 1] != NULL) {
            free(names[SCOREBOARD_LEN - 1]);
        }

        for (int i = SCOREBOARD_LEN - 1; i > place; --i) {
            scores[i] = scores[i-1];
            names[i] = names[i-1];
        }

        scores[place] = score;

        size_t name_len = strlen(name);
        names[place] = malloc(name_len + 1);
        memcpy(names[place], name, name_len);
        names[place][name_len] = '\0';
    }
}

void scoreboard_init() {
    for (int i = 0; i < SCOREBOARD_LEN; ++i) {
        names[i] = NULL;
        scores[i] = -1;
    }

    for (int i = 0; i < SCOREBOARD_LEN; ++i) {
        scoreboard_insert("PLAYER", i * 100);
    }

    scoreboard_load();
}

void scoreboard_free() {
    scoreboard_save();

    for (int i = 0; i < SCOREBOARD_LEN; ++i) {
        if (names[i] != NULL) {
            free(names[i]);
        }
    }
}


