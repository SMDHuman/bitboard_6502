#include <stdio.h>
#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_err.h"
#include "driver/gpio.h"
#include "esp_task_wdt.h"

#include "fake6502.h"
#include "info_display.h"
#include "command_handler.h"

//-----------------------------------------------------------------------------
uint8_t break_flag = 0; // Global variable to track the break flag
uint8_t fakemem[1<<16]; // Simulated memory for the 6502 CPU

// addresses between 0xFE00 and 0xFEFF are reserved for callable memory
fakemem_callable_t fakemem_callable[256];


uint16_t fake6502_memaccess_address;
uint8_t fake6502_memaccess_data;
uint8_t fake6502_memaccess_mode;

const uint16_t EXEC_START = 0xE000;
//-----------------------------------------------------------------------------
uint8_t read6502(uint16_t addr){
  // Debugging output
  fake6502_memaccess_mode = 1;
  uint8_t return_data = fakemem[addr]; // Default return data from memory
  fake6502_memaccess_address = addr; // Update display with memory address being read
  // Handle callable memory reads
  
  if((addr & 0xff00)  == 0xFE00){
    if(fakemem_callable[addr & 0xFF].read != 0){

      return_data = fakemem_callable[addr & 0xFF].read(addr);
      fake6502_memaccess_data = return_data; // Update display with memory data read  
      return return_data; // Placeholder for read function
    }
  }
  // Handle hardware interrupt vectors
  switch(addr) {
    case 0xFFFE: // IRQ/BRK vector (low byte)
    case 0xFFFF: // IRQ/BRK vector (high byte)
      break_flag = 1; // Set break flag when IRQ/BRK vector is read
      return_data = 0x00;
      break;
  }
  fake6502_memaccess_data = return_data; // Update display with memory data read  
  return return_data; // Placeholder for read function
}
//-----------------------------------------------------------------------------
void write6502(uint16_t addr, uint8_t byte)
{
  // Handle callable memory reads
  if((addr & 0xff00)  == 0xFE00){
    if(fakemem_callable[addr & 0xFF].write != 0){
      fakemem_callable[addr & 0xFF].write(addr, byte);
    }
  }
  fake6502_memaccess_mode = 2;
  fake6502_memaccess_address = addr; // Update display with memory address being written
  fake6502_memaccess_data = byte; // Update display with memory data written
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
void io_init(){
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
}
//-----------------------------------------------------------------------------
void io_task(void *pvParameters) {
  while(1) {
    // Check if GPIO 0 button is pressed
    if(!gpio_get_level(GPIO_NUM_0)) {
      //printf("Resetting 6502 CPU...\n");
      reset6502(); // Reset the CPU state
      clearinterrupt(); // Clear the interrupt flag
      break_flag = 0; // Clear the break flag
      while(!gpio_get_level(GPIO_NUM_0)){
        vTaskDelay(pdMS_TO_TICKS(250)); 
      }
    }
    vTaskDelay(pdMS_TO_TICKS(150)); // Adjust delay as needed
  }
}

//-----------------------------------------------------------------------------
void io_write(uint16_t addr, uint8_t byte) {
  // Handle IO write operations
  if(byte == 0x01) {
    gpio_set_level(GPIO_NUM_45, 1); // Set GPIO 45 high
  } else if(byte == 0x00) {
    gpio_set_level(GPIO_NUM_45, 0); // Set GPIO 45 low
  }
}
void delay_write(uint16_t addr, uint8_t byte){
  vTaskDelay(pdMS_TO_TICKS(byte));
}
//-----------------------------------------------------------------------------
void log_perf_task(void *pvParameters) {
  static uint32_t old_instructions = 0;
  while(1) {
    // Log performance metrics every second
    printf("6502 CPU Speed: %dKips\n", (int)((instructions - old_instructions)/1000));
    old_instructions = instructions; // Update old instruction count
    vTaskDelay(pdMS_TO_TICKS(1000)); // Log every second
  }
}
//-----------------------------------------------------------------------------
const uint8_t program[] = {
    0xA9, 0x01, // LDA #$01
    0x85, 0x00, // STA $00
    0xA9, 0x02, // LDA #$02
    0x85, 0x01, // STA $01
    0xA9, 0x03, // LDA #$03
    0x85, 0x02, // STA $02
    0xA9, 0x04, // LDA #$04
    0x85, 0x03, // STA $03
    // jump to start
    0x4C, 0x00, 0xE0, 
    // write 1 to fe00 to turn on LED
    0xA9, 0x01, // LDA #$01
    0x8D, 0x00, 0xFE, // STA $FE00
    // 5*200ms  delay
    0xA9, 0x8C, // LDA #$01
    0x8D, 0x01, 0xFE, // STA $FE01
    0x8D, 0x01, 0xFE, // STA $FE01
    0x8D, 0x01, 0xFE, // STA $FE01
    0x8D, 0x01, 0xFE, // STA $FE01
    0x8D, 0x01, 0xFE, // STA $FE01
    // Turn off LED
    0xA9, 0x00, // LDA #$00
    0x8D, 0x00, 0xFE, // STA $FE00
    // 5*200ms  delay
    0xA9, 0x8C, // LDA #$01
    0x8D, 0x01, 0xFE, // STA $FE01
    0x8D, 0x01, 0xFE, // STA $FE01
    0x8D, 0x01, 0xFE, // STA $FE01
    0x8D, 0x01, 0xFE, // STA $FE01
    0x8D, 0x01, 0xFE, // STA $FE01
    
    0x4C, 0x0C, 0xE0, // JMP $E00C (jump back to the LED on instruction)
    
    0x00        // BRK (End of program)
  };
//-----------------------------------------------------------------------------
void app_main(void)
{
  io_init(); // Initialize IO for buttons and LEDs
  fakemem_init(); // Initialize fake memory 
  //  Set up callable memory for IO operations
  fakemem_set_callable_write(0, &io_write);
  fakemem_set_callable_write(1, &delay_write);
  // Load Default program into memory
  memcpy(&fakemem[EXEC_START], program, sizeof(program)); 
  //printf("Program loaded into memory at address %04X\n", EXEC_START);
  idisplay_init(); // Initialize the display 

  // Create tasks
  xTaskCreatePinnedToCore(
    (TaskFunction_t)idisplay_task, // Task function
    "idisplay_task", // Task name
    4096, // Stack size
    NULL, // Task parameters
    1, // Priority
    NULL, // Task handle
    1 // Core ID (0 for core 0)
  );
  xTaskCreatePinnedToCore(
    (TaskFunction_t)io_task, // Task function
    "io_task", // Task name
    2048, // Stack size
    NULL, // Task parameters
    1, // Priority
    NULL, // Task handle
    1 // Core ID (0 for core 0)
  );
  xTaskCreatePinnedToCore(
    (TaskFunction_t)log_perf_task, // Task function
    "log_perf_task", // Task name
    2048, // Stack size
    NULL, // Task parameters
    2, // Priority
    NULL, // Task handle
    1 // Core ID (0 for core 0)
  );

  // Reset the 6502 CPU before starting execution
  reset6502(); 
  while(1) {
    if(break_flag){
      vTaskDelay(1); 
      continue; // Skip execution if break flag is set
    }
    step6502();
    if(instructions % 50000 == 0) {
      // Reset the watchdog timer every 50,000 instructions
      vTaskDelay(1); 
    }
  }
}

