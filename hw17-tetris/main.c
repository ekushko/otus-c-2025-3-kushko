#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "scoreboard.h"
#include "game.h"

#define DEFAULT_NAME "PLAYER"

int main() {
    scoreboard_init();

    char        name[256];
    const char* default_name = DEFAULT_NAME;
    bool        has_getting_name_err = false;

    printf("Please, enter your name: ");

    if (fgets(name, sizeof(name), stdin) != NULL) {
        const size_t enter_pos = strcspn(name, "\n");
        if (enter_pos > 1) {
            name[strcspn(name, "\n")] = '\0';
            printf("You entered: '%s'\n", name);
        } else {
            has_getting_name_err = true;
        }
    } else {
        has_getting_name_err = true;
    }

    if (has_getting_name_err) {
        strcpy(name, default_name);
        printf("Getting name error! Used default: '%s'\n", name);
    }

    int score = run_game();
    scoreboard_insert(name, score);

    scoreboard_free();

    return 0;
}
