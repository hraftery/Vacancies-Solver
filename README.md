# Vacancies-Solver

An exhaustive search algorithm for solutions to the Vacancies solitaire card game.

![In-game screenshot](In-game_screenshot.png?raw=true "Vacancies In-game Screenshot")

## Description

Vacancies is a variation on a solitaire card game more commonly known as [Gaps](https://en.wikipedia.org/wiki/Gaps). This particular variation is available for play on the in-flight entertainment system on Cathay Pacific flights in the [Solitrio](https://www.cathaypacific.com/cx/en_US/travel-information/flying-with-us/inflight-entertainment/games.html) category. The rule variations seem to be:

1. Face cards are not used. Only 2 to 10 are used, making each of the four rows only 10 cards long.
1. When cards are redealt after a round, no gap is left to the right of each suit sequence. Instead, the Aces are included in the redeal, and then they are removed to form gaps in random locations.

## Motivation

Like all good solitaire, the game is tantalisingly winnable, though the optimal strategy is hard to grasp. After playing many games with an average success rate of about 1 in 6, I became intrigued about the actual complexity of the game and whether there was a winning strategy. Not having access to the Internet on an international flight to draw upon the world's knowledge, I instead turned to DIY computer science.

## Solution

The result is a brute force solver that plays every possible game by walking the tree of possible moves. Each time it reaches a impasse, it calculates how many correctly placed cards there are (called `score`). If the score is better than previously achieved, it prints out the new best score, the moves that got there, and the card layout at that point. It then continues searching for a better score.

It turns out the game is stunningly complex. For most initial card layouts there are more than a billion ways to play the game, despite there always being less than 40 moves per round. Efficiently searching the solution space calls for significant optimisation. Replacing loops with lookup tables and other optimisations has increased the search speed to over 100 million turns per second. This still wasn't even scratching the surface of the solution space when running 100% on a single modern processor core, so parallel processing has been introduced. Now an individual process spawns to search each of the solution sub-spaces defined by each of the possible first moves. Now, even running 100% on each of the four cores in a 2.7GHz Intel Core i5, most games cannot be fully searched even after 24 hours.

## Results

My plan was to run several exhaustive searches on random initial layouts to get a feel for how often a winning play could be found. Given that it's not feasible to run even one exhaustive search, I've instead run a number of truncated searches, and analysed the evolution of best scores that occurred.

The results show:

1. Most of the best solutions are found within a second.
1. Sometimes a better solution is found after the first second, but it may take 30 seconds or it may take 40 minutes.
1. Running overnight revealed no better solutions to those found in the first hour, though I've only done this once.
1. An optimal solution was found in 1 out of 7 games that were allowed to run for at least a minute. That solution was found in the first second of the search.
1. There remains doubt about whether the inability to find optimal solutions is due to being stuck searching low-value avenues or whether the optimal solution does not exist.

Raw results follow. Time is in seconds. Perfect score is 36.

| Time | Score | Time | Score | Time | Score | Time | Score | Time | Score | Time | Score | Time | Score |
|------|-------|------|-------|------|-------|------|-------|------|-------|------|-------|------|-------|
|  0   |   9   |  0   |   4   |  0   |   5   |  0   |   2   |  0   |   6   |  0   |   5   |  0   |   2   |
|  0   |   11  |  0   |   15  |  0   |   9   |  0   |   4   |  0   |   20  |  0   |   9   |  0   |   5   |
|  0   |   13  |  0   |   19  |  0   |   15  |  0   |   6   |  0   |   36  |  0   |   12  |  0   |   10  |
|  27  |   25  |      |       |  0   |   16  |  0   |   20  |      |       |  0   |   14  |  0   |   12  |
|      |       |      |       |  0   |   17  |  0   |   29  |      |       |      |       |  0   |   16  |
|      |       |      |       |      |       |      |       |      |       |      |       |  1   |   17  |
|      |       |      |       |      |       |      |       |      |       |      |       | 2484 |   20  |
|      |       |      |       |      |       |      |       |      |       |      |       | 2662 |   21  |

## TODO

Plenty.

1. Allow specification of initial layout, rather than creating a random layout every time. This would allow solving of an actual in-game layout.
1. Allow "chances" to be taken - that is, allow the round to finish, unplaced cards to be redealt, and play to begin again. Not done because it's rare that a round finishes by itself due to exhaustive search.
1. Pruning of solution space to find more optimal solutions rather than getting stuck down some low value branch.
