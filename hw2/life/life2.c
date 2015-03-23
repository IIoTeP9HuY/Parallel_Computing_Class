#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define ALIVE 'X'
#define DEAD '.'

int toindex(int row, int col, int N) {
    if (row < 0) {
        row = row + N;
    } else if (row >= N) {
        row = row - N;
    }
    if (col < 0) {
        col = col + N;
    } else if (col >= N) {
        col = col - N;
    }
    return row * N + col;
}

void printgrid(char* grid, char* buf, FILE* f, int N) {
    for (int i = 0; i < N; ++i) {
        strncpy(buf, grid + i * N, N);
        buf[N] = 0;
        fprintf(f, "%s\n", buf);
    }
}

int main(int argc, char* argv[]) {
    if (argc != 5) {
        fprintf(stderr, "Usage: %s N input_file iterations output_file\n", argv[0]);
        return 1;
    }

    int N = atoi(argv[1]); // grid size
    int iterations = atoi(argv[3]);

    FILE* input = fopen(argv[2], "r");
    char* grid = (char*) malloc(N * N * sizeof(char));
    for (int i = 0; i < N; ++i) {
        fscanf(input, "%s", grid + i * N);
    }
    fclose(input);

    char* buf = (char*) malloc(N * N * sizeof(char));
    char* front = (char*) malloc(N * N * sizeof(char));
    memset(front, 1, N * N);

    for (int iter = 0; iter < iterations; ++iter) {
        int next_front = (iter + 1) % 2 + 1;
        for (int i = 0; i < N; ++i) {
            for (int j = 0; j < N; ++j) {
                int current = i * N + j;
                if (front[current] != 0) {
                    int alive_count = 0;
                    for (int di = -1; di <= 1; ++di) {
                        for (int dj = -1; dj <= 1; ++dj) {
                            if ((di != 0 || dj != 0) && grid[toindex(i + di, j + dj, N)] == ALIVE) {
                                ++alive_count;
                            }
                        }
                    }
                    if (alive_count == 3 || (alive_count == 2 && grid[current] == ALIVE)) {
                        buf[current] = ALIVE;
                    } else {
                        buf[current] = DEAD;
                    }
                    if (buf[current] != grid[current]) {
                        for (int di = -1; di <= 1; ++di) {
                            for (int dj = -1; dj <= 1; ++dj) {
                                front[toindex(i + di, j + dj, N)] = next_front;
                            }
                        }
                    } else {
                        if (front[current] != next_front) {
                            front[current] = 0;
                        }
                    }
                }
            }
        }     
        char* tmp = grid; grid = buf; buf = tmp;
    }  

    FILE* output = fopen(argv[4], "w");
    printgrid(grid, buf, output, N);
    fclose(output);

    free(grid);
    free(buf);
    free(front);

    return 0;
}