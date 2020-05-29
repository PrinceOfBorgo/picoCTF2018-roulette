# picoCTF2018 - roulette
## Text
> This Online [Roulette](https://github.com/PrinceOfBorgo/picoCTF2018-roulette/blob/master/roulette) Service is in Beta. Can you find a way to win $1,000,000,000 and get the flag? [Source](https://github.com/PrinceOfBorgo/picoCTF2018-roulette/blob/master/roulette.c). Connect with `nc 2018shell.picoctf.com 25443`

Port may be different.


## Hints
> There are 2 bugs!


## Problem description
Connecting to server we can see a welcome page of a roulette service. We are assigned a random initial balance to start betting. We are asked to make our bet and to choose a number. That's enough, let's see the provided [source code](https://github.com/PrinceOfBorgo/picoCTF2018-roulette/blob/master/roulette.c).

First let's see what we have to do in order to get the flag. As seen in the request we must gain one billion but, actually, it isn't enough... In `main()` we can see that two conditions must be verified to call `print_flag()` function: `cash > ONE_BILLION` and `wins >= HOTSTREAK`. The first condition sounds familiar but the second one tells us to win at least three times (`HOTSTREAK = 3`).

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
Initially `c` is not a digit (it is set to `0` that is the null character) so the function enters the first `while` and, since `stdin` is line buffered, `get_char()` will wait for us to press `enter`. At this point the program will start scanning the buffer one character at a time doing nothing if it is a non-digit (first `while` loop). When a digit is encountered the first loop ends end we enter the second one that will run until we get another non-digit character: the second loop will end and we will enter the third one that keeps ignoring every character until a `\n` (new line character) is found that is the end of the input string. This means that the core of the function is in the second loop: an unsigned 64 bit integer `l` is constructed appending a digit at a time, that is if `l` is `123` and the next digit is `4`, then `l` will become `123*10 + 4` that is `1234`. Before doing this operations, it is checked if `l` is less than `LONG_MAX`, otherwise it will be set to `LONG_MAX` and the `while` will be terminated. `LONG_MAX` is the maximum value a `long` variable can assume and depends on the system: a `long` variable is guaranteed to be at least 32 bits wide but in some systems it can be 64 bit, in particular, the remote application uses 32 bit integers as `long` variables (see [**Solution**](#solution) section for details). We have to notice that `get_long()` returns a `long` variable (that is a signed 32 bit integer) while operations are executed on `l` that is an unsigned 64 bit integer. Moreover, the control `l >= LONG_MAX` is done before doing operations and not after, so we can have an `l` value that is less than `LONG_MAX` but after appending a digit it will become greater and, if the while is interrupted, `l` will remain greater than `LONG_MAX`. This fact can be used to pass properly built values for `l` that, returned as `long`, will become negative. This will allow us to assign (almost) arbitrary values to `bet` (even negative ones), bypassing `bet <= cash` condition in `get_bet()` function.

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
The two bugs the hint was referring to are these:
1. the seed used for pseudo-random integer generation is visible in the original value of `cash`;
2. `get_long()` function returns a signed integer while working on an unsigned one and constrains it to a maximum value only before doing operations.

I created an [application](https://github.com/PrinceOfBorgo/picoCTF2018-roulette/blob/master/spin_results) (click [here](https://github.com/PrinceOfBorgo/picoCTF2018-roulette/blob/master/spin_results.c) for source code) that takes as input the seed we want to use for generate random numbers and, pressing `enter`, prints the result of the roulette spins.

Now we can run the remote service, take note of the random generated seed (readable in our starting balance) and start `spin_results` with the same seed as argument. This way, pressing `enter`, we will get our first random `spin` result: using this as our `choice` in the remote service we will get our first win.  
Example:
<pre>
$ nc 2018shell.picoctf.com 25443
Welcome to ONLINE ROULETTE!
Here, have $<b>2634</b> to start on the house! You'll lose it all anyways >:)

How much will you wager?
Current Balance: $2634 	 Current Wins: 0
>
</pre>
The seed is `2634`, so we run `spin_results` using this value as argument and then we make our bet:
<pre>
$ ./spin_results <b>2634</b>
Seed: 2634

Press ENTER to get the next spin result:
Spin: <b>18</b>

Press ENTER to get the next spin result:
</pre>
We can see that the result of the first spin is `18`, this will be the same even in the remote application. Let's go all-in and bet on `18`:
<pre>
$ nc 2018shell.picoctf.com 25443
Welcome to ONLINE ROULETTE!
Here, have $2634 to start on the house! You'll lose it all anyways >:)

How much will you wager?
Current Balance: $2634 	 Current Wins: 0
> <b>2634</b>
Choose a number (1-36)
> <b>18</b>

Spinning the Roulette for a chance to win $5268!

Roulette  :  <b>18</b>

You.. win.. this round...

How much will you wager?
<b>Current Balance: $5268 	 Current Wins: 1</b>
>
</pre>
It is notable that playing roulette and lose will print two random messages while winning it prints only one. We can assume that we want always win in the remote application so it will always call one `rand()` function for the message instead of two. This way `spin_results` after printing the `spin` result it will call `rand()` just once.

Using the same approach we can easily obtain three wins to get one of the win conditions (`wins >= HOTSTREAK`):
<pre>
$ ./spin_results <b>2634</b>
Seed: 2634

Press ENTER to get the next spin result:
Spin: <b>18</b>

Press ENTER to get the next spin result:
Spin: <b>10</b>

Press ENTER to get the next spin result:
Spin: <b>28</b>

Press ENTER to get the next spin result:
</pre>

<pre>
$ nc 2018shell.picoctf.com 25443
Welcome to ONLINE ROULETTE!
Here, have $<b>2634</b> to start on the house! You'll lose it all anyways >:)

How much will you wager?
Current Balance: $2634 	 Current Wins: 0
> 2634
Choose a number (1-36)
> <b>18</b>

Spinning the Roulette for a chance to win $5268!

Roulette  :  18

You.. win.. this round...

How much will you wager?
Current Balance: $5268 	 Current Wins: 1
> 5268
Choose a number (1-36)
> <b>10</b>

Spinning the Roulette for a chance to win $10536!

Roulette  :  10

Darn.. Here you go

How much will you wager?
Current Balance: $10536 	 Current Wins: 2
> 10536
Choose a number (1-36)
> <b>28</b>

Spinning the Roulette for a chance to win $21072!

Roulette  :  28

Winner!

How much will you wager?
<b>Current Balance: $21072 	 Current Wins: 3</b>
>
</pre>

One could think to use this method even to reach one billion but there is a problem: roulette application sets a maximum number of wins to `16`, after which we are kicked out. Since our initial balance is at most `4999` and we can at most double our current balance at each bet (going all-in), we got a maximum win of `4999 * 2^16 = 327614464` that is way less than one billion.
