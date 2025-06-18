#include <stdio.h>
#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_sntp.h"
#include "driver/gpio.h"

#include "fake6502.h"
#include "fakemem.h"
#include "info_display.h"
#include "command_handler.h"
#include "p_slip.h"

//-----------------------------------------------------------------------------

// Memory map
// 0x0000 - 0x00FF: Zero Page
// 0x0100 - 0x01FF: Stack
// 0x6000 - 0x6FFF: 6522 Peripheral (Fake 6522)
// 0x7000 - 0x70FF: Callable Memory (Custom emulator functions)
// 0x8000 - 0xFFFF: ROM

// Custom Emulator Functions
// 0x7000 : -w : led
// 0x7001 : -w : Delay 

//-----------------------------------------------------------------------------
const uint16_t EXEC_START = 0x8000;
uint8_t fake6502_running_status;

//-----------------------------------------------------------------------------
void io_init(){
  // Initialize the IO Buttons and Leds
  gpio_config_t config_input = {
    // GPIO 0 and 13 Buttons
    .pin_bit_mask = (1ULL << GPIO_NUM_0) | (1ULL << GPIO_NUM_13), 
    .mode = GPIO_MODE_INPUT,
    .pull_up_en = GPIO_PULLUP_DISABLE,
    .pull_down_en = GPIO_PULLDOWN_ENABLE,
    .intr_type = GPIO_INTR_DISABLE
  };
  // Configure GPIO 0 and 13 as input with pull-up enabled
  gpio_config(&config_input); 

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
    sprintf(text, "6502 CPU Speed: %dKips\n", 
                (int)((instructions - old_instructions)/1000));
    serial_send_slip_byte(CMD_LOG);
    serial_send_slip_bytes((uint8_t *)text, strlen(text)); // Send log message
    serial_send_slip_end();
    old_instructions = instructions; // Update old instruction count
    vTaskDelay(pdMS_TO_TICKS(1000)); // Log every second
  }
}
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
    (TaskFunction_t)serial_task, // Task function
    "serial_task", // Task name
    2048, // Stack size
    NULL, // Task parameters
    1, // Priority
    NULL, // Task handle
    1 // Core ID (0 for core 0)
  );

  // Reset the 6502 CPU before starting execution
  reset6502(); 
  // ----- MAIN LOOP -----
  static time_t last_vtask_delay = 0;
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
    time_t now;
    time(&now); // Get current time
    if(now - last_vtask_delay > 1000) {
      // Log performance every second
      time(&last_vtask_delay); // Get current time
      vTaskDelay(1); 
    }
  }
}
