//-----------------------------------------------------------------------------
// info_display.c: Information Display Module for 6502 Emulator
// 11/06/2025 github.com/SMDHuman
//-----------------------------------------------------------------------------
#include "info_display.h"
#include "esp_log.h"
#define P_ARRAY_IMPLEMENTATION
#include "p_array.h"
//-----------------------------------------------------------------------------
TFT_t dev;
FontxFile font[2];
FontxFile font_small[2];
array* idisplay_blocks;

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
}
//-----------------------------------------------------------------------------
static void idisplay_draw_block(TFT_t *dv, idisplay_block_t *block) {
	char text[32];
	sprintf(text, "%s", block->label);
	if(block->value_text_len > 0 && block->label[0] != 0) {
		strcat(text, ":");
	}
	if(block->value_text_len == 1)
		sprintf(text+strlen(text), "%01X", (unsigned int)(block->value));
	else if(block->value_text_len == 2)
		sprintf(text+strlen(text), "%02X", (unsigned int)(block->value));
	else if(block->value_text_len == 4)
		sprintf(text+strlen(text), "%04X", (unsigned int)(block->value));
	uint8_t x = block->x;
	uint8_t y = block->y;
	draw_info_block(dv, text, x, y, block->bool_value);
}
//-----------------------------------------------------------------------------
void idisplay_set_block(uint8_t index, idisplay_block_t *block) {
	array_set(idisplay_blocks, index, block);
	idisplay_draw_block(&dev, block); // Draw the block immediately
}
//-----------------------------------------------------------------------------
uint8_t idisplay_add_block(idisplay_block_t *block) {
	array_push(idisplay_blocks, block);
	idisplay_draw_block(&dev, block); // Draw the block immediately
	return((uint8_t)(idisplay_blocks->length - 1)); // Return the index of the newly added block
}
//-----------------------------------------------------------------------------
uint8_t idisplay_create_block(const char *label, uint8_t value_text_len,
															uint8_t x, uint8_t y) {
	idisplay_block_t new_block;
	strncpy(new_block.label, label, sizeof(new_block.label) - 1);
	new_block.label[sizeof(new_block.label) - 1] = '\0'; // Ensure null-termination
	new_block.value = 0; // Default value
	new_block.value_text_len = value_text_len;
	new_block.x = x;
	new_block.y = y;
	new_block.bool_value = 0; // Default boolean value

	return idisplay_add_block(&new_block);
}
//-----------------------------------------------------------------------------
// Updates the block data at the specified index *if the new data is different
void idisplay_update_block(uint8_t index, idisplay_block_t *block) {
	// If new text different from existing text, update it
	idisplay_block_t existing_block;
	array_get(idisplay_blocks, index, &existing_block);
	if(memcmp(&existing_block, block, sizeof(idisplay_block_t)) == 0) {
		return; // No change needed
	}
	array_set(idisplay_blocks, index, block);
	idisplay_draw_block(&dev, block); // Redraw the block with updated values
}
//-----------------------------------------------------------------------------
void idisplay_update_block_value(uint8_t index, int32_t value) {
	idisplay_block_t block;
	array_get(idisplay_blocks, index, &block);
	block.value = value; // Update the value
	idisplay_update_block(index, &block); // Update the block with new value
}
//-----------------------------------------------------------------------------
void idisplay_update_block_bool(uint8_t index, int32_t bool_value) {
	idisplay_block_t block;
	array_get(idisplay_blocks, index, &block);
	block.bool_value = bool_value; // Update the value
	idisplay_update_block(index, &block); // Update the block with new value
}
//-----------------------------------------------------------------------------
void idisplay_update_block_label(uint8_t index, const char *label) {
	idisplay_block_t block;
	array_get(idisplay_blocks, index, &block);
	strncpy(block.label, label, sizeof(block.label) - 1);
	block.label[sizeof(block.label) - 1] = '\0'; // Ensure null-termination
	idisplay_update_block(index, &block); // Update the block with new label
}

//-----------------------------------------------------------------------------
// Initializes the information display module
void idisplay_init(){
	idisplay_blocks = array_create(32, sizeof(idisplay_block_t));
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
}

//-----------------------------------------------------------------------------
void idisplay_task(){

	// Draw Interrupt Request (IRQ) status
	uint8_t block_irq = idisplay_create_block("IRQ", 0, 1, 3);
	// Draw Non-Maskable Interrupt (NMI) status
	uint8_t block_nmi = idisplay_create_block("NMI", 0, 5, 3);
	// Draw Accumulator
	uint8_t block_a = idisplay_create_block("A", 2, 10, 3);
	// Draw Stack Pointer
	uint8_t block_sp = idisplay_create_block("SP", 2, 1, 6);
	// Draw X Register
	uint8_t block_x = idisplay_create_block("X", 2, 10, 6);
	//// Draw Program Counter
	uint8_t block_pc = idisplay_create_block("PC", 4, 1, 9);
	// Draw Y Register
	uint8_t block_y = idisplay_create_block("Y", 2, 10, 9);
	// Draw Memory Access Header
	char buffer[32];
	sprintf(buffer, "MEMORY ACCESS");
	lcdDrawString(&dev, font_small, 1*grid_size, 10*grid_size+2, (uint8_t*)buffer, theme_colors[2]);
	// Draw Memory Read/Write
	uint8_t block_rw = idisplay_create_block("-", 0, 1, 12);
	// Draw Memory Address
	uint8_t block_address = idisplay_create_block("$", 4, 3, 12);
	// Draw Memory Data
	uint8_t block_data = idisplay_create_block("$", 2, 10, 12);
	// Draw Status Registers Header
	sprintf(buffer, "STATUS REGISTERS");
	lcdDrawString(&dev, font_small, 1*grid_size, 13*grid_size+2, (uint8_t*)buffer, theme_colors[2]);
	//// Draw Status Register
	uint8_t block_n = idisplay_create_block("N", 0, 1, 15);
	uint8_t block_v = idisplay_create_block("V", 0, 3, 15);
	uint8_t block_b = idisplay_create_block("B", 0, 5, 15);
	uint8_t block_d = idisplay_create_block("D", 0, 7, 15);
	uint8_t block_i = idisplay_create_block("I", 0, 9, 15);
	uint8_t block_z = idisplay_create_block("Z", 0, 11, 15);
	uint8_t block_c = idisplay_create_block("C", 0, 13, 15);
	while(1){
		// Update Interrupt Request (IRQ) status
		//idisplay_update_block_bool(block_irq, );
		// Update Non-Maskable Interrupt (NMI) status
		//idisplay_update_block_bool(block_nmi, );
		// Update Accumulator
		idisplay_update_block_value(block_a, *fake6502_a);
		// Update Stack Pointer
		idisplay_update_block_value(block_sp, *fake6502_sp);
		// Update X Register
		idisplay_update_block_value(block_x, *fake6502_x);
		// Update Program Counter
		idisplay_update_block_value(block_pc, *fake6502_pc);
		// Update Y Register
		idisplay_update_block_value(block_y, *fake6502_y);
		// Update Memory Access Address
		idisplay_update_block_value(block_address, fake6502_memaccess_address);
		// Update Memory Access Data
		idisplay_update_block_value(block_data, fake6502_memaccess_data);
		// Update Memory Access Read/Write
		if(fake6502_memaccess_mode == 1) {
			idisplay_update_block_label(block_rw, "R");
		} else if(fake6502_memaccess_mode == 2) {
			idisplay_update_block_label(block_rw, "W");
		} else {
			idisplay_update_block_label(block_rw, "-");
		}
		// Update Status Registers
		{
		idisplay_update_block_bool(block_n, (*fake6502_status & 0x80) ? 1 : 0);
		idisplay_update_block_bool(block_v, (*fake6502_status & 0x40) ? 1 : 0);
		idisplay_update_block_bool(block_b, (*fake6502_status & 0x10) ? 1 : 0);
		idisplay_update_block_bool(block_d, (*fake6502_status & 0x08) ? 1 : 0);
		idisplay_update_block_bool(block_i, (*fake6502_status & 0x04) ? 1 : 0);
		idisplay_update_block_bool(block_z, (*fake6502_status & 0x02) ? 1 : 0);
		idisplay_update_block_bool(block_c, (*fake6502_status & 0x01) ? 1 : 0); 
}
    vTaskDelay(1); 
	}
}

