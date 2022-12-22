# hangman-rc

Computer Networks project, Y3S1 (P2) 2022/23 LEIC-A IST.

Run `make` to compile both the player and GS executables.

## How to run the game server:

```
./GS word_file [-p GSport] [-v] [-r]
```

-   `word_file` - the name of the file containing a set of words that the GS can select from when a new game is started. Each line of the file should contain a word and the name of a file inside the `hints` directory which relates to the class the word belongs to, separated by a single space. An example file is already provided in the root directory.
-   `GSport` - the port number where the game server is going to accept requests (UDP & TCP). Default value is 58043.
-   If the `-v` option is set, the program operates in verbose mode, outputting to `stderr` a log of all traffic (requests and responses, UDP and TCP), along with the player's IP address and port number.
-   If the `-r` option is set, the game server selects a word from the word file at random, rather than sequentially (default).

## How to run the player:

```
./player [-n GSIP] [-p GSport]
```

-   `GSIP` - the IP address of the machine where the game server is running. Default value is localhost.
-   `GSport` - the port where the game server is accepting requests (UDP & TCP). Default value is 58043.

## How to change timeout values

Both the player and the game server have a timeout value for communicating with each other. You can change this value, for each executable, by changing the `TIMEOUT_SECS` and `TIMEOUT_MICROSECS` macros in `src/player/network.h` and `src/GS/network.h`, respectively. You then need to `make clean` and `make` again.

A value of 0 means that requests will never timeout.

## Available Make Commands

-   `make` - compiles both the player and GS executables.
-   `make player` - compiles the player executable.
-   `make GS` - compiles the GS executable.
-   `make clean` - removes all object files and executables.
-   `make clean-artifacts` - removes all game state files and potential files received by the player via TCP (using an assumption of common file extensions).
-   `make fmt` - formats all source files.
-   `make fmt-check` - checks if all source files are formatted correctly.
-   `make release` - creates a `proj_043.zip` file to be submitted.
