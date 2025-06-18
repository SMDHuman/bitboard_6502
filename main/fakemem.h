//-----------------------------------------------------------------------------
// fakemem.h
// 15.06.2025 github.com/SMDHuman
//-----------------------------------------------------------------------------
#ifndef FAKEMEM_H
#define FAKEMEM_H

#include <stddef.h>
#include <stdint.h>


typedef struct {
  uint8_t (*read)(uint16_t address); // Function pointer for reading memory
  void (*write)(uint16_t address, uint8_t value); // Function pointer for writing memory
} fakemem_callable_t;

// addresses between 0xF000 and 0xF0FF are reserved for callable memory
#define FAKEMEM_CALLABLE_START 0xF000
extern fakemem_callable_t fakemem_callables[];
extern uint8_t fakemem[]; // Simulated memory for the 6502 CPU

extern uint16_t fakemem_access_address;
extern uint8_t fakemem_access_data;
extern uint8_t fakemem_access_mode;

void fakemem_init(uint16_t reset_vector);
uint8_t read6502(uint16_t addr);
void write6502(uint16_t addr, uint8_t byte);
void fakemem_set_callable_read(uint16_t address, uint8_t (*read)(uint16_t));
void fakemem_set_callable_write(uint16_t address, void (*write)(uint16_t, uint8_t));
void fakemem_set_callable_read_block(uint16_t address, uint8_t size, uint8_t (*read)(uint16_t));
void fakemem_set_callable_write_block(uint16_t address, uint8_t size, void (*write)(uint16_t, uint8_t));

#endif