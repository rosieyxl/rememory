#include <stdio.h>
#include <time.h>
#include <stdbool.h>

int main(void) {
    srand(time(0)); // seed with time
    int graphics[8] = {1,2,3,4,1,2,3,4}; // the possible graphics
    int cards[8] = {}; // the array of cards; will hold their corresponding graphic
    int j;

    for (int i = 0; i < 8; i++){
        // assign a graphic to each card
        j = rand()%(8-i);
        printf((char)j);
        cards[i] = graphics[j];
        // remove the value we just used & shift the rest of the items down
        if (j != 7) {
            for (int k = j; k < (8-i); k++) {
                graphics[k] = graphics[k+1];
            }
        }
    }

    // debugging
    printf("This is the final array: \n");
    for (int i = 0; i <8; i++){
        printf((char)cards[i]);
    }
}