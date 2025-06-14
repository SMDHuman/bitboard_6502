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
    CMD_RSP_ESPNET_ERROR,
    CMD_REQ_PING,
    CMD_RSP_PONG
} CMD_PACKET_TYPE_E;

//-----------------------------------------------------------------------------
void command_init();
void command_task();
void command_parse(uint8_t *msg_data, uint32_t package_size);

#endif