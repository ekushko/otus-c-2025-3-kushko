#include "game.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ncurses.h>
#include <unistd.h>
#include <time.h>

#define ARENA_WIDTH      10
#define ARENA_HEIGHT     20
#define TETROMINO_WIDTH  4
#define TETROMINO_HEIGHT 4

const int tetrominoes[7][16] = {
    {0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0},  // I
    {0, 0, 0, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 0, 0, 0},  // O
    {0, 0, 0, 0, 0, 0, 1, 1, 0, 1, 1, 0, 0, 0, 0, 0},  // S
    {0, 0, 0, 0, 1, 1, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0},  // Z
    {0, 0, 0, 0, 0, 1, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0},  // T
    {0, 0, 0, 0, 1, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0},  // L
    {0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 1, 1, 0, 0, 0, 0}   // J
};

int  arena[ARENA_WIDTH][ARENA_HEIGHT];
int  curr_index = 0;
int  curr_rotation = 0;
int  curr_x = 0;
int  curr_y = 0;
bool is_game_over = false;
int  score = 0;

int rotate(int rotation, int x, int y) {
    const int direction = rotation % 4;

    if (direction == 0) {
        return x + y * TETROMINO_WIDTH;
    } else if (direction == 1) {
        return 12 + y - (x * TETROMINO_WIDTH);
    } else if (direction == 2) {
        return 15 - (y * TETROMINO_WIDTH) - x;
    } else if (direction == 3) {
        return 3 - y + (x * TETROMINO_WIDTH);
    }

    return 0;
}

bool check_position(int index, int rotation, int x, int y) {
    for (int x_idx = 0; x_idx < TETROMINO_WIDTH; ++x_idx) {
        for (int y_idx = 0; y_idx < TETROMINO_HEIGHT; ++y_idx) {
            int idx = rotate(rotation, x_idx, y_idx);
            if (tetrominoes[index][idx] != 1) {
                continue;
            }

            int x_arena = x_idx + x;
            int y_arena = y_idx + y;
            if (x_arena < 0 || x_arena >= ARENA_WIDTH || y_arena >= ARENA_HEIGHT) {
                return false;
            }

            int pos_arena = arena[x_arena][y_arena];
            if (y_arena >= 0 && pos_arena == 1) {
                return false;
            }
        }
    }
    return true;
}

void generate_tetromino() {
    curr_index = rand() % 7;
    curr_rotation = 0;
    curr_x = (ARENA_WIDTH / 2) - (TETROMINO_WIDTH / 2);
    curr_y = 0;
    is_game_over = !check_position(curr_index, curr_rotation, curr_x, curr_y);
}

bool move_down() {
    if (check_position(curr_index, curr_rotation, curr_x, curr_y + 1) == true) {
        curr_y += 1;
        return true;
    }
    return false;
}

void place_to_arena() {
    for (int y_idx = 0; y_idx < TETROMINO_HEIGHT; ++y_idx) {
        for (int x_idx = 0; x_idx < TETROMINO_WIDTH; ++x_idx) {
            int idx = rotate(curr_rotation, x_idx, y_idx);
            if (tetrominoes[curr_index][idx] != 1) {
                continue;
            }
            int x_arena = x_idx + curr_x;
            int y_arena = y_idx + curr_y;
            if (x_arena >= 0 && x_arena < ARENA_WIDTH
                && y_arena >= 0 && y_arena < ARENA_HEIGHT) {
                arena[x_arena][y_arena] = 1;
            }
        }
    }
}

void check_lines() {
    int count = 0;
    for (int y = ARENA_HEIGHT - 1; y >=0; --y) {
        bool is_line = true;
        for (int x = 0; x < ARENA_WIDTH; ++x) {
            if (arena[x][y] == 0) {
                is_line = false;
                break;
            }
        }
        if (!is_line) {
            continue;
        }
        ++count;
        for (int yy = y; yy > 0; --yy) {
            for (int xx = 0; xx < ARENA_WIDTH; ++xx) {
                arena[xx][yy] = arena[xx][yy-1];
            }
        }
        for (int xx = 0; xx < ARENA_WIDTH; ++xx) {
            arena[xx][0] = 0;
        }
        ++y;
    }
    if (count > 0) {
        score += count * 100;
    }
}

void draw_arena() {
    char buf[512];
    int  buf_idx = 0;
    for (int y_idx = 0; y_idx < ARENA_HEIGHT; ++y_idx) {
        buf[buf_idx++] = '|';
        for (int x_idx = 0; x_idx < ARENA_WIDTH; ++x_idx) {
            int idx = rotate(curr_rotation, x_idx - curr_x, y_idx - curr_y);
            if (arena[x_idx][y_idx] == 1
                || ((x_idx >= curr_x && x_idx < curr_x + TETROMINO_WIDTH)
                    && (y_idx >= curr_y && y_idx < curr_y + TETROMINO_HEIGHT)
                    && (tetrominoes[curr_index][idx] == 1)))
                buf[buf_idx++] = '#';
            else
                buf[buf_idx++] = ' ';
        }

        buf[buf_idx++] = '|';
        buf[buf_idx++] = '\r';
        buf[buf_idx++] = '\n';
    }
    buf[buf_idx] = '\0';

    printf("\e[2J");
    printf("\e[H");

    printf("%s\n\nScore: %d\n\n", buf, score);
}

int kbhit() {
    int ch = getch();
    if (ch != ERR) {
        ungetch(ch);
        return 1;
    } else {
        return 0;
    }
}

void handle_input() {
    while (kbhit()) {
        int key = getch();
        switch (key) {
        case 119: {
                int rotation = (curr_rotation + 1) % 4;
                if (check_position(curr_index, rotation, curr_x, curr_y)) {
                    curr_rotation = rotation;
                }
            }
            break;
        case 97: {
                if (check_position(curr_index, curr_rotation, curr_x - 1, curr_y)) {
                    --curr_x;
                }
            }
            break;
        case 100: {
                if (check_position(curr_index, curr_rotation, curr_x + 1, curr_y)) {
                    ++curr_x;
                }
            }
            break;
        case 115: {
                if (check_position(curr_index, curr_rotation, curr_x, curr_y + 1)) {
                    ++curr_y;
                }
            }
            break;
        }
    }
}

void run_game() {
    initscr();
    cbreak();
    noecho();
    nodelay(stdscr, TRUE);

    printf("\033[?25l");

    printf("\e[2J");
    printf("\e[H");

    memset(arena, 0, sizeof(arena[0][0]) * ARENA_WIDTH * ARENA_HEIGHT);
    generate_tetromino();

    const int frame_time = 10000;
    clock_t   last_time = clock();
    while (!is_game_over) {
        clock_t now = clock();
        clock_t elapsed = (now - last_time);
        handle_input();
        if (elapsed >= frame_time) {
            if (!move_down()) {
                place_to_arena();
                check_lines();
                generate_tetromino();
            }
            last_time = now;
        }
        draw_arena();
        usleep(10);
    }

    printf("\e[2J");
    printf("\e[H");

    endwin();

    printf("Game over!\nScore: %d\n", score);

    printf("\033[?25h");
}
