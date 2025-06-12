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
#define COLOR2 0x222222 // #222222
#define COLOR3 0x1DCD9F // #1DCD9F
#define COLOR4 0x169976 // #169976

//-----------------------------------------------------------------------------
void idisplay_init();
void idisplay_task();

#endif