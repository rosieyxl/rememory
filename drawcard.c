#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240
#define CARD_WIDTH 50
#define CARD_HEIGHT 70
#define CARD_GAP 10
#define BACKGROUND_COLOR 0xFFFFFF
#define BACK 0
#define MATCHED_CARD 5

int pixel_buffer_start;  // Global variable
volatile int *switches_ptr = (int *)0xFF200040;
volatile int *leds_ptr = (int *)0xFF200000;  

void clear_screen(short int color);
void draw_card(int position, int type);
void plot_pixel(int x, int y, short int color);

void main(void) {
  volatile int *pixel_ctrl_ptr = (int *)0xFF203020;
  pixel_buffer_start = *pixel_ctrl_ptr;

  // Clear the screen with white color
  clear_screen(BACKGROUND_COLOR);
  
  // RANDOMIZATION OF CARDS
  srand((unsigned int)time(NULL));             // seed with time
  int graphics[8] = {1, 2, 3, 4, 1, 2, 3, 4};  // the possible graphics
  int cards[8];  // the array of cards; will hold their corresponding graphic
  int j;
  int k=1;
  int first_card, second_card;
  int first_pos, second_pos;
  int match_counter = 0;

  for (int i = 0; i < 8; i++) {
    // assign a graphic to each card
    j = rand() % (8 - i);
    cards[i] = graphics[j];
    // remove the value we just used & shift the rest of the items down
    if (j != 7) {
      for (int k = j; k < (8 - i); k++) {
        graphics[k] = graphics[k + 1];
      }
    }
  }

  // INITIAL SET UP OF THE CARDS
  // Top left
  draw_card(1, BACK);
  // Top right
  draw_card(2, BACK);
  draw_card(3, BACK);
  draw_card(4, BACK);
  // Bottom left
  draw_card(5, BACK);
  draw_card(6, BACK);
  draw_card(7, BACK);
  draw_card(8, BACK);
  // SET UP DONE

  while (1) {
    // GAME LOGIC
    // turn on interrupts for switch

    // take in value of switch
    int switchValue = *switches_ptr;
    // 'flip' the card chosen (draw its front side)
    draw_card(switchValue, cards[switchValue-1]);
    
    // store value of chosen card
    if (k == 1) {
        first_card = cards[switchValue-1];
        first_pos = switchValue;
        k++;
    } else if (k == 2) {
        second_card = cards[switchValue-1];
        second_pos = switchValue;
    }

    // compare the two selected cards;
    if (k == 2) {
        if (first_card == second_card) {
            // match!!
            // show on LEDs & print a green dot onto the card
            *leds_ptr = 1;
            draw_card(first_pos, MATCHED_CARD);
            draw_card(second_pos, MATCHED_CARD);
            match_counter++;
        } else {
            // not a match loser
            // flip cards back over
            draw_card(first_pos, BACK);
            draw_card(second_pos, BACK);
        }
        k = 1;
    }

    if (match_counter == 4) {
        // all matches made!
        // PRINT WINNING SCREEN (UGLY FUGLY GREEN)
        clear_screen(0x00FF00);
        break;
    }

  }
}

void clear_screen(short int color) {
  for (int y = 0; y < SCREEN_HEIGHT; y++) {
    for (int x = 0; x < SCREEN_WIDTH; x++) {
      plot_pixel(x, y, color);
    }
  }
}

void draw_card(int position, int type) {
  int x=30, y=60;
  const int card_width = CARD_WIDTH;
  const int card_height = CARD_HEIGHT;

  switch (position) {
    case 1:
      x = 30;
      y = 60;
      break;
    case 2:
      x = 30 + CARD_WIDTH + 20;
      y = 60;
      break;
    case 3:
      x = 100 + CARD_WIDTH + 20;
      y = 60;
      break;
    case 4:
      x = 170 + CARD_WIDTH + 20;
      y = 60;
      break;
    case 5:
      x = 30;
      y = SCREEN_HEIGHT - CARD_HEIGHT - 20;
      break;
    case 6:
      x = 30 + CARD_WIDTH + 20;
      y = SCREEN_HEIGHT - CARD_HEIGHT - 20;
      break;
    case 7:
      x = 100 + CARD_WIDTH + 20;
      y = SCREEN_HEIGHT - CARD_HEIGHT - 20;
      break;
    case 8:
      x = 170 + CARD_WIDTH + 20;
      y = SCREEN_HEIGHT - CARD_HEIGHT - 20;
      break;
  }

  switch (type) {
    case BACK:
      for (int i = 0; i < card_height; i++) {
        for (int j = 0; j < card_width; j++) {
          plot_pixel(x + j, y + i,
                     0xFFD1DC);  // solid purple colour for back of cards
        }
      }
      break;

    case 1:
      for (int i = 0; i < card_height; i++) {
        for (int j = 0; j < card_width; j++) {
          plot_pixel(x + j, y + i, 0x0000FF);  // blue for front side of cards
        }
      }
      plot_pixel(x, y, 0xFFFFFF);  // 1 white dot on left corner
      break;

    case 2:
      for (int i = 0; i < card_height; i++) {
        for (int j = 0; j < card_width; j++) {
          plot_pixel(x + j, y + i, 0x0000FF);  // blue for front side of cards
        }
      }
      plot_pixel(x, y, 0xFFFFFF);
      plot_pixel(x + 2, y, 0xFFFFFF);  // 2 white dots on left corner
      break;

    case 3:
      for (int i = 0; i < card_height; i++) {
        for (int j = 0; j < card_width; j++) {
          plot_pixel(x + j, y + i, 0x0000FF);  // blue for front side of cards
        }
      }
      plot_pixel(x, y, 0xFFFFFF);
      plot_pixel(x + 2, y, 0xFFFFFF);
      plot_pixel(x + 4, y, 0xFFFFFF);  // 3 white dots on left corner
      break;

    case 4:
      for (int i = 0; i < card_height; i++) {
        for (int j = 0; j < card_width; j++) {
          plot_pixel(x + j, y + i, 0x0000FF);  // blue for front side of cards
        }
      }
      plot_pixel(x, y, 0xFFFFFF);
      plot_pixel(x + 2, y, 0xFFFFFF);
      plot_pixel(x + 4, y, 0xFFFFFF);
      plot_pixel(x + 6, y, 0xFFFFFF);  // 4 white dots on left corner
      break;

    case MATCHED_CARD:
      plot_pixel(x + 25, y + 35, 0x00FF00);  // green dot in center
  }
}

void plot_pixel(int x, int y, short int color) {
  volatile short int *one_pixel_address;
  one_pixel_address = pixel_buffer_start + (y << 10) + (x << 1);
  *one_pixel_address = color;
}