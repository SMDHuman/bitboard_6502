//-----------------------------------------------------------------------------
// File: command_handler.cpp
// Last modified: 29/03/2025
//-----------------------------------------------------------------------------
#include "command_handler.h"

#include <string.h>

#include "esp_log.h"
#include "esp_err.h"
#include "fake6502.h"
#include "fakemem.h"
#include "info_display.h"
#include "driver/uart.h"

#define SLIP_IMPLEMENTATION
#include "p_slip.h"

void serial_send_slip_byte(uint8_t data);

//-----------------------------------------------------------------------------
void command_init(){
}

//-----------------------------------------------------------------------------
void command_task(){
}

//-----------------------------------------------------------------------------
void command_parse(uint8_t *msg_data, uint32_t len){
  CMD_PACKET_TYPE_E cmd = (CMD_PACKET_TYPE_E)msg_data[0];
  uint8_t *data = msg_data + 1; 
  len -= 1;
  esp_err_t res = ESP_OK;
  switch(cmd){
    case CMD_NONE:
    case CMD_RSP_ERROR:
    case CMD_RSP_PONG:
    case CMD_LOG:
    case CMD_REQ_PING:
    break;
    case CMD_WRITE_MEM:
    {
      if(len < 3){
        res = ESP_ERR_INVALID_SIZE;
      } else {
        uint16_t addr = (data[0] | (data[1] << 8));
        memcpy(&fakemem[addr], data + 2, len - 2);
        res = ESP_OK;
        //serial_send_slip_byte(CMD_LOG);
        //uint8_t text[64];
        //sprintf((char *)text, "Wrote %d bytes to address %04X\n", (int)(len - 2), addr);
        //serial_send_slip_bytes(text, strlen((char *)text)); // Send log message
        //serial_send_slip_end();
      }
    }
    break;
    case CMD_START_EMU:
    {
      fake6502_running_status = 0; // Set running status to 1
    }break;
    case CMD_STOP_EMU:
    {
      fake6502_running_status = 1; // Set running status to 0
    }break;
    case CMD_STEP_EMU:
    {
      fake6502_running_status = 2; // Set running status to 2 for stepping
    }break;
    case CMD_GET_INST_COUNT:
    {
      uint32_t inst_count = instructions; // Get the current instruction count
      serial_send_slip_byte(CMD_GET_INST_COUNT); // Send log command
      serial_send_slip_bytes((uint8_t *)&inst_count, sizeof(inst_count)); // Send instruction count
      serial_send_slip_end(); // End the SLIP message
    }break;
    default:
      res = ESP_ERR_INVALID_ARG; // Invalid command
    break;
  } 
  if(res != ESP_OK){
    serial_send_slip_byte(CMD_RSP_ERROR);
  } else {
    serial_send_slip_byte(CMD_RSP_PONG);
  }
  serial_send_slip_end();
}
//-----------------------------------------------------------------------------
uint8_t serial_slip_buffer[1024]; // Buffer for SLIP data

void serial_init(){
  // Initialize UART for serial communication
  uart_config_t uart_config = {
    .baud_rate = 115200,
    .data_bits = UART_DATA_8_BITS,
    .parity = UART_PARITY_DISABLE,
    .stop_bits = UART_STOP_BITS_1,
    .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    .source_clk = UART_SCLK_DEFAULT
  };
  uart_param_config(UART_NUM_0, &uart_config); // Configure UART parameters
  uart_driver_install(UART_NUM_0, 1024 * 2, 0, 0, NULL, 0); // Install UART driver
  uart_set_mode(UART_NUM_0, UART_MODE_UART); // Set UART mode
  // Initialize SLIP buffer
  slip_init(serial_slip_buffer, sizeof(serial_slip_buffer), false); // Initialize SLIP buffer
}
void serial_task(void *pvParameters){
  static uint8_t serial_buffer[1024]; // Buffer for SLIP data
  while(1){
    
    int len = uart_read_bytes(UART_NUM_0, serial_buffer, sizeof(serial_buffer), 100 / portTICK_PERIOD_MS);

    if(len > 0){
      // Process the received SLIP data
      slip_push_all(serial_slip_buffer, serial_buffer, len);
      if(slip_is_ready(serial_slip_buffer)){
        idsplay_blink_led(100); // Blink LED to indicate command received
        uint32_t size = slip_get_size(serial_slip_buffer);
        uint8_t *data = slip_get_buffer(serial_slip_buffer);
        command_parse(data, size); // Parse the command
        slip_reset(serial_slip_buffer); // Reset the SLIP buffer after processing
      }

    }
    vTaskDelay(1); // Adjust delay as needed
  }
  
}
//-----------------------------------------------------------------------------
void serial_send_slip_byte(uint8_t data){
  static uint8_t slip_buffer[2]; 
  static uint8_t buffer_size = 0; 
  if(data == S_END || data == S_ESC){
    slip_buffer[0] = S_ESC; // Escape character
    slip_buffer[1] = (data == S_END) ? S_ESC_END : S_ESC_ESC; // Escaped character
    buffer_size = 2; // Set buffer size to 2 for escaped characters
  }
  else {
    slip_buffer[0] = data; // Normal character
    buffer_size = 1;
  }
  uart_write_bytes(UART_NUM_0, slip_buffer, buffer_size);
}

//-----------------------------------------------------------------------------
void serial_send_slip_bytes(uint8_t *data, uint32_t len){
  for(uint32_t i = 0; i < len; i++){
    serial_send_slip_byte(data[i]);
  }
}

void serial_send_slip_end(){
  uint8_t end_char = S_END; // SLIP end character
  uart_write_bytes(UART_NUM_0, (const char *)&end_char, 1); // Send SLIP end character
}