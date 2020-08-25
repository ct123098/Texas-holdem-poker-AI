# Texas Hold’em AI

This project is a Texas Hold'em AI and won the top three in the Game Theory course in 2020 Spring. The AI uses a heuristic idea. More specifically, the algorithm calculates the winning rate of the current visible cards, and decides an action based on the estimated winning rate.

The AI has the feature of a fast speed and a short implementation. It only takes about 5s for 1000 hands (i.e. 5ms/hand).

The algorithm is implemented in C++ and obeys the Annual Computer Poker Competition(ACPC)  heads-up (two-player) no-limit Texas hold'em competition protocol. (reference: http://www.computerpokercompetition.org/downloads/documents/protocols/protocol.pdf)

Code
----

The algorithm is implemented in `gambler.cpp`, and the parameter fine-tuning is implemented in `gambler-v3.0-training.cpp`.  `utils` is the directory containing some basic functions which my implementation relies on. And `bin` is the directory where the execute file generates.

## How to Use

1. You need to Compile the code
	```bash
	make clean && make
	```

2. You only need to run the executable file `bin/gambler` , which is implemented following the required protocol:

    ```bash
    ./bin/gambler {IP Adress} {Port}
    ```

    For example,

    ```bash
    ./bin/gambler 127.0.0.1 58669
    ```

Note: please make sure that file `holdem.nolimit.2p.reverse_blinds.game` is under your working directory of the terminal.
