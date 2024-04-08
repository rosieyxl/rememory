#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdint.h>
#include <unistd.h>
#include "graphics.h"

#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240
#define CARD_WIDTH 50
#define CARD_HEIGHT 70
#define CARD_GAP 10
#define BACK 0
#define MATCHED_CARD 5

int pixel_buffer_start;  // Global variable
volatile int *switches_ptr = (int *)0xFF200040;
volatile int *leds_ptr = (int *)0xFF200000;
volatile int HEX_BASE1 = 0xff200020;
volatile int HEX_BASE2 = 0xff200030;

#ifndef __NIOS2_CTRL_REG_MACROS__
#define __NIOS2_CTRL_REG_MACROS__
/*****************************************************************************/
/* Macros for accessing the control registers. */
/*****************************************************************************/
#define NIOS2_READ_STATUS(dest) \
  do {                          \
    dest = __builtin_rdctl(0);  \
  } while (0)
#define NIOS2_WRITE_STATUS(src) \
  do {                          \
    __builtin_wrctl(0, src);    \
  } while (0)
#define NIOS2_READ_ESTATUS(dest) \
  do {                           \
    dest = __builtin_rdctl(1);   \
  } while (0)
#define NIOS2_READ_BSTATUS(dest) \
  do {                           \
    dest = __builtin_rdctl(2);   \
  } while (0)
#define NIOS2_READ_IENABLE(dest) \
  do {                           \
    dest = __builtin_rdctl(3);   \
  } while (0)
#define NIOS2_WRITE_IENABLE(src) \
  do {                           \
    __builtin_wrctl(3, src);     \
  } while (0)
#define NIOS2_READ_IPENDING(dest) \
  do {                            \
    dest = __builtin_rdctl(4);    \
  } while (0)
#define NIOS2_READ_CPUID(dest) \
  do {                         \
    dest = __builtin_rdctl(5); \
  } while (0)
#endif

// for the initial 5 second countdown
volatile int countdown = 6;
volatile bool countdown_active = true;

int k = 1;
int first_card, second_card;
int first_pos, second_pos;
int graphics[8] = {1, 2, 3, 4, 1, 2, 3, 4};  // the possible graphics
int cards[8];  // the array of cards; will hold their corresponding graphic
int track = 0;

int j;
int match_counter = 0;
int counter = 100000000;  // 1/(100 MHz) × (5000000) = 50 msec
volatile bool keypressed = false;
volatile bool extrainterrupt = true;
volatile unsigned char key = 0;
volatile unsigned char byte1, byte2, byte3; 
int keytracker = 0;
volatile bool gameEnded = false;
bool spacePressed = false;
bool wonGame = false;

void interrupt_handler(void);
void interval_timer_ISR(void);
void pushbutton_ISR(void);
void PS2_ISR(void);

void clear_screen(short int color);
void draw_card(int position, int type);
void plot_pixel(int x, int y, short int color);
void display_hex(int value);
void LED_PS2(unsigned char letter);

int main(void) {
  volatile int *interval_timer_ptr = (int *)0xFF202000; // interval timer base address
  volatile int *PS2_ptr = (int *) 0xFF200100;

  volatile int *pixel_ctrl_ptr = (int *)0xFF203020;
  pixel_buffer_start = *pixel_ctrl_ptr;

  // draw starting screen
  for (int y = 0; y < SCREEN_HEIGHT; y++) {
    for (int x = 0; x < SCREEN_WIDTH; x++) {
      plot_pixel(x, y, startscreen[y][x]);
    }
  }

  // poll for the spacebar being pressed
  int  PS2_data, RVALID;
  char byte1 = 0, byte2 = 0, byte3 = 0;

  while (!spacePressed) {
    PS2_data = *(PS2_ptr);        // read the Data register in the PS/2 port
    RVALID   = PS2_data & 0x8000; // extract the RVALID field
    if (RVALID) {
        /* shift the next data byte into the display */
        byte1 = byte2;
        byte2 = byte3;
        byte3 = PS2_data & 0xFF;
    }
    if (byte3 == 0x29) {
      spacePressed = true;
    }
  }

  // draw background for memorization time
  for (int y = 0; y < SCREEN_HEIGHT; y++) {
    for (int x = 0; x < SCREEN_WIDTH; x++) {
      plot_pixel(x, y, memorize[y][x]);
    }
  }

  // RANDOMIZATION OF CARDS
  // srand((unsigned int)time(NULL));  // seed with time

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
  // Draw the cards face up for memorization
  for (int i = 0; i < 8; i++) {
    draw_card(i + 1, cards[i]);
  }

  NIOS2_WRITE_IENABLE(0x83); /* set interrupt mask bits for levels 0 (interval
                             * timer) and level 1 (pushbuttons) and levels 7 (PS2)*/
  NIOS2_WRITE_STATUS(1);    // enable Nios II interrupts

  // set the interval timer period for scrolling the HEX displays
  *(interval_timer_ptr + 0x2) = (counter & 0xFFFF);
  *(interval_timer_ptr + 0x3) = (counter >> 16) & 0xFFFF;

  // start interval timer, enable its interrupts
  *(interval_timer_ptr + 1) = 0x7;  // STOP = 0, START = 1, CONT = 1, ITO = 1

  while (countdown_active); // idle until 5 seconds is up

  // turn off timer interrupts
  NIOS2_WRITE_IENABLE(0b10000010);

  // Reset PS2 and enable PS2 interrupts
  *(PS2_ptr) = 0xFF; /* reset */
  *(PS2_ptr + 1) = 0x1; /* write to the PS/2 Control register to enable interrupts */

  int num_seconds = 10;
  while (!gameEnded) {
    // display num_seconds on hex
    display_hex(num_seconds);
    // count for a second
    int game_timer = 50000000;
    while (game_timer > 0) {
      game_timer--;
    }
    num_seconds--;
    if (num_seconds == 0) {
      gameEnded = true;
    }
  }; // idle until game ends (either time runs out or they win)

  if (!wonGame) {
    // draw losing screen :(
    for (int y = 0; y < SCREEN_HEIGHT; y++) {
      for (int x = 0; x < SCREEN_WIDTH; x++) {
        plot_pixel(x, y, lost[y][x]);
      }
    }
  }

  // disable all interrupts
  NIOS2_WRITE_IENABLE(0x0);

  return 0;
}

void clear_screen(short int color) {
  for (int y = 0; y < SCREEN_HEIGHT; y++) {
    for (int x = 0; x < SCREEN_WIDTH; x++) {
      plot_pixel(x, y, color);
    }
  }
}

void draw_card(int position, int type) {
  int x = 30, y = 60;
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
      for (int i = 0; i < 70; i++) {
        for (int j = 0; j < 50; j++) {
          plot_pixel(x + j, y + i, boba[i][j]);  // draw spade
        }
      }
      break;

    case 2:
      for (int i = 0; i < card_height; i++) {
        for (int j = 0; j < card_width; j++) {
          plot_pixel(x + j, y + i, donut[i][j]);  // blue for front side of cards
        }
      }
      plot_pixel(x, y, 0xFFFFFF);
      plot_pixel(x + 2, y, 0xFFFFFF);  // 2 white dots on left corner
      break;

    case 3:
      for (int i = 0; i < card_height; i++) {
        for (int j = 0; j < card_width; j++) {
          plot_pixel(x + j, y + i, headphones[i][j]);  // blue for front side of cards
        }
      }
      plot_pixel(x, y, 0xFFFFFF);
      plot_pixel(x + 2, y, 0xFFFFFF);
      plot_pixel(x + 4, y, 0xFFFFFF);  // 3 white dots on left corner
      break;

    case 4:
      for (int i = 0; i < card_height; i++) {
        for (int j = 0; j < card_width; j++) {
          plot_pixel(x + j, y + i, cat[i][j]);  // blue for front side of cards
        }
      }
      plot_pixel(x, y, 0xFFFFFF);
      plot_pixel(x + 2, y, 0xFFFFFF);
      plot_pixel(x + 4, y, 0xFFFFFF);
      plot_pixel(x + 6, y, 0xFFFFFF);  // 4 white dots on left corner
      break;

    case MATCHED_CARD:
      for (int i = 0; i < card_height; i++) {
        for (int j = 0; j < card_width; j++) {
          plot_pixel(x + j, y + i, matchedcard[i][j]);  // graphic for a matched card
        }
      }
      break;
  }
}

void plot_pixel(int x, int y, short int color) {
  volatile short int *one_pixel_address;
  one_pixel_address = pixel_buffer_start + (y << 10) + (x << 1);
  *one_pixel_address = color;
}

void interrupt_handler(void) {
  int ipending;
  NIOS2_READ_IPENDING(ipending);
  if (ipending & 0x1) {
    interval_timer_ISR();}
  if(ipending & 0x80) {
    PS2_ISR();
  }
  return;
}

void interval_timer_ISR(void) {
  // Clear the interrupt flag
  volatile int *interval_timer_ptr = (int *)0xFF202000;
  *interval_timer_ptr = 0;
  // Decrement countdown within the delay loop
  if (countdown_active && countdown > 0) {
    countdown--;
    display_hex(countdown);
  }

  // Flip cards down if we reach 0
  if (countdown_active && countdown == 0) {
    countdown = 6;
    countdown_active = false;
    *(interval_timer_ptr + 1) = 0x6;  // STOP = 0, START = 1, CONT = 1, ITO = 1
    // draw background for matching time
    for (int y = 0; y < SCREEN_HEIGHT; y++) {
      for (int x = 0; x < SCREEN_WIDTH; x++) {
        plot_pixel(x, y, makematches[y][x]);
      }
    }
    for (int i = 1; i <= 8; i++) {
      draw_card(i, BACK);
    }
  }
}

void PS2_ISR(void)
{ 
  volatile int *PS2_ptr = (int *) 0xFF200100;
  int PS2_data, RAVAIL;
  PS2_data = *(PS2_ptr); // read the data register in the PS/2 port
  RAVAIL = (PS2_data & 0xFFFF0000) >> 16; // extract the RAVAIL field
  int hold = 0;
  
  if (RAVAIL > 0) {
    byte1 = byte2;
	  byte2 = byte3;
    byte3 = (unsigned char)(PS2_data);
    } // extracts the pressed key

  keytracker = 0;
  
  LED_PS2(byte3); // draws the front of the card corresponding to pressed key

  if (!keypressed) {
    // the detected data was a BREAK code; we ignore it by breaking out of the ISR
    return;
  }

  // delay loop to SHOW the cards
  int counter = 20000000;
  while (counter > 0) {
    counter--; 
  }

  if (k == 3) {
    k = 1;
  } // checking if 2 cards have been flipped; then we reset to treat this card as a first flip

  if (keytracker > 0 && keypressed == true) {
    if (k == 1) {
        first_card = cards[keytracker - 1];
        first_pos = keytracker;
        printf("keypressed %d, /n",k);
    } else if (k == 2) {
        second_card = cards[keytracker - 1];
        second_pos = keytracker;
        printf("keypressed %d, /n",k);

    }
    keypressed = false;
  
  // compare the two selected cards;
  if (k == 2) {
    if (first_card == second_card) {
      // match!!
      *leds_ptr = 1;
      draw_card(first_pos, MATCHED_CARD);
      printf("being matched");
      draw_card(second_pos, MATCHED_CARD);   
      match_counter++;
    } else {
      //not a match loser
      *leds_ptr = 2;
      // flip cards back over
      draw_card(first_pos, BACK);
      draw_card(second_pos, BACK);
    }
  }

  if (match_counter >= 4) {
    // all matches made!
    // PRINT WINNING SCREEN (UGLY FUGLY GREEN)
    gameEnded = true;
    wonGame = true;
    for (int y = 0; y < SCREEN_HEIGHT; y++) {
      for (int x = 0; x < SCREEN_WIDTH; x++) {
        plot_pixel(x, y, win[y][x]);
      }
    }
  }

k++;
  }
  return;
}

void display_hex(int value) {
  const unsigned char bit_codes[] = {
      0b00111111, 0b00000110, 0b01011011, 0b01001111, 0b01100110, 0b01101101,
      0b01111101, 0b00000111, 0b01111111, 0b01100111, 0b01110111, 0b01111100,
      0b00111001, 0b01011110, 0b01111001, 0b01110001};

  volatile unsigned int *hexDisplay1 =
      (unsigned int *)HEX_BASE1;  // Assuming HEX_BASE1 is defined elsewhere

  if (value < 100) {
    unsigned char onesDigit = value % 10;
    unsigned char tensDigit = value / 10;

    // Display ones digit on the first 7-segment display (HEX1)
    unsigned char segmentValue1 = bit_codes[onesDigit];
    unsigned char segmentValue2 = bit_codes[tensDigit];
    *hexDisplay1 = segmentValue1 + (segmentValue2 * 256);
  }
}

void LED_PS2(unsigned char letter) {
   extrainterrupt = true;

  switch (letter) {
    printf("switch");
    case 0x1C:  // Key A
      if(track == 1){ //checking for extra interrupts
        extrainterrupt=false;
        track = 0;
      }
      if(extrainterrupt){ //only proceed for first interrupt, ignore the other ones!
      draw_card(1, cards[0]);
      keytracker = 1;
      keypressed = true;
      track++;
      printf("track %d", track);}
      break;
    case 0x1B:  // Key S
     if(track == 1){
        extrainterrupt=false;
        track = 0;
      }
      if(extrainterrupt){
      draw_card(2, cards[1]);
      keytracker = 2;
      keypressed = true;
      track++;
      }
      break;
    case 0x23:  // Key D
     if(track == 1){
        extrainterrupt=false;
        track = 0;
      }
      if(extrainterrupt){
      draw_card(3, cards[2]);
      keytracker = 3;
      keypressed = true;
      track++;}
      break;
    case 0x2B:  // Key F
      if(track == 1){
        extrainterrupt=false;
        track = 0;
      }
      if(extrainterrupt){
      draw_card(4, cards[3]);
      keytracker = 4;
      keypressed = true;
      track++;}
      break;
    case 0x1A:  // Key Z
      if(track == 1){
        extrainterrupt=false;
        track = 0;
      }
      if(extrainterrupt){
      draw_card(5, cards[4]);
      keytracker = 5;
      keypressed = true;
      track++;}
      break;
    case 0x22:  // Key X
      if(track == 1){
        extrainterrupt=false;
        track = 0;
      }
      if(extrainterrupt){
      draw_card(6, cards[5]);
      keytracker = 6;
      keypressed = true;
      track++;}
      break;
    case 0x21:  // Key C
      if(track == 1){
        extrainterrupt=false;
        track = 0;
      }
      if(extrainterrupt){
      draw_card(7, cards[6]);
      keytracker = 7;
      keypressed = true;
      track++;}
      break;
    case 0x2A:  // Key v
      if(track == 1){
        extrainterrupt=false;
        track = 0;
      }
      if(extrainterrupt){
      draw_card(8, cards[7]);
      keytracker = 8;
      keypressed = true;
      track++;}
      break;
    default:
        keypressed = false;
        break;
  }
  return;
}
  
/*******************************************************************************
 * Reset code; by using the section attribute with the name ".reset" we allow
 *the linker program to locate this code at the proper reset vector address.
 *This code just calls the main program
 ******************************************************************************/
void the_reset(void) __attribute__((section(".reset")));
void the_reset(void) {
  asm(".set noat");       // magic, for the C compiler
  asm(".set nobreak");    // magic, for the C compiler
  asm("movia r2, main");  // call the C language main program
  asm("jmp r2");
}
/* The assembly language code below handles Nios II exception processing. This
 * code should not be modified; instead, the C language code in the function
 * interrupt_handler() can be modified as needed for a given application. */
void the_exception(void) __attribute__((section(".exceptions")));
void the_exception(void)
/*******************************************************************************
 * Exceptions code; by giving the code a section attribute with the name
 *".exceptions" we allow the linker to locate this code at the proper exceptions
 *vector address. This code calls the interrupt handler and later returns from
 *the exception.
 ******************************************************************************/
{
  asm(".set noat");     // magic, for the C compiler
  asm(".set nobreak");  // magic, for the C compiler
  asm("subi sp, sp, 128");
  asm("stw et, 96(sp)");
  asm("rdctl et, ctl4");
  asm("beq et, r0, SKIP_EA_DEC");  // interrupt is not external
  asm("subi ea, ea, 4"); /* must decrement ea by one instruction for external
                          * interrupts, so that the instruction will be run */
  asm("SKIP_EA_DEC:");
  asm("stw r1, 4(sp)");  // save all registers
  asm("stw r2, 8(sp)");
  asm("stw r3, 12(sp)");
  asm("stw r4, 16(sp)");
  asm("stw r5, 20(sp)");
  asm("stw r6, 24(sp)");
  asm("stw r7, 28(sp)");
  asm("stw r8, 32(sp)");
  asm("stw r9, 36(sp)");
  asm("stw r10, 40(sp)");
  asm("stw r11, 44(sp)");
  asm("stw r12, 48(sp)");
  asm("stw r13, 52(sp)");
  asm("stw r14, 56(sp)");
  asm("stw r15, 60(sp)");
  asm("stw r16, 64(sp)");
  asm("stw r17, 68(sp)");
  asm("stw r18, 72(sp)");
  asm("stw r19, 76(sp)");
  asm("stw r20, 80(sp)");
  asm("stw r21, 84(sp)");
  asm("stw r22, 88(sp)");
  asm("stw r23, 92(sp)");
  asm("stw r25, 100(sp)");  // r25 = bt (skip r24 = et, because it was saved
                            // above)
  asm("stw r26, 104(sp)");  // r26 = gp
  // skip r27 because it is sp, and there is no point in saving this
  asm("stw r28, 112(sp)");  // r28 = fp
  asm("stw r29, 116(sp)");  // r29 = ea
  asm("stw r30, 120(sp)");  // r30 = ba
  asm("stw r31, 124(sp)");  // r31 = ra
  asm("addi fp, sp, 128");
  asm("call interrupt_handler");  // call the C language interrupt handler
  asm("ldw r1, 4(sp)");           // restore all registers
  asm("ldw r2, 8(sp)");
  asm("ldw r3, 12(sp)");
  asm("ldw r4, 16(sp)");
  asm("ldw r5, 20(sp)");
  asm("ldw r6, 24(sp)");
  asm("ldw r7, 28(sp)");
  asm("ldw r8, 32(sp)");
  asm("ldw r9, 36(sp)");
  asm("ldw r10, 40(sp)");
  asm("ldw r11, 44(sp)");
  asm("ldw r12, 48(sp)");
  asm("ldw r13, 52(sp)");
  asm("ldw r14, 56(sp)");
  asm("ldw r15, 60(sp)");
  asm("ldw r16, 64(sp)");
  asm("ldw r17, 68(sp)");
  asm("ldw r18, 72(sp)");
  asm("ldw r19, 76(sp)");
  asm("ldw r20, 80(sp)");
  asm("ldw r21, 84(sp)");
  asm("ldw r22, 88(sp)");
  asm("ldw r23, 92(sp)");
  asm("ldw r24, 96(sp)");
  asm("ldw r25, 100(sp)");  // r25 = bt
  asm("ldw r26, 104(sp)");  // r26 = gp
  // skip r27 because it is sp, and we did not save this on the stack
  asm("ldw r28, 112(sp)");  // r28 = fp
  asm("ldw r29, 116(sp)");  // r29 = ea
  asm("ldw r30, 120(sp)");  // r30 = ba
  asm("ldw r31, 124(sp)");  // r31 = ra
  asm("addi sp, sp, 128");
  asm("eret");
}
