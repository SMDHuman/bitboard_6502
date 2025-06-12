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

extern unsigned int idisplay_numbers[32];

//-----------------------------------------------------------------------------
void idisplay_init();
void idisplay_task();

#endif