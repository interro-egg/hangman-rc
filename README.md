# hangman-rc

Computer Networks project, Y3S1 (P2) 2022/23 LEIC-A IST.

Run `make` to compile both the player and GS executables.

## How to run server:

```
./GS word_file [-p GSport] [-v]
```

- `word_file` - the name of the file containing a set of words that the GS can select from when a new game is started. Each line of the file should contain a word and a class to which the word belongs, separated by a single space. There's an example already in the root directory.
- `GSport` - the port where the game server is going to accept requests (UDP & TCP). Default value is 58043.
- If the `-v` option is set, the program operates in verbose mode, outputing a short description of the requests received and the IP and port originating those requests.

## How to run player:

```
./player [-n GSIP] [-p GSport]
```

- `GSIP` - the IP address of the machine where the game server is running. Default value is localhost.
- `GSport` - the port where the game server is accepting requests (UDP & TCP). Default value is 58043.
