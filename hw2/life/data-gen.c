#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define ALIVE 'X'
#define DEAD '.'

int main(int argc, char* argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s N output_file\n", argv[0]);
        return 1;
    }

    int N = atoi(argv[1]); // grid size
    FILE* output = fopen(argv[2], "w");

    srand(time(NULL));
    int i, j;
    for (i = 0; i < N; i++) {
        for (j = 0; j < N; j++) {
            fprintf(output, "%c", rand()%3 > 0 ? DEAD : ALIVE);
        }
        fprintf(output, "\n");
    }
    fclose(output);

    return 0;
}