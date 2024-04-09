# rememory

Rememory is a memory-based card matching game created using C and Assembly on the Nios II Processor.

The objective is simple: within a limited amount of time, the player must match four pairs of cards. Key elements of the game implementation include: VGA display, PS2 keyboard and timer interrupts in C, as well as game logic.

To begin, initiate the game by pressing the spacebar. This will bring you to the memorization stage: where the set of cards is revealed for a five-second interval, during which you must commit the card graphics to memory (to the best of your ability). 

Then, the game will automatically move to the matching phase, which lasts 15 seconds. Here, the cards are flipped face down, and you must select ones you believe have matching graphics. To do so, press the corresponding keys (A, S, D, F for the top row; Z, X, C, V for the bottom). You may only flip two cards at a time: if they match, a check will appear; if they don't, the cards will flip back down.

There are two ways for the game to end: either you make all the correct matches within the allotted 15 seconds (a WIN), or the time runs out (a LOSS).

Time is updated on the HEX displays.
