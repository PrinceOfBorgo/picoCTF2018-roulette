# picoCTF2018 - roulette
## Text
> This Online [Roulette](https://github.com/PrinceOfBorgo/picoCTF2018-roulette/blob/master/roulette) Service is in Beta. Can you find a way to win $1,000,000,000 and get the flag? [Source](https://github.com/PrinceOfBorgo/picoCTF2018-roulette/blob/master/roulette.c). Connect with `nc 2018shell.picoctf.com 25443`

Port may be different.

## Hints
> There are 2 bugs!

## Problem description
Connecting to server we can see a welcome page of a roulette service. We are assigned a random initial balance to start betting. We are asked to make our bet and to choose a number. That's enough, let's see the provided [source code](https://github.com/PrinceOfBorgo/picoCTF2018-roulette/blob/master/roulette.c).

First let's look what we have to do in order to get the flag. As seen in the request we must gain one billion but, actually, it isn't enough... In `main()` we can see that two conditions must be verified to call `print_flag()` function: `cash > ONE_BILLION` and `wins >= HOTSTREAK`. The first condition sounds familiar but the second tells us to win at least three times (`HOTSTREAK = 3`).

Our current balance is stored in `cash` variable that is initialized in `main()` calling `get_rand()` function:
```c
long get_rand() {
    long seed;
    FILE *f = fopen("/dev/urandom", "r");
    fread(&seed, sizeof(seed), 1, f);
    fclose(f);
    seed = seed % 5000;
    if (seed < 0) seed = seed * -1;
    srand(seed);
    return seed;
}
```
`seed` variable is set to a random value obtained through the special file [`/dev/urandom`](https://en.wikipedia.org/wiki//dev/random) then it is restricted to range `[0, 4999]` taking the remainder of division by 5000 and making the result positive. `srand(seed)` sets `seed` as the seed for a new sequence of pseudo-random integers to be returned by `rand()`. Finally the procedure returns `seed`.

This tell us that `cash = seed` so the balance we see running the remote service is, in fact, the seed used to initialize the sequence of results we will get calling `rand()` function. Knowing the seed we will try to predict what number will result after spinning the roulette (see [**Solution**](#solution) section).

After setting `cash` and printing it to screen with the welcome message, the program enters in a `while` loop where we are asked to make our `bet` and to choose the number (`choice`) we hope to see after spinning the roulette.

`bet` and `choice` variables are set calling `get_bet()` and `get_choice()` respectively. Both functions work calling `get_long()` and doing a control on the result: `get_bet()` accepts bets that are less or equal to `cash` while `get_choice()` accepts choices in range `[1, 36]`. Let's look to `get_long()` function:
```c
long get_long() {
    printf("> ");
    uint64_t l = 0;
    char c = 0;
    while(!is_digit(c))
        c = getchar();
    while(is_digit(c)) {
        if(l >= LONG_MAX) {
            l = LONG_MAX;
            break;
        }
        l *= 10;
        l += c - '0';
        c = getchar();
    }
    while(c != '\n')
        c = getchar();
    return l;
}
```
Initially `c` is not a digit (it is set to `0` that is the null character) so the function enters the first `while` and, since `stdin` is line buffered, `get_char()` will wait for us to press `enter`. At this point the program will start scanning the buffer one character at a time doing nothing if it is a non-digit (first `while` loop). When a digit is encountered the first loop ends end we enter the second one that will run until we get another non-digit character: the second loop will end and we will enter the third one that keeps ignoring every character until a `\n` (new line character) is found that is the end of the input string. This means that the core of the function is in the second loop: an unsigned 64 bit integer `l` is constructed appending a digit at a time, that is if `l` is `123` and the next digit is `4`, then `l` will become `123*10 + 4` that is `1234`. Before doing this operations, it is checked if `l` is less than `LONG_MAX`, otherwise it will be set to `LONG_MAX` and the `while` will be terminated. `LONG_MAX` is the maximum value a `long` variable can assume and depends on the system: a `long` variable is guaranteed to be at least 32 bits wide but in some systems it can be 64 bit, in particular, the remote application uses 32 bit integers as `long` variables (see [**Solution**](#solution) section for details). We have to notice that `get_long()` returns a `long` variable (that is a signed 32 bit integer) while operations are executed on `l` that is an unsigned 64 bit integer. This fact can be used to pass properly built values for `l` variable that, returned as `long`, will become negative. This will be necessary to use almost arbitrary values bypassing `bet <= cash` condition in `get_bet()` function.

Returning to `main()`, after setting `bet` (and subtracting it from `cash`) and `choice`, `play_roulette(choice, bet)` is called:
```c
void play_roulette(long choice, long bet) {

    printf("Spinning the Roulette for a chance to win $%lu!\n", 2*bet);
    long spin = (rand() % ROULETTE_SIZE)+1;

    spin_roulette(spin);

    if (spin == choice) {
        cash += 2*bet;
        puts(win_msgs[rand()%NUM_WIN_MSGS]);
        wins += 1;
    }
    else {
        puts(lose_msgs1[rand()%NUM_LOSE_MSGS]);
        puts(lose_msgs2[rand()%NUM_LOSE_MSGS]);
    }
    puts("");
}
```
Variable `spin` is randomly set to a value in `[1, 32]` (calling `rand()`) then a useless animation is performed in `spin_roulette(spin)` and if `spin` is equal to our `choice`, we win, a random message is print and `wins` variable is incremented by one. Otherwise two random message are printed to screen.

## Solution
