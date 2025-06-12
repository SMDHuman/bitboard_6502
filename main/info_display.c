//-----------------------------------------------------------------------------
// info_display.c: Information Display Module for 6502 Emulator
// 11/06/2025 github.com/SMDHuman
//-----------------------------------------------------------------------------
#include "info_display.h"
#include "esp_log.h"
//-----------------------------------------------------------------------------
TFT_t dev;
FontxFile font[2];
FontxFile font_small[2];

unsigned int idisplay_numbers[32];
static char idisplay_blocks_text[32][32];
static uint8_t idisplay_blocks_bool[32];
static uint8_t idisplay_blocks_x[32];
static uint8_t idisplay_blocks_y[32];
static uint8_t idisplay_blocks_count = 0;

static const uint8_t grid_size = 15;
static const uint16_t theme_colors[4] = {
	hex565(COLOR1),
	hex565(COLOR2),
	hex565(COLOR3),
	hex565(COLOR4)
};

//-----------------------------------------------------------------------------
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

	InitFontx(font,"/fonts/DIGITAL7_30x15.FNT",""); 
	InitFontx(font_small,"/fonts/DIGITAL7_16x8.FNT",""); 
}

//-----------------------------------------------------------------------------
static void draw_info_block(TFT_t *dv, const char *text, uint16_t x, uint16_t y, uint8_t value) {
	if(value){
		lcdDrawFillRect(dv, grid_size*x, grid_size*(y-2), grid_size*(x+strlen(text)+1), grid_size*y, theme_colors[1]);
	}else{
		lcdDrawFillRect(dv, grid_size*x, grid_size*(y-2), grid_size*(x+strlen(text)+1), grid_size*y, theme_colors[0]);
	}
	lcdDrawRoundRect(dv, grid_size*x, grid_size*(y-2), grid_size*(x+strlen(text)+1), grid_size*y, 
									 4, theme_colors[3]);
	lcdDrawString(dv, font, x*grid_size+5, y*grid_size + 1, (uint8_t*)text, theme_colors[2]);
	return;
}
//-----------------------------------------------------------------------------
static void idisplay_draw_block(TFT_t *dv, uint8_t index) {
	if(index >= idisplay_blocks_count) {
		ESP_LOGE("IDISPLAY", "Block index %d out of range (max %d)", index, idisplay_blocks_count - 1);
		return;
	}
	const char *text = idisplay_blocks_text[index];
	uint8_t x = idisplay_blocks_x[index];
	uint8_t y = idisplay_blocks_y[index];
	draw_info_block(dv, text, x, y, idisplay_blocks_bool[index]);
	return;
}
static void idisplay_set_block(uint8_t index, const char *text, uint8_t x, uint8_t y) {
	if(index < 32) {
		strncpy(idisplay_blocks_text[index], text, sizeof(idisplay_blocks_text[index]) - 1);
		idisplay_blocks_text[index][sizeof(idisplay_blocks_text[index]) - 1] = '\0'; // Ensure null-termination
		idisplay_blocks_x[index] = x;
		idisplay_blocks_y[index] = y;
		idisplay_draw_block(&dev, index); // Draw the block immediately
	}
	return;
}
void idisplay_add_block(const char *text, uint8_t x, uint8_t y) {
	if(idisplay_blocks_count < 32) {
		idisplay_set_block(idisplay_blocks_count++, text, x, y);
	}
	return;
}
void idisplay_update_block_text(uint8_t index, const char *text, uint8_t text_len, uint8_t block_bool) {
	// If new text different from existing text, update it
	if((strcmp(idisplay_blocks_text[index], text) == 0) && 
	   (idisplay_blocks_bool[index] == block_bool)) {
		return; // No change needed
	}
	if(text == NULL || text_len == 0) {
		ESP_LOGE("IDISPLAY", "Invalid text or text length");
		return;
	}
	if(index >= 32) {
		ESP_LOGE("IDISPLAY", "Block index %d out of range (max 31)", index);
		return;
	}
	if(text_len >= sizeof(idisplay_blocks_text[index])) {
		ESP_LOGE("IDISPLAY", "Text length exceeds block size");
		return;
	}
	if(index < idisplay_blocks_count) {
		strncpy(idisplay_blocks_text[index], text, text_len);
		idisplay_blocks_text[index][text_len] = '\0'; // Ensure null-termination
		idisplay_blocks_bool[index] = block_bool; // Update the boolean state
		idisplay_draw_block(&dev, index); // Redraw the block with updated text
	} else {
		ESP_LOGE("IDISPLAY", "Block index %d out of range (max %d)", index, idisplay_blocks_count - 1);
	}
	return;
}

//-----------------------------------------------------------------------------
// Initializes the information display module
void idisplay_init(){
	// Initialize the display
	spi_master_init(&dev, CONFIG_MOSI_GPIO, 
					CONFIG_SCLK_GPIO, CONFIG_CS_GPIO, 
					CONFIG_DC_GPIO, CONFIG_RESET_GPIO, 
					CONFIG_BL_GPIO);
	lcdInit(&dev, CONFIG_WIDTH, CONFIG_HEIGHT, 
			CONFIG_OFFSETX, CONFIG_OFFSETY);
  
	// Initialize the font
  font_init();
  lcdSetFontDirection(&dev, 0); // Set font direction to 0 (normal)
	
	// Draw for demo
	lcdFillScreen(&dev, theme_colors[0]);
	
	char buffer[32];

	// Draw Interrupt Request (IRQ) status
	idisplay_add_block("IRQ", 1, 3);
	// Draw Non-Maskable Interrupt (NMI) status
	idisplay_add_block("NMI", 5, 3);
	// Draw Accumulator
	sprintf(buffer, "A:%02X", 0);
	idisplay_add_block(buffer, 10, 3);
	// Draw Stack Pointer
	sprintf(buffer, "SP:%02X", 0);
	idisplay_add_block(buffer, 1, 6);
	// Draw X Register
	sprintf(buffer, "X:%02X", 0);
	idisplay_add_block(buffer, 10, 6);
	//// Draw Program Counter
	sprintf(buffer, "PC:%04X", 0);
	idisplay_add_block(buffer, 1, 9);
	// Draw Y Register
	sprintf(buffer, "Y:%02X", 0);
	idisplay_add_block(buffer, 10, 9);
	// Draw Memory Access Header
	sprintf(buffer, "MEMORY ACCESS");
	lcdDrawString(&dev, font_small, 1*grid_size, 10*grid_size+2, (uint8_t*)buffer, theme_colors[2]);
	// Draw Memory Read/Write
	idisplay_add_block("-", 1, 12);
	// Draw Memory Address
	sprintf(buffer, "$%04X", 0);
	idisplay_add_block(buffer, 3, 12);
	// Draw Memory Data
	sprintf(buffer, "$%02X", 0);
	idisplay_add_block(buffer, 10, 12);
	// Draw Status Registers Header
	sprintf(buffer, "STATUS REGISTERS");
	lcdDrawString(&dev, font_small, 1*grid_size, 13*grid_size+2, (uint8_t*)buffer, theme_colors[2]);
	//// Draw Status Register
	idisplay_add_block("N", 1, 15);
	idisplay_add_block("V", 3, 15);
	idisplay_add_block("B", 5, 15);
	idisplay_add_block("D", 7, 15);
	idisplay_add_block("I", 9, 15);
	idisplay_add_block("Z", 11, 15);
	idisplay_add_block("C", 13, 15);

}

//-----------------------------------------------------------------------------
void idisplay_task(){
	char buffer[32];
	while(1){
		// Update Interrupt Request (IRQ) status
		idisplay_update_block_text(0, "IRQ", 3, idisplay_numbers[0]);
		// Update Non-Maskable Interrupt (NMI) status
		idisplay_update_block_text(1, "NMI", 3, idisplay_numbers[1]);
		// Update Accumulator
		sprintf(buffer, "A:%02X", idisplay_numbers[2]);
		idisplay_update_block_text(2, buffer, 4, 0);
		// Update Stack Pointer
		sprintf(buffer, "SP:%02X",idisplay_numbers[3]);
		idisplay_update_block_text(3, buffer, 5, 0);
		// Update X Register
		sprintf(buffer, "X:%02X", idisplay_numbers[4]);
		idisplay_update_block_text(4, buffer, 4, 0);
		// Update Program Counter
		sprintf(buffer, "PC:%04X",idisplay_numbers[5]);
		idisplay_update_block_text(5, buffer, 7, 0);
		// Update Y Register
		sprintf(buffer, "Y:%02X", idisplay_numbers[6]);
		idisplay_update_block_text(6, buffer, 4, 0);

		// Update Memory Access Address
		sprintf(buffer, "$%04X", idisplay_numbers[8]);
		idisplay_update_block_text(8, buffer, 5, 0);
		// Update Memory Access Data
		sprintf(buffer, "$%02X", idisplay_numbers[9]);
		idisplay_update_block_text(9, buffer, 5, 0);

		// Update Status Registers 
		idisplay_update_block_text(10, "N", 1, idisplay_numbers[10]);
		idisplay_update_block_text(11, "V", 1, idisplay_numbers[11]);
		idisplay_update_block_text(12, "B", 1, idisplay_numbers[12]);
		idisplay_update_block_text(13, "D", 1, idisplay_numbers[13]);
		idisplay_update_block_text(14, "I", 1, idisplay_numbers[14]);
		idisplay_update_block_text(15, "Z", 1, idisplay_numbers[15]);
		idisplay_update_block_text(16, "C", 1, idisplay_numbers[16]);

    vTaskDelay(1); // Adjust delay as needed
	}
}

