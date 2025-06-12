#include <stdio.h>
#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_err.h"

#include "fake6502.c"
#include "info_display.h"

//-----------------------------------------------------------------------------
uint8_t break_flag = 0; // Global variable to track the break flag
uint8_t stack[256]; // Simulated stack 

static const uint16_t EXEC_START = 0xE000;

static const uint8_t program[] = {
  0xA9, 0x01, // LDA #$01
  0x48,        // PHA (push accumulator to stack)
  0xA9, 0x04, // LDA #$04
  0xA2, 0x02, // LDX #$02
  0xA0, 0x03, // LDY #$03
  0x68,        // PLA (pull accumulator from stack)
  0x00        // BRK (end of program)
};
//-----------------------------------------------------------------------------
uint8_t read6502(uint16_t addr){
  // Debugging output
  printf("MCS6502_Read called with addr: %04X\n", addr);

  // Handle hardware interrupt vectors
  if(addr >= 0xFFFA && addr <= 0xFFFF) {
    switch(addr) {
      case 0xFFFC: return EXEC_START & 0xFF;       // Reset vector (low byte)
      case 0xFFFD: return (EXEC_START >> 8) & 0xFF; // Reset vector (high byte)
      case 0xFFFE: // IRQ/BRK vector (low byte)
      case 0xFFFF: // IRQ/BRK vector (high byte)
        break_flag = 1; // Set break flag when IRQ/BRK vector is read
        return 0x00;
      default: // 0xFFFA and 0xFFFB (NMI vector)
        return 0x00;
    }
  }
  // Handle stack operations
  else if(addr >= 0x0100 && addr < 0x0200) {
    // Stack operations
    return stack[addr - 0x100]; // Read from stack
  }

  // Handle program memory read
  else if(addr >= EXEC_START && addr < EXEC_START + sizeof(program)) {
    return program[addr - EXEC_START]; // Return program byte
  }

  return 0; // Placeholder for read function
}
//-----------------------------------------------------------------------------
void write6502(uint16_t addr, uint8_t byte)
{
  // Debugging output
  printf("MCS6502_Write called with addr: %04X, byte: %02X\n", addr, byte);
  // Handle stack operations
  if(addr >= 0x0100 && addr < 0x0200) {
    // Stack operations
    stack[addr - 0x100] = byte; // Write to stack
    return;
  }
  return; // Placeholder for write function
}

//-----------------------------------------------------------------------------
void app_main(void)
{
  reset6502(); // Initialize the 6502 CPU state

  idisplay_init(); // Initialize the display
  while(!break_flag) {
    step6502(); // Execute a single instruction
    printf("PC: %04X, A: %02X, X: %02X, Y: %02X, SP: %02X, P: %02X\n",
           pc, a, x, y, sp, status);
    vTaskDelay(pdMS_TO_TICKS(500)); // Adjust delay as needed
  }
}

