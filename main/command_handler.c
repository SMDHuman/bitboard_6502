//-----------------------------------------------------------------------------
// File: command_handler.cpp
// Last modified: 29/03/2025
//-----------------------------------------------------------------------------
#include "command_handler.h"
#include "esp_log.h"
#include "esp_err.h"

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
  esp_err_t res;
  switch(cmd){
    case CMD_REQ_PING:
    {
      //serial_send_slip_byte(CMD_RSP_PONG);
      //serial_end_slip();
    }break;
  } 
}
