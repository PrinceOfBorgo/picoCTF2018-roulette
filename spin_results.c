#include <stdio.h>
#include <stdlib.h>

#define ROULETTE_SIZE 36

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <seed>\n", argv[0]);
    } else {    // Pass seed as first argument
        setvbuf(stdout, NULL, _IONBF, 0);

        long seed;
        sscanf(argv[1], "%ld", &seed);
        srand(seed);    // Set seed for future rand() calls

        printf("Seed: %ld\n", seed);

        while(1) {
            printf("\nPress ENTER to get the next spin result:");
            while('\n' != getchar());

            long spin = (rand() % ROULETTE_SIZE)+1;
            printf("Spin: %ld\n", spin);
            // Call a rand() because we expect to win in the remote application
            // Winning will call puts(win_msgs[rand()%NUM_WIN_MSGS]);
            // This line contains one rand() call.
            rand();
        }
    }
    return 0;
}
