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
// Flip the pins later for port a
#define PORTA_1 GPIO_NUM_18
#define PORTA_2 GPIO_NUM_17
#define PORTA_3  GPIO_NUM_16
#define PORTA_4  GPIO_NUM_15
#define PORTA_5 GPIO_NUM_7
#define PORTA_6 GPIO_NUM_6
#define PORTA_7 GPIO_NUM_5
#define PORTA_8 GPIO_NUM_4

#define PORTB_1 GPIO_NUM_8
#define PORTB_2 GPIO_NUM_3
#define PORTB_3 GPIO_NUM_46
#define PORTB_4 GPIO_NUM_21
#define PORTB_5 GPIO_NUM_47
#define PORTB_6 GPIO_NUM_48
#define PORTB_7 GPIO_NUM_35
#define PORTB_8 GPIO_NUM_36
#define PORTC_1 GPIO_NUM_37
#define PORTC_2 GPIO_NUM_38
#define PORTC_3 GPIO_NUM_39
#define PORTC_4 GPIO_NUM_40
#define PORTC_5 GPIO_NUM_41
#define PORTC_6 GPIO_NUM_42
#define PORTC_7 GPIO_NUM_2
#define PORTC_8 GPIO_NUM_1


uint64_t porta_gpio_nums[] = {
    PORTA_1, PORTA_2, PORTA_3, PORTA_4,
    PORTA_5, PORTA_6, PORTA_7, PORTA_8
};
uint64_t portb_gpio_nums[] = {
    PORTB_1, PORTB_2, PORTB_3, PORTB_4,
    PORTB_5, PORTB_6, PORTB_7, PORTB_8
};
uint64_t portc_gpio_nums[] = {
    PORTC_1, PORTC_2, PORTC_3, PORTC_4,
    PORTC_5, PORTC_6, PORTC_7, PORTC_8
};

//-----------------------------------------------------------------------------
// Memory map
// 0x0000 - 0x00FF: Zero Page
// 0x0100 - 0x01FF: Stack
// 0x7F00 - 0x7FFF: Callable Memory (Custom emulator functions)
// 0x8000 - 0xFFFF: ROM

// Custom Emulator Functions
// 0x00 : -w : led
// 0x01 : -w : Delay 
// 0x02 : rw : io_porta_direction
// 0x03 : rw : io_portb_direction
// 0x04 : rw : io_portc_direction
// 0x05 : rw : io_porta_values
// 0x06 : rw : io_portb_values
// 0x07 : rw : io_portc_values

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

void io_porta_write_direction(uint16_t addr, uint8_t byte) {
  uint64_t porta_gpio_out_mask = 0;
  uint64_t porta_gpio_in_mask = 0;
  // Log the command
  serial_send_slip_byte(CMD_LOG);
  char log_msg[64];
  sprintf(log_msg, "Setting PORTA direction: %02X", byte);
  serial_send_slip_bytes((uint8_t *)log_msg, strlen(log_msg)); //
  serial_send_slip_end();

  // Create a mask for PORTA GPIOs
  for(int i = 0; i < 8; i++) {
    if(byte & (1 << i)) {
      porta_gpio_out_mask |= (1ULL << porta_gpio_nums[i]); // Set bit for PORTA GPIOs
    }else{
      porta_gpio_in_mask |= (1ULL << porta_gpio_nums[i]); // Clear bit for PORTA GPIOs
    }
  }

  //log the direction setting
  serial_send_slip_byte(CMD_LOG);
  sprintf(log_msg, "PORTA GPIOs direction mask: %016llX", porta_gpio_out_mask);
  serial_send_slip_bytes((uint8_t *)log_msg, strlen(log_msg)); //
  serial_send_slip_end();
  // Set direction for PORTA GPIOs
  gpio_config_t in_config = {
    .pin_bit_mask = porta_gpio_in_mask,
    .mode = GPIO_MODE_INPUT,
    .pull_up_en = GPIO_PULLUP_DISABLE,
    .pull_down_en = GPIO_PULLUP_DISABLE,
    .intr_type = GPIO_INTR_DISABLE
  };
  gpio_config_t out_config = {
    .pin_bit_mask = porta_gpio_out_mask,
    .mode = GPIO_MODE_OUTPUT,
    .pull_up_en = GPIO_PULLUP_DISABLE,
    .pull_down_en = GPIO_PULLDOWN_DISABLE,
    .intr_type = GPIO_INTR_DISABLE
  };
  gpio_config(&in_config); // Configure PORTA GPIOs as input
  gpio_config(&out_config); // Configure PORTA GPIOs as output
}
//-----------------------------------------------------------------------------
void io_portb_write_direction(uint16_t addr, uint8_t byte) {
  uint64_t portb_gpio_out_mask = 0;
  uint64_t portb_gpio_in_mask = 0;
  // Create a mask for PORTB GPIOs
  for(int i = 0; i < 8; i++) {
    if(byte & (1 << i)) {
      portb_gpio_out_mask |= (1ULL << portb_gpio_nums[i]); // Set bit for PORTB GPIOs
    }else{
      portb_gpio_in_mask |= (1ULL << portb_gpio_nums[i]); // Clear bit for PORTB GPIOs
    }
  }
  // Set direction for PORTB GPIOs
  gpio_config_t in_config = {
    .pin_bit_mask = portb_gpio_in_mask,
    .mode = GPIO_MODE_INPUT,
    .pull_up_en = GPIO_PULLUP_DISABLE,
    .pull_down_en = GPIO_PULLDOWN_ENABLE,
    .intr_type = GPIO_INTR_DISABLE
  };
  gpio_config_t out_config = {
    .pin_bit_mask = portb_gpio_out_mask,
    .mode = GPIO_MODE_OUTPUT,
    .pull_up_en = GPIO_PULLUP_DISABLE,
    .pull_down_en = GPIO_PULLDOWN_DISABLE,
    .intr_type = GPIO_INTR_DISABLE
  };
  gpio_config(&in_config); // Configure PORTB GPIOs as input
  gpio_config(&out_config); // Configure PORTB GPIOs as output
}
//-----------------------------------------------------------------------------
void io_portc_write_direction(uint16_t addr, uint8_t byte) {
  uint64_t portc_gpio_out_mask = 0;
  uint64_t portc_gpio_in_mask = 0;
  // Create a mask for PORTC GPIOs
  for(int i = 0; i < 8; i++) {
    if(byte & (1 << i)) {
      portc_gpio_out_mask |= (1ULL << portc_gpio_nums[i]); // Set bit for PORTC GPIOs
    }else{
      portc_gpio_in_mask |= (1ULL << portc_gpio_nums[i]); // Clear bit for PORTC GPIOs
    }
  }
  // Set direction for PORTC GPIOs
  gpio_config_t in_config = {
    .pin_bit_mask = portc_gpio_in_mask,
    .mode = GPIO_MODE_INPUT,
    .pull_up_en = GPIO_PULLUP_DISABLE,
    .pull_down_en = GPIO_PULLDOWN_ENABLE,
    .intr_type = GPIO_INTR_DISABLE
  };
  gpio_config_t out_config = {
    .pin_bit_mask = portc_gpio_out_mask,
    .mode = GPIO_MODE_OUTPUT,
    .pull_up_en = GPIO_PULLUP_DISABLE,
    .pull_down_en = GPIO_PULLDOWN_DISABLE,
    .intr_type = GPIO_INTR_DISABLE
  };
  gpio_config(&in_config); // Configure PORTC GPIOs as input
  gpio_config(&out_config); // Configure PORTC GPIOs as output
}
//-----------------------------------------------------------------------------
uint8_t io_porta_read_values(uint16_t addr) {
  uint8_t value = 0;
  // Read values from PORTA GPIOs
  for(int i = 0; i < 8; i++) {
    if(gpio_get_level(porta_gpio_nums[i])) {
      value |= (1 << i); // Set bit if GPIO is high
    } else {
      value &= ~(1 << i); // Clear bit if GPIO is low
    }
  }
  return value; // Return the read value
}
void io_porta_write_values(uint16_t addr, uint8_t byte) {
  // Write values to PORTA GPIOs
  for(int i = 0; i < 8; i++) {
    if(byte & (1 << i)) {
      gpio_set_level(porta_gpio_nums[i], 1); // Set GPIO high
    } else {
      gpio_set_level(porta_gpio_nums[i], 0); // Set GPIO low
    }
  }
}
uint8_t io_portb_read_values(uint16_t addr) {
  uint8_t value = 0;
  // Read values from PORTB GPIOs
  for(int i = 0; i < 8; i++) {
    if(gpio_get_level(portb_gpio_nums[i])) {
      value |= (1 << i); // Set bit if GPIO is high
    } else {
      value &= ~(1 << i); // Clear bit if GPIO is low
    }
  }
  return value; // Return the read value
}
void io_portb_write_values(uint16_t addr, uint8_t byte) {
  // Write values to PORTB GPIOs
  for(int i = 0; i < 8; i++) {
    if(byte & (1 << i)) {
      gpio_set_level(portb_gpio_nums[i], 1); // Set GPIO high
    } else {
      gpio_set_level(portb_gpio_nums[i], 0); // Set GPIO low
    }
  }
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
  fakemem_set_callable_write(2, &io_porta_write_direction);
  fakemem_set_callable_write(3, &io_portb_write_direction);
  fakemem_set_callable_write(4, &io_portc_write_direction);
  fakemem_set_callable_read(5, &io_porta_read_values);
  fakemem_set_callable_write(5, &io_porta_write_values);
  fakemem_set_callable_read(6, &io_portb_read_values);
  fakemem_set_callable_write(6, &io_portb_write_values);
  //fakemem_set_callable_read(7, &io_portc_read_values);
  //fakemem_set_callable_write(7, &io_portc_write_values);
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

