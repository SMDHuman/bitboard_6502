#include <stdio.h>
#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_err.h"

#include "MCS6502.h"
#include "info_display.h"

//-----------------------------------------------------------------------------
uint8 break_flag = 0; // Global variable to track the break flag
uint8 stack[256]; // Simulated stack 

MCS6502ExecutionContext mcs6502;
static const uint16 EXEC_START = 0x0600;

static const uint8 program[] = {
  0xA9, 0x01, // LDA #$01
  0x48,        // PHA (push accumulator to stack)
  0xA9, 0x04, // LDA #$04
  0xA2, 0x02, // LDX #$02
  0xA0, 0x03, // LDY #$03
  0x68,        // PLA (pull accumulator from stack)
  0x00        // BRK (end of program)
};

//-----------------------------------------------------------------------------
uint8 MCS6502_Read(uint16 addr, void * readWriteContext){
  // Debugging output
  printf("MCS6502_Read called with addr: %04X\n", addr);

  // Handle hardware interrupt vectors
  if(addr == MCS6502_NMI_LO){
    return 0x00; // Return low byte of NMI vector (not used in this example)
  }
  else if(addr == MCS6502_NMI_HI){
    return 0x00; // Return high byte of NMI vector (not used in this example)
  }
  else if(addr == MCS6502_IRQ_BRK_LO){
    break_flag = 1; // Set the break flag when IRQ/BRK is read
    return 0x00; // Return low byte of IRQ/BRK vector (not used in this example)
  }
  else if(addr == MCS6502_IRQ_BRK_HI){
    break_flag = 1; // Set the break flag when IRQ/BRK is read
    return 0x00; // Return high byte of IRQ/BRK vector (not used in this example)
  }
  else if(addr == (MCS6502_RESET_LO)){
    return EXEC_START & 0xFF; // Return low byte of reset vector
  }
  else if(addr == (MCS6502_RESET_HI)){
    return (EXEC_START >> 8) & 0xFF; // Return high byte of reset vector
  }
  // Handle stack operations
  else if(addr >= 0x0100 && addr < 0x0200) {
    // Stack operations
    return stack[addr - 0x100]; // Read from stack
  }

  // Handle program memory read
  if(addr >= EXEC_START && addr < EXEC_START + sizeof(program)) {
    return program[addr - EXEC_START]; // Return program byte
  }

  return 0; // Placeholder for read function
}
//-----------------------------------------------------------------------------
void MCS6502_Write(uint16 addr, uint8 byte, void * readWriteContext)
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
  
  MCS6502Init(&mcs6502, MCS6502_Read, MCS6502_Write, NULL);
  MCS6502Reset(&mcs6502);

  idisplay_init(&mcs6502); // Initialize the display

  while(!break_flag) {
    MCS6502Tick(&mcs6502);
    printf("PC: %04X, A: %02X, X: %02X, Y: %02X, SP: %02X, P: %02X\n",
           mcs6502.pc, mcs6502.a, mcs6502.x, mcs6502.y, mcs6502.sp, mcs6502.p);
    vTaskDelay(pdMS_TO_TICKS(500)); // Adjust delay as needed
  }
}

