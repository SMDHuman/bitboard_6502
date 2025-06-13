#ifndef FAKE6502_H
#define FAKE6502_H

#include <stddef.h>
#include <stdint.h>


typedef struct {
  uint8_t (*read)(uint16_t address); // Function pointer for reading memory
  void (*write)(uint16_t address, uint8_t value); // Function pointer for writing memory
} fakemem_callable_t;
extern fakemem_callable_t fakemem_callable[];
void fakemem_set_callable_read(uint16_t address, uint8_t (*read)(uint16_t));
void fakemem_set_callable_write(uint16_t address, void (*write)(uint16_t, uint8_t));
void fakemem_set_callable_read_block(uint16_t address, uint8_t size, uint8_t (*read)(uint16_t));
void fakemem_set_callable_write_block(uint16_t address, uint8_t size, void (*write)(uint16_t, uint8_t));


//6502 defines
extern uint8_t fakemem[65536]; // Simulated memory for the 6502 CPU
extern uint16_t *fake6502_pc;
extern uint8_t *fake6502_sp; 
extern uint8_t *fake6502_a;
extern uint8_t *fake6502_x;
extern uint8_t *fake6502_y;
extern uint8_t *fake6502_status;
extern uint32_t instructions; 

extern uint16_t fake6502_memaccess_address;
extern uint8_t fake6502_memaccess_data;
extern uint8_t fake6502_memaccess_mode;

#define FLAG_CARRY     0x01
#define FLAG_ZERO      0x02
#define FLAG_INTERRUPT 0x04
#define FLAG_DECIMAL   0x08
#define FLAG_BREAK     0x10
#define FLAG_CONSTANT  0x20
#define FLAG_OVERFLOW  0x40
#define FLAG_SIGN      0x80

#define BASE_STACK     0x100

//flag modifier macros
#define setcarry() *fake6502_status |= FLAG_CARRY
#define clearcarry() *fake6502_status &= (~FLAG_CARRY)
#define setzero() *fake6502_status |= FLAG_ZERO
#define clearzero() *fake6502_status &= (~FLAG_ZERO)
#define setinterrupt() *fake6502_status |= FLAG_INTERRUPT
#define clearinterrupt() *fake6502_status &= (~FLAG_INTERRUPT)
#define setdecimal() *fake6502_status |= FLAG_DECIMAL
#define cleardecimal() *fake6502_status &= (~FLAG_DECIMAL)
#define setoverflow() *fake6502_status |= FLAG_OVERFLOW
#define clearoverflow() *fake6502_status &= (~FLAG_OVERFLOW)
#define setsign() *fake6502_status |= FLAG_SIGN
#define clearsign() *fake6502_status &= (~FLAG_SIGN)

void exec6502(uint32_t tickcount);
void step6502();
void hookexternal(void *funcptr);
void irq6502();
void nmi6502();
void reset6502();
void push16(uint16_t pushval);
void push8(uint8_t pushval);
uint16_t pull16();
uint8_t pull8();
//externally supplied functions
extern uint8_t read6502(uint16_t address);
extern void write6502(uint16_t address, uint8_t value);

#endif