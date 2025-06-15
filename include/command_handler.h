//-----------------------------------------------------------------------------
// File: command_handler.h
// Last edit: 06/03/2025
//-----------------------------------------------------------------------------
#ifndef COMMAND_HANDLER_H
#define COMMAND_HANDLER_H

#include <stdint.h>

//-----------------------------------------------------------------------------
typedef enum{
    CMD_NONE = 0,
    CMD_RSP_ERROR,
    CMD_REQ_PING,
    CMD_RSP_PONG,
    CMD_LOG,
    CMD_WRITE_MEM,
    CMD_START_EMU,
    CMD_STOP_EMU,
    CMD_STEP_EMU,
} CMD_PACKET_TYPE_E;

//-----------------------------------------------------------------------------
void command_init();
void command_task();
void command_parse(uint8_t *msg_data, uint32_t package_size);

void serial_init();
void serial_task(void *pvParameters);
void serial_send_slip_byte(uint8_t data);
void serial_send_slip_bytes(uint8_t *data, uint32_t len);
void serial_send_slip_end();

#endif