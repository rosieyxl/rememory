#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
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
volatile int HEX_BASE1 = 0xff200020;
volatile int HEX_BASE2 = 0xff200030;

#include <stdbool.h>
#include <unistd.h>  // Include the sleep function

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

#define KEY0 0
#define KEY1 1
#define KEY2 2
#define KEY3 3
#define BIT_CODES_LEN 16
// define HEX_BASE1 ((volatile int *)0xff200020)  // Assuming the base address
// for HEX display

volatile int key_pressed = 0;
volatile int pattern = 0x0000000F;
volatile int countdown = 0;
volatile bool countdown_active = false;
int k = 1;
int first_card, second_card;
int first_pos, second_pos;
int graphics[8] = {1, 2, 3, 4, 1, 2, 3, 4};  // the possible graphics
int cards[8];  // the array of cards; will hold their corresponding graphic
int j;
int match_counter = 0;
bool gameStarted = 0;
int counter = 100000000;  // 1/(100 MHz) × (5000000) = 50 msec

void interrupt_handler(void);
void interval_timer_isr(void);
void pushbutton_ISR(void);

void clear_screen(short int color);
void draw_card(int position, int type);
void plot_pixel(int x, int y, short int color);
void display_hex(int value);
int switchCard(int switchval);

int main(void) {
  /* Declare volatile pointers to I/O registers (volatile means that IO load and
   * store instructions will be used to access these pointer locations instead
   * of regular memory loads and stores) */
  volatile int *interval_timer_ptr =
      (int *)0xFF202000;                      // interval timer base address
  volatile int *KEY_ptr = (int *)0xFF200050;  // pushbutton KEY address

  NIOS2_WRITE_IENABLE(0x3); /* set interrupt mask bits for levels 0 (interval
                             * timer) and level 1 (pushbuttons) */
  NIOS2_WRITE_STATUS(1);    // enable Nios II interrupts

  /* set the interval timer period for scrolling the HEX displays */
  *(interval_timer_ptr + 0x2) = (counter & 0xFFFF);
  /* start interval timer, enable its interrupts */
  *(interval_timer_ptr + 1) = 0x7;  // STOP = 0, START = 1, CONT = 1, ITO = 1
  countdown = 6;
  countdown_active = true;

  volatile int *pixel_ctrl_ptr = (int *)0xFF203020;
  pixel_buffer_start = *pixel_ctrl_ptr;

  // Clear the screen with white color
  clear_screen(BACKGROUND_COLOR);

  // RANDOMIZATION OF CARDS
  srand((unsigned int)time(NULL));  // seed with time

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

  // while (!gameStarted);  // idle here until timer is done and cards are
  // flipped

  // turn on interrupts for KEYS
  *(KEY_ptr + 2) = 0xF;  // write to the pushbutton interrupt mask register
                         // and set mask bits to 1

  while (1);  // idle here until game done

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
      for (int i = 0; i < card_height; i++) {
        for (int j = 0; j < card_width; j++) {
          plot_pixel(x + j, y + i, 0x0000FF);  // blue for front side of cards
        }
      }
      plot_pixel(x, y, 0xFFFFFF);
      // 1 white dot on left corner
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
      for (int i = 0; i < card_height; i++) {
        for (int j = 0; j < card_width; j++) {
          plot_pixel(x + j, y + i, 0x00FF00);  // green for front side of cards
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

int switchCard(int switchval) {
  int cardNum = 0;
  switch (switchval & 0b1111) {
    case 0b0001:
      cardNum = 1;
      break;
    case 0b0010:
      cardNum = 2;
      break;
    case 0b0100:
      cardNum = 3;
      break;
    case 0b1000:
      cardNum = 4;
      break;
    case 0b0011:
      cardNum = 5;
      break;
    case 0b0101:
      cardNum = 6;
      break;
    case 0b1001:
      cardNum = 7;
      break;
    case 0b0111:
      cardNum = 8;
      break;
  }
  return cardNum;
}

void interrupt_handler(void) {
  int ipending;
  NIOS2_READ_IPENDING(ipending);
  if (ipending & 0x1) interval_timer_isr();
  if (ipending & 0x2) pushbutton_ISR();
  return;
}

void pushbutton_ISR(void) {
  if (k == 3) {
    k = 1;
  }
  volatile int *KEY_ptr = (int *)0xFF200050;
  // take in value of edge capture reg
  int keyValue = switchCard(*(KEY_ptr + 3));
  // reset edge capture
  *(KEY_ptr + 3) = 0b1111;
  if (keyValue > 0) {
    // 'flip' the card chosen (draw its front side)
    draw_card(keyValue, cards[keyValue - 1]);
  }
  // store value of chosen card
  if (k == 1) {
    first_card = cards[keyValue - 1];
    first_pos = keyValue;
  } else if (k == 2) {
    second_card = cards[keyValue - 1];
    second_pos = keyValue;
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
  }

  if (match_counter >= 4) {
    // all matches made!
    // PRINT WINNING SCREEN (UGLY FUGLY GREEN)
    clear_screen(0x00FF00);
  }

  k++;
}

void interval_timer_isr(void) {
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
    for (int i = 1; i <= 8; i++) {
      draw_card(i, BACK);
    }
  }
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
