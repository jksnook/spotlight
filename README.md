# Spotlight
A partial rewrite and cleanup of my C++ chess engine. Future name TBD.

## Features:

### Move generation
* Fully legal move generation with magic bitboards
* around 45 million nps in perft on my (mediocre) system without bulk counting

### Search:
* Transposition Table
* Iterative Deepening
* Principal variation search
* Aspiration Windows
* Null move pruning
* Futility pruning
* Reverse futility pruning
* Internal iterative reductions
* Late move reductions
* Late move pruning
* SEE pruning in quiescence search

### Move ordering
* TTmove
* captures ordered by SEE
* Killer moves
* Butterfly history heuristic

### Evaluation
* Tuned piece square tables (Tuning implementation from Andrew Grant's tuning paper: https://github.com/AndyGrant/Ethereal/blob/master/Tuning.pdf)
* Tuned entirely with self-generated data

## Planned Improvements:
* Better eval (eventually)
* Refactor movegen and move ordering