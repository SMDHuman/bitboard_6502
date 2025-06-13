#include <stdio.h>
#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_err.h"
#include "driver/gpio.h"

#include "fake6502.c"
#include "info_display.h"

//-----------------------------------------------------------------------------
uint8_t break_flag = 0; // Global variable to track the break flag
uint8_t stack[256]; // Simulated stack 
uint8_t fakemem[65536]; // Simulated memory for the 6502 CPU

static const uint16_t EXEC_START = 0xE000;

// Demo Fibonacci program
static const uint8_t program[] = {
  0xA9, 0x00,       // LDA #$00
  0x85, 0x00,       // STA $00
  0xA9, 0x01,       // LDA #$01
  0x85, 0x01,       // STA $01
  // loop:
  0xA5, 0x00,       // LDA $00
  0x18,             // CLC
  0x65, 0x01,       // ADC $01
  0x85, 0x02,       // STA $02
  0xB0, 0x07,       // BCS done (skip next 7 bytes if carry set)
  0xA5, 0x01,       // LDA $01
  0x85, 0x00,       // STA $00
  0xA5, 0x02,       // LDA $02
  0x85, 0x01,       // STA $01
  0x4C, 0x08, 0xE0, // JMP loop (absolute to EXEC_START+8)
  // done:
  0x00              // BRK
};
//-----------------------------------------------------------------------------
uint8_t read6502(uint16_t addr){
  // Debugging output
  //idisplay_numbers[7] = 1;
  uint8_t return_data = fakemem[addr]; // Default return data from memory
  printf("MCS6502_Read called with addr: %04X\n", addr);
  idisplay_numbers[8] = addr; // Update display with memory address being read
  // Handle hardware interrupt vectors
  switch(addr) {
    case 0xFFFE: // IRQ/BRK vector (low byte)
    case 0xFFFF: // IRQ/BRK vector (high byte)
      break_flag = 1; // Set break flag when IRQ/BRK vector is read
      return_data = 0x00;
      break;
    case 0xFFFA: // 0xFFFA and 0xFFFB (NMI vector)
    case 0xFFFB: // NMI vector (high byte)
      return_data = 0x00; // Return 0 for NMI vector
      break;
  }
  idisplay_numbers[9] = return_data; // Update display with memory data read  
  return return_data; // Placeholder for read function
}
//-----------------------------------------------------------------------------
void write6502(uint16_t addr, uint8_t byte)
{
  // Debugging output
  //idisplay_numbers[7] = 2;
  idisplay_numbers[8] = addr; // Update display with memory address being written
  idisplay_numbers[9] = byte; // Update display with memory data written
  printf("MCS6502_Write called with addr: %04X, byte: %02X\n", addr, byte);
  fakemem[addr] = byte; // Write to fake memory  
}

//-----------------------------------------------------------------------------
// Initialize the 6502 Memory
void fakemem_init(){
  memset(fakemem, 0, sizeof(fakemem)); // Initialize fake memory
  fakemem[0xFFFC] = EXEC_START & 0xFF; // Set reset vector low byte
  fakemem[0xFFFD] = (EXEC_START >> 8) & 0xFF; // Set reset vector high byt
}

//-----------------------------------------------------------------------------
void app_main(void)
{
  // Initialize the IO Buttons and Leds
  gpio_config_t config_input = {
    .pin_bit_mask = (1ULL << GPIO_NUM_0) | (1ULL << GPIO_NUM_13), // GPIO 0 and 13 Buttons
    .mode = GPIO_MODE_INPUT,
    .pull_up_en = GPIO_PULLUP_DISABLE,
    .pull_down_en = GPIO_PULLDOWN_ENABLE,
    .intr_type = GPIO_INTR_DISABLE
  };
  gpio_config(&config_input); // Configure GPIO 0 and 13 as input with pull-up enabled
  gpio_config_t config_led = {
    .pin_bit_mask = (1ULL << GPIO_NUM_45), // GPIO 45 LED
    .mode = GPIO_MODE_OUTPUT,
    .pull_up_en = GPIO_PULLUP_DISABLE,
    .pull_down_en = GPIO_PULLDOWN_DISABLE,
    .intr_type = GPIO_INTR_DISABLE // No interrupt for output pins
  };
  gpio_config(&config_led); // Configure GPIO 45 as output
  // Initialize fake memory
  fakemem_init(); 
  // Load program into memory
  memcpy(&fakemem[EXEC_START], program, sizeof(program)); 
  printf("Program loaded into memory at address %04X\n", EXEC_START);
  // Initialize the 6502 CPU state
  reset6502(); 
  
  // Initialize the display
  idisplay_init(); 
  xTaskCreatePinnedToCore(
    (TaskFunction_t)idisplay_task, // Task function
    "idisplay_task", // Task name
    2048, // Stack size
    NULL, // Task parameters
    1, // Priority
    NULL, // Task handle
    1 // Core ID (0 for core 0)
  );
  
  while(1) {
    if(!gpio_get_level(GPIO_NUM_0)) { // Check if GPIO 0 button is pressed
      printf("Resetting 6502 CPU...\n");
      reset6502(); // Reset the CPU state
      break_flag = 0; // Clear the break flag
    }
    if(break_flag){
      vTaskDelay(pdMS_TO_TICKS(1)); // Adjust delay as needed
      continue; // Skip execution if break flag is set
    }
    step6502(); // Execute a single instruction
    
    idisplay_numbers[2] = a; // Update display with accumulator value
    idisplay_numbers[3] = sp; // Update display with stack pointer value
    idisplay_numbers[4] = x; // Update display with X register value
    idisplay_numbers[5] = pc; // Update display with Y register value
    idisplay_numbers[6] = y; // Update display with Y register value
    idisplay_numbers[10] = status & 0x80; // Update display with status register (N flag)
    idisplay_numbers[11] = status & 0x40; // Update display with status register (V flag)
    idisplay_numbers[12] = status & 0x10; // Update display with status register (B flag)
    idisplay_numbers[13] = status & 0x08; // Update display with status register (D flag)
    idisplay_numbers[14] = status & 0x04; // Update display with status register (I flag)
    idisplay_numbers[15] = status & 0x02; // Update display with status register (Z flag)
    idisplay_numbers[16] = status & 0x01; // Update display with status register (C flag)
    vTaskDelay(pdMS_TO_TICKS(50)); // Adjust delay as needed
  }
}

