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
`seed` variable is set to a random value obtained through the special file [`/dev/urandom`](https://linux.die.net/man/4/urandom) then it is restricted to range `[0, 4999]` taking the remainder of division by 5000 and making the result positive. `srand(seed)` sets `seed` as the seed for a new sequence of pseudo-random integers to be returned by `rand()`. Finally the procedure returns `seed`.

This tell us that `cash = seed` so the balance we see running the remote service is, in fact, the seed used to initialize the sequence of results we will get calling `rand()` function. Knowing the seed we will try to predict what number will result after spinning the roulette (see [**Hard work**](#hard-work) section).

After setting `cash` and printing it to screen with the welcome message, the program enters in a `while` loop where we are asked to make our `bet` and to choose the number (`choice`) we hope to see after spinning the roulette.

`bet` and `choice` variables are set calling `get_bet()` and `get_choice()` respectively. Both functions work calling `get_long()` and doing a control on the result: `get_bet()` accepts bets that are less or equal to `cash` while `get_choice()` accepts choices in range `[1, 36]`. Let's look at `get_long()` function:
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
Initially `c` is not a digit (it is set to `0` that is the null character) so the function enters the first `while` and, since `stdin` is line buffered, `get_char()` will wait for us to press ENTER. At this point the program will start scanning the buffer one character at a time doing nothing if it is a non-digit (first `while` loop). When a digit is encountered the first loop ends end we enter the second one that will run until we get another non-digit character: the second loop will end and we will enter the third one that keeps ignoring every character until a `\n` (new line character) is found that is the end of the input string. This means that the core of the function is in the second loop: an unsigned 64 bit integer `l` is constructed appending a digit at a time, that is if `l` is `123` and the next digit is `4`, then `l` will become `123*10 + 4` that is `1234`. Before doing these operations, it is checked if `l` is less than `LONG_MAX`, otherwise it will be set to `LONG_MAX` and the `while` will be terminated. `LONG_MAX` is the maximum value a `long` variable can assume and depends on the system: a `long` variable is guaranteed to be at least 32 bits wide but in some systems it can be 64 bit, in particular, the remote application uses 32 bit integers as `long` variables (see [**Hard work**](#hard-work) section for details). We have to notice that `get_long()` returns a `long` variable (that is a signed 32 bit integer) while operations are executed on `l` that is an unsigned 64 bit integer. Moreover, the control `l >= LONG_MAX` is done before doing operations and not after, so we can have an `l` value that is less than `LONG_MAX` but after appending a digit it will become greater and, if the loop is interrupted, `l` will remain greater than `LONG_MAX`. This fact can be used to pass properly built values for `l` that, returned as `long`, will become negative. This will allow us to assign (almost) arbitrary values to `bet` (even negative ones), bypassing `bet <= cash` condition in `get_bet()` function.

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
Variable `spin` is randomly set to a value in `[1, 32]` (calling `rand()`) then a useless animation is performed in `spin_roulette(spin)` and if `spin` is equal to our `choice`, we win, a random message is printed and `wins` variable is incremented by one. Otherwise two random message are printed to screen.


## Hard work
The two bugs the hint was referring to are these:
1. the seed used for pseudo-random integer generation is visible in the original value of `cash`;
2. `get_long()` function returns a signed integer while working on an unsigned one and constrains it to a maximum value only before doing operations.

I created an [application](https://github.com/PrinceOfBorgo/picoCTF2018-roulette/blob/master/spin_results) (click [here](https://github.com/PrinceOfBorgo/picoCTF2018-roulette/blob/master/spin_results.c) for source code) that takes as input the seed we want to use to generate random numbers and, pressing ENTER, prints the numbers obtained spinning the roulette.

Now we can run the remote service, take note of the random generated seed (readable in our starting balance) and run `spin_results` with that same seed as argument. This way, pressing ENTER, we will get our first random `spin` result: using this as our `choice` in the remote service we will get our first win.  
Let's see an example:
<pre>
$ nc 2018shell.picoctf.com 25443
Welcome to ONLINE ROULETTE!
Here, have $<b>2634</b> to start on the house! You'll lose it all anyways >:)

How much will you wager?
Current Balance: $2634 	 Current Wins: 0
>
</pre>
The seed is `2634`, so we run `spin_results` using this value as argument and then we press ENTER to get the first result:
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
Notice that if we play and lose, the remote application will print two random messages while, winning, only one message will be printed. We can assume that we want to win every time so `rand()` function will be called only once (instead of twice) to print the random message. Hence `spin_results` will call `rand()` (just once) after printing the `spin` result.

Using the same approach we can easily obtain three wins to satisfy one of the conditions to get the flag (`wins >= HOTSTREAK`):
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

To get to one billion we must exploit the `get_long()` function. We already know that `get_long()` stores input into an unsigned 64 bit integer and returns a `long` variable (that is signed). First of all we have to know how many bits a `long` variable is in the remote syste. To do this let's notice that we can pass a non-negative integer less than `LONG_MAX` and, appending another digit, we can bypass the condition `l >= LONG_MAX` (since `l` was the number before appending the last digit). Now we have a value greater than `LONG_MAX`, this means that as an `unsigned long` number, it will have the most significant bit set to `1` and, converting to `long`, this corresponds to a negative number. Hence, to prove that `long` is exactly a 32 bit integer, we can evaluate `LONG_MAX` as `2^31 - 1 = 2147483647 = 0x7fffffff`. Now we can pass as input the number `LONG_MAX + 1 = 2^31 = 2147483648 = 0x80000000`, press ENTER and see what happens:
1. digits are read from left to right to build `l`;
2. when we get to last but one digit we obtain `l = 214748364 = 0x0ccccccc`;
3. the last character `8` is saved to `c` and results to be a digit;
4. `l` is less than `LONG_MAX` (`0x0ccccccc < 0x7fffffff`) so we skip the `if` block;
5. `l` becomes `214748364*10 + 8 = 2147483648` and another character is saved to `c`;
6. `c` is `\n` so it isn't a digit, the current `while` is interrupted and the next one fails so `l` is returned as a `long` variable.

Now we can see that if `long` variables are more than 32 bits long nothing will happen to the result since `2147483648 = 2^31` could be represented as a  positive signed integer of at least 33 bits (the most significant bit will `0` so it will represent a positive number), instead the program will print this output:
<pre>
...
Current Balance: $3891   Current Wins: 0
> <b>2147483648</b>
Choose a number (1-36)
> 1

Spinning the Roulette for a chance to win $<b>0</b>!

Roulette  :  29

<b>Better luck next time...                                                             
Just give up!                                                                        
                                                                                     
Haha, lost all the money I gave you already? See ya later!</b>
</pre>

We can see a `0` where we should read twice the amount we bet `0x80000000`: this is because `2 * 0x80000000 = 0x100000000 = 2^32` is a 33 bit integer (in binary it is a `1` followed by 32 `0`'s) hence the most significant bit (`1`) will be cut off to make it fit 32 bits resulting on a 32 bits integer of only `0`'s (`(int32) 2^32 = 0x00000000 = 0`), this confirms that the remote application treats `long` variables are as 32 bits integers. Moreover we can see we have lost, this is because making the bet the program subtracts our `bet` from our current `cash` if `bet <= cash`. Since `bet` is a `long` variable (so it is signed) and it is equal to `0x80000000` (that has the sign bit set to `1`), `bet` is interpreted as `-2147483648` so the condition `bet <= cash` becomes `-2147483648 <= 3891` that is `true` and so `bet` can be subtracted from `cash`: `3891 - (-2147483648) = 2147487539 = 0x80000f33` that is negative if seen as a `long` variable (most signficant bit is `1`). Our new balance is negative and so we will lose.

Our target is to pass an input (unsigned) such that converted to `long` (signed) becomes negative (so the most significant bit must be `1`). This way `bet` will be always less than `cash`. We want `cash - bet` to be greater than one billion (`0x3b9aca00`) so that losing a bet will make us gain money.

Let's call:
* `x` the typed input without the last digit (that is the last value of `l` checked in `l >= LONG_MAX`);
* `X = x*10 + C` the final input with `C in [0, 9]` the unit digit (so `bet = (long) X`);
* `Y` the current value of `cash`.

Now it's time for some math:
1. Our first constraint is `x < LONG_MAX = 0x7fffffff = 2147483647`, otherwise, appending `C`, the condition `l >= LONG_MAX` will fail;
2. `cash - bet` must be greater than one billion:
    <pre>
    Y - X > 1000000000 = 0x3b9aca00
    --> -X > 0x3b9aca00 - Y
    --> X < Y - 0x3b9aca00 = Y + 0xc4653600 (<a href="https://en.wikipedia.org/wiki/Two%27s_complement">two's complement</a>)
    Since Y >= 1 --> X < 1 + 0xc4653600 --> X < 0xc4653601
    --> X <= 0xc4653600 = 3294967296
    </pre>
    Hence `X <= 3294967296`. This condition is stronger than 1;
3. We also want `cash - bet` to be positive, so it must be less than or equal to `LONG_MAX = 0x7fffffff`:
    <pre>
    Y - X <= 0x7fffffff
    --> -X <= 0x7fffffff - Y
    --> X >= Y - 0x7fffffff = Y + 0x80000001 (<a href="https://en.wikipedia.org/wiki/Two%27s_complement">two's complement</a>)
    We got at most 15 wins (at 16 we are kicked out) so:
    Y <= 4999*2 ^ 15 = 163807232 --> X >= 163807232 + 0x80000001 --> X >= 0x89c38001
    --> X >= 0x89c38001 = 2311290881
    </pre>
    Hence `X >= 2311290881`.

So our input must be in range `[2311290881, 3294967296]`.
This way we found an interval for our input that assures us that the `bet` will be interpreted as a negative number that subtracted from `cash` will make us gain one billion :D

N.B. theoretically we can have more than `4999*2 ^ 15 = 163807232` as our current balance, for example having `cash = 1` and `bet = 3294967297`, losing the bet we get to exactly one billion (that is not sufficient to win, we have to overcome it): the point of the previous calculations is to find a good range to use if the only losing bet is the last one, i.e the one that bring us to one billion.

## Solution
To get to the solution follow these steps:
1. Run the remote application and take note of the starting balance value then use it as first argument running `spin_results` application ([download it here](https://github.com/PrinceOfBorgo/picoCTF2018-roulette/blob/master/spin_results) it or compile the [source code](https://github.com/PrinceOfBorgo/picoCTF2018-roulette/blob/master/spin_results.c) with `gcc spin_results.c -o spin_results`);
2. Press ENTER in `spin_results` to know what number you have to choose in order to win in the remote application. Use it in the remote application with any amount not greater than your current balance (e.g. use `0`);
3. Repeat step 2 other two times to obtain three wins;
4. Set your bet to any amount in range `[2311290881, 3294967296]`;
5. You want to lose this bet: press ENTER in `spin_results` to know what number you DO NOT have to choose in the remote application.
6. Get your well deserved flag!

Example:
* Run remote service and take note of starting balance:
    <pre>
$ nc 2018shell.picoctf.com 25443
Welcome to ONLINE ROULETTE!
Here, have $<b>49</b> to start on the house! You'll lose it all anyways >:)

How much will you wager?
Current Balance: $49     Current Wins: 0
>
</pre>
* Run `spin_results` using starting balance as argument and press ENTER three times:
<pre>
$ ./spin_results <b>49</b>
Seed: 49

Press ENTER to get the next spin result:
Spin: <b>5</b>

Press ENTER to get the next spin result:
Spin: <b>20</b>

Press ENTER to get the next spin result:
Spin: <b>22</b>

Press ENTER to get the next spin result:
</pre>
* Get three wins on roulette betting using the three predictions above (in that order):
<pre>
$ nc 2018shell.picoctf.com 25443
Welcome to ONLINE ROULETTE!
Here, have $49 to start on the house! You'll lose it all anyways >:)

How much will you wager?
Current Balance: $49     Current Wins: 0
> 0       
Choose a number (1-36)
> <b>5</b>

Spinning the Roulette for a chance to win $0!

Roulette  :  5 

You're not cheating are you?

How much will you wager?
Current Balance: $49     Current Wins: 1
> 0
Choose a number (1-36)
> <b>20</b>

Spinning the Roulette for a chance to win $0!

Roulette  :  20

Darn, you got it right.

How much will you wager?
Current Balance: $49     Current Wins: 2
> 0
Choose a number (1-36)
> <b>22</b>

Spinning the Roulette for a chance to win $0!

Roulette  :  22

Wow.. Nice One!

How much will you wager?
<b>Current Balance: $49     Current Wins: 3</b>
>
</pre>
* Get another prediction that you DO NOT want to use:
<pre>
$ ./spin_results 49
Seed: 49

Press ENTER to get the next spin result:
Spin: 5

Press ENTER to get the next spin result:
Spin: 20

Press ENTER to get the next spin result:
Spin: 22

Press ENTER to get the next spin result:
Spin: <b>3</b>

Press ENTER to get the next spin result:
</pre>
* Bet any amount in `[2311290881, 3294967296]` on a number NOT equal to previous prediction:
<pre>
$ nc 2018shell.picoctf.com 25443
Welcome to ONLINE ROULETTE!
Here, have $49 to start on the house! You'll lose it all anyways >:)

How much will you wager?
Current Balance: $49     Current Wins: 0
> 0       
Choose a number (1-36)
> 5

Spinning the Roulette for a chance to win $0!

Roulette  :  5 

You're not cheating are you?

How much will you wager?
Current Balance: $49     Current Wins: 1
> 0
Choose a number (1-36)
> 20

Spinning the Roulette for a chance to win $0!

Roulette  :  20

Darn, you got it right.

How much will you wager?
Current Balance: $49     Current Wins: 2
> 0
Choose a number (1-36)
> 22

Spinning the Roulette for a chance to win $0!

Roulette  :  22

Wow.. Nice One!

How much will you wager?
Current Balance: $49     Current Wins: 3
> <b>2311290881</b>
Choose a number (1-36)
> <b>1</b>

Spinning the Roulette for a chance to win $327614466!

Roulette  :  3 

<b>Nice try..
If you keep it up, maybe you'll get the flag in 100000000000 years

*** Current Balance: $1983676464 ***
Wow, I can't believe you did it.. You deserve this flag!
picoCTF{1_h0p3_y0u_f0uNd_b0tH_bUg5_8b7aef91}</b>
</pre>
* Flag: `picoCTF{1_h0p3_y0u_f0uNd_b0tH_bUg5_8b7aef91}`.
