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
#include "fakemem.h"
#include "info_display.h"
#include "command_handler.h"
#include "p_slip.h"

//-----------------------------------------------------------------------------
const uint16_t EXEC_START = 0x8000;
uint8_t fake6502_running_status;

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
      *fake6502_status = FLAG_CONSTANT;
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
    char text[32];
    sprintf(text, "6502 CPU Speed: %dKips\n", (int)((instructions - old_instructions)/1000));
    serial_send_slip_byte(CMD_LOG);
    serial_send_slip_bytes((uint8_t *)text, strlen(text)); // Send log message
    serial_send_slip_end();
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
    0x00, // BRK (End of program)
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
  // Initialize Everything
  serial_init(); // Initialize serial communication
  command_init(); // Initialize command handler
  io_init(); // Initialize IO for buttons and LEDs
  fakemem_init(EXEC_START); // Initialize fake memory

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
  xTaskCreatePinnedToCore(
    (TaskFunction_t)serial_task, // Task function
    "serial_task", // Task name
    2048, // Stack size
    NULL, // Task parameters
    3, // Priority
    NULL, // Task handle
    1 // Core ID (0 for core 0)
  );

  // Reset the 6502 CPU before starting execution
  reset6502(); 
  while(1) {
    if(fake6502_running_status == 1) {
      vTaskDelay(1); 
      continue; // Skip execution if break flag is set
    }
    if(fake6502_running_status == 2)
    {
      fake6502_running_status = 1;
    }
    step6502();
    if(instructions % 50000 == 0) {
      // Reset the watchdog timer every 50,000 instructions
      vTaskDelay(1); 
    }
  }
}

