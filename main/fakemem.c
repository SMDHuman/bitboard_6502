//-----------------------------------------------------------------------------
// fakemem.c
// 15.06.2025 github.com/SMDHuman
//-----------------------------------------------------------------------------

#include "fakemem.h"
#include <string.h>

//-----------------------------------------------------------------------------
fakemem_callable_t fakemem_callables[];
uint8_t fakemem[1<<16]; // Simulated memory for the 6502 CPU

uint16_t fakemem_access_address;
uint8_t fakemem_access_data;
uint8_t fakemem_access_mode;

//-----------------------------------------------------------------------------
// Initialize the 6502 Memory
void fakemem_init(uint16_t reset_vector){
  memset(fakemem, 0, sizeof(fakemem)); // Initialize fake memory
  fakemem[0xFFFC] = reset_vector & 0xFF; // Set reset vector low byte
  fakemem[0xFFFD] = (reset_vector >> 8) & 0xFF; // Set reset vector high byt
}
//-----------------------------------------------------------------------------
uint8_t read6502(uint16_t addr){
  // Debugging output
  fakemem_access_mode = 1;
  uint8_t return_data = fakemem[addr]; // Default return data from memory
  fakemem_access_address = addr; // Update display with memory address being read
  // Handle callable memory reads
  
  if((addr & 0xff00)  == 0x7f00){
    if(fakemem_callables[addr & 0xFF].read != 0){

      return_data = fakemem_callables[addr & 0xFF].read(addr);
      fakemem_access_data = return_data; // Update display with memory data read  
      return return_data; // Placeholder for read function
    }
  }
  // Handle hardware interrupt vectors
  switch(addr) {
    case 0xFFFE: // IRQ/BRK vector (low byte)
    case 0xFFFF: // IRQ/BRK vector (high byte)
      //break_flag = 1; // Set break flag when IRQ/BRK vector is read
      return_data = 0x00;
      break;
  }
  fakemem_access_data = return_data; // Update display with memory data read  
  return return_data; // Placeholder for read function
}
//-----------------------------------------------------------------------------
void write6502(uint16_t addr, uint8_t byte)
{
  // Handle callable memory reads
  if((addr & 0xff00)  == 0x7F00){
    if(fakemem_callables[addr & 0xFF].write != 0){
      fakemem_callables[addr & 0xFF].write(addr, byte);
    }
  }
  fakemem_access_mode = 2;
  fakemem_access_address = addr; // Update display with memory address being written
  fakemem_access_data = byte; // Update display with memory data written
  fakemem[addr] = byte; // Write to fake memory  
}
//-----------------------------------------------------------------------------
void fakemem_set_callable_read(uint16_t address, uint8_t (*read)(uint16_t)){
  fakemem_callables[address].read = read;
}
//-----------------------------------------------------------------------------
void fakemem_set_callable_write(uint16_t address, void (*write)(uint16_t, uint8_t)){
  fakemem_callables[address].write = write;
}
//-----------------------------------------------------------------------------
void fakemem_set_callable_read_block(uint16_t address, uint8_t size, uint8_t (*read)(uint16_t)){
  for(int i = 0; i < size; i++){
    fakemem_set_callable_read(address + i, read);
  }
}
//-----------------------------------------------------------------------------
void fakemem_set_callable_write_block(uint16_t address, uint8_t size, void (*write)(uint16_t, uint8_t)){
  for(int i = 0; i < size; i++){
    fakemem_set_callable_write(address + i, write);
  }
}
