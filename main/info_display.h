//-----------------------------------------------------------------------------
// info_display.h: Header file of Information Display Module for 6502 Emulator
// 11/06/2025 github.com/SMDHuman
//-----------------------------------------------------------------------------
#ifndef INFGO_DISPLAY_H
#define INFGO_DISPLAY_H

#include "st7789.h"
#include "fontx.h"

#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "esp_spiffs.h"
//-----------------------------------------------------------------------------

#define COLOR1 0x000000 // #000000
#define COLOR2 0x273F4F // #273F4F
#define COLOR3 0xEFEEEA // #EFEEEA
#define COLOR4 0xFE7743 // #FE7743

extern idisplay_block_t idisplay_blocks[32];
extern uint8_t idisplay_blocks_count = 0;
typedef struct {
	char label[32]; 		// Text to display
	int32_t value;      // Value to display (can be used for numbers)
	uint8_t value_text_len; // Length of the value text
	uint8_t display_value_enabled; // Flag to enable/disable value display
	uint8_t x;        // X position in grid
	uint8_t y;        // Y position in grid
	uint8_t bool_value; // Boolean value for the block (0 or 1)
}idisplay_block_t;

//-----------------------------------------------------------------------------
void idisplay_init();
void idisplay_task();

#endif