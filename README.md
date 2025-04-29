# Spotlight
A partial rewrite and cleanup of my C++ chess engine

## Features:

### Move generation
* Fully legal move generation with magic bitboards

### Search:
* Transposition Table
* Iterative Deepening
* Aspiration Windows
* Null move pruning
* Futility pruning
* Reverse futility pruning
* Late move reductions
* Late move pruning

### Move ordering
* TTmove
* captures ordered by SEE
* Killer moves
* History heuristic

### Evaluation
* Tuned piece square tables (Tuning implementation from Andrew Grant's tuning paper: https://github.com/AndyGrant/Ethereal/blob/master/Tuning.pdf)
* Tuned entirely with self-generated data

## Planned Improvements:
* Better eval (eventually)