#include <stdbool.h>
#include <stdlib.h>

#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240
#define CARD_WIDTH 50
#define CARD_HEIGHT 70
#define CARD_GAP 10
#define BACKGROUND_COLOR 0xFFFF


int pixel_buffer_start; // Global variable

void draw_card(int x, int y);

void main(void)
{
    volatile int *pixel_ctrl_ptr = (int *)0xFF203020;
    pixel_buffer_start = *pixel_ctrl_ptr;

    // Clear the screen with baby blue color
    clear_screen(BACKGROUND_COLOR);

    // Draw cards at various positions

    // Top left
    draw_card(30, 60);

    // Top right
    draw_card(30+CARD_WIDTH+20, 60);
	
	draw_card(100+CARD_WIDTH+20, 60);
	
	draw_card(170+CARD_WIDTH+20, 60);

    // Bottom left
    draw_card(30, SCREEN_HEIGHT - CARD_HEIGHT - 20);
	
	draw_card(30+CARD_WIDTH+20, SCREEN_HEIGHT - CARD_HEIGHT - 20);
	
	draw_card(100+CARD_WIDTH+20, SCREEN_HEIGHT - CARD_HEIGHT - 20);
	
    draw_card(170+CARD_WIDTH+20, SCREEN_HEIGHT - CARD_HEIGHT - 20);


    while (1)
    {
        // Your game logic can go here
    }
}

void clear_screen(short int color)
{
    for (int y = 0; y < SCREEN_HEIGHT; y++)
    {
        for (int x = 0; x < SCREEN_WIDTH; x++)
        {
            plot_pixel(x, y, color);
        }
    }
}

void draw_card(int x, int y)
{
    const int card_width = CARD_WIDTH;
    const int card_height = CARD_HEIGHT;

    for (int i = 0; i < card_height; i++)
    {
        for (int j = 0; j < card_width; j++)
        {
            plot_pixel(x + j, y + i, 0xFFD1DC); // Red color for cards
        }
    }
}


void plot_pixel(int x, int y, short int color)
{
    volatile short int *one_pixel_address;
    one_pixel_address = pixel_buffer_start + (y << 10) + (x << 1);
    *one_pixel_address = color;
}


