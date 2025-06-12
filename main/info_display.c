//-----------------------------------------------------------------------------
// info_display.c: Information Display Module for 6502 Emulator
// 11/06/2025 github.com/SMDHuman
//-----------------------------------------------------------------------------
#include "info_display.h"

TFT_t dev;
MCS6502ExecutionContext *cpu;
FontxFile font[2];
FontxFile font_small[2];

static const uint8_t grid_size = 15;
static const uint16_t theme_colors[4] = {
	hex565(COLOR1),
	hex565(COLOR2),
	hex565(COLOR3),
	hex565(COLOR4)
};

static void font_init(){
	esp_vfs_spiffs_conf_t conf = {
		.base_path = "/fonts",
		.partition_label = "storage1",
		.max_files = 7,
		.format_if_mount_failed = true
	};

	// Use settings defined above to initialize and mount SPIFFS filesystem.
	// Note: esp_vfs_spiffs_register is an all-in-one convenience function.
	esp_err_t ret = esp_vfs_spiffs_register(&conf);
	if(ret != ESP_OK) {
		if (ret == ESP_FAIL) {
			printf("Failed to mount or format filesystem\n");
		} else if (ret == ESP_ERR_NOT_FOUND) {
			printf("Failed to find SPIFFS partition\n");
		} else {
			printf("Failed to initialize SPIFFS (%s)\n", esp_err_to_name(ret));
		}
		return;
	}

	InitFontx(font,"/fonts/DIGITAL7_32x16.FNT",""); 
	InitFontx(font_small,"/fonts/DIGITAL7_16x8.FNT",""); 
}

//-----------------------------------------------------------------------------
// Draws the grid boxes on the display
static void draw_boxes(TFT_t *dv) {
	const uint8_t box_color = 1;
	const uint8_t sep_color = 3;
	
	lcdDrawFillRect(dv, grid_size, grid_size, grid_size*9, grid_size*3, theme_colors[box_color]); // IRQ
	lcdDrawLine(dv, grid_size*5, grid_size*1, grid_size*5, grid_size*3, theme_colors[sep_color]); // NMI
	lcdDrawFillRect(dv, grid_size*10, grid_size, grid_size*15, grid_size*3, theme_colors[box_color]); // Accumulator
	
	lcdDrawFillRect(dv, grid_size, grid_size*4, grid_size*7, grid_size*6, theme_colors[box_color]); // Stack Pointer
	lcdDrawFillRect(dv, grid_size*10, grid_size*4, grid_size*15, grid_size*6, theme_colors[box_color]); // X Register

	lcdDrawFillRect(dv, grid_size, grid_size*7, grid_size*9, grid_size*9, theme_colors[box_color]); // Program Counter
	lcdDrawFillRect(dv, grid_size*10, grid_size*7, grid_size*15, grid_size*9, theme_colors[box_color]); // Y Register

	lcdDrawFillRect(dv, grid_size, grid_size*10, grid_size*15, grid_size*12, theme_colors[box_color]); // Memory R/W
	lcdDrawLine(dv, grid_size*3, grid_size*10, grid_size*3, grid_size*12, theme_colors[sep_color]); // Memory Address
	lcdDrawLine(dv, grid_size*10, grid_size*10, grid_size*10, grid_size*12, theme_colors[sep_color]); // Memory Data

	lcdDrawFillRect(dv, grid_size, grid_size*13, grid_size*15, grid_size*15, theme_colors[box_color]); // Status Registers
	for (uint8_t i = 3; i < 15; i+=2) {
		lcdDrawLine(dv, grid_size*i, grid_size*13, grid_size*i, grid_size*15, theme_colors[sep_color]);
	}
}

//-----------------------------------------------------------------------------
static void draw_info_text(TFT_t *dv, const char *text, uint16_t x, uint16_t y, uint8_t color_i) {
	uint8_t buffer[32];
	strncpy((char*)buffer, text, sizeof(buffer) - 1);
	buffer[sizeof(buffer) - 1] = '\0'; // Ensure null-termination
	lcdDrawString(dv, font, x*grid_size+5, y*grid_size + 1, buffer, theme_colors[color_i]);
	return;
}
// Draws the CPU state information on the display
static void draw_info(TFT_t *dv, MCS6502ExecutionContext *context) {
	const uint8_t text_color = 2;
	const uint8_t text_xpad = 5;
	// Draw CPU state information
	char buffer[32];

	// Draw Interrupt Request (IRQ) status
	draw_info_text(dv, "IRQ", 1, 3, text_color);
	// Draw Non-Maskable Interrupt (NMI) status
	draw_info_text(dv, "NMI", 5, 3, text_color);
	
	// Draw Accumulator
	sprintf(buffer, "A:%02X", context->a);
	draw_info_text(dv, buffer, 10, 3, text_color);
	// Draw Stack Pointer
	sprintf(buffer, "SP:%02X", context->sp);
	draw_info_text(dv, buffer, 1, 6, text_color);
	// Draw X Register
	sprintf(buffer, "X:%02X", context->x);
	draw_info_text(dv, buffer, 10, 6, text_color);
	// Draw Y Register
	sprintf(buffer, "Y:%02X", context->y);
	draw_info_text(dv, buffer, 10, 9, text_color);
	//// Draw Program Counter
	sprintf(buffer, "PC:%04X", context->pc);
	draw_info_text(dv, buffer, 1, 9, text_color);
	// Draw Memory Access Header
	sprintf(buffer, "MEMORY ACCESS");
	lcdDrawString(dv, font_small, 1*grid_size+text_xpad, 10*grid_size+2, (uint8_t*)buffer, theme_colors[text_color]);
	// Draw Memory Read/Write
	draw_info_text(dv, "-", 1, 12, text_color);
	// Draw Memory Address
	sprintf(buffer, "$%04X", 0);
	draw_info_text(dv, buffer, 3, 12, text_color);
	// Draw Memory Data
	sprintf(buffer, "$%02X", context->readByte(0, context->readWriteContext));
	draw_info_text(dv, buffer, 10, 12, text_color);
	// Draw Status Registers Header
	sprintf(buffer, "STATUS REGISTERS");
	lcdDrawString(dv, font_small, 1*grid_size+text_xpad, 13*grid_size+2, (uint8_t*)buffer, theme_colors[text_color]);
	//// Draw Status Register
	draw_info_text(dv, "N V B", 1, 15, text_color);
	draw_info_text(dv, "D I Z C", 7, 15, text_color);
	return;
}


//-----------------------------------------------------------------------------
// Initializes the information display module
void idisplay_init(MCS6502ExecutionContext *context){
	spi_master_init(&dev, CONFIG_MOSI_GPIO, 
					CONFIG_SCLK_GPIO, CONFIG_CS_GPIO, 
					CONFIG_DC_GPIO, CONFIG_RESET_GPIO, 
					CONFIG_BL_GPIO);
	lcdInit(&dev, CONFIG_WIDTH, CONFIG_HEIGHT, 
			CONFIG_OFFSETX, CONFIG_OFFSETY);
  
  font_init();
  lcdSetFontDirection(&dev, 0); // Set font direction to 0 (normal)

	lcdFillScreen(&dev, theme_colors[0]);
	draw_boxes(&dev);

	draw_info(&dev, context);

  return;
}

//-----------------------------------------------------------------------------
void idisplay_task(void){
  return;
}

