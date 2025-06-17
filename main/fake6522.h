//-----------------------------------------------------------------------------
// fake6522.h
// 17.06.2025 Github.com/SMDHuman
//-----------------------------------------------------------------------------

#ifndef FAKE6522_H
#define FAKE6522_H

#include <stdint.h>
#include <stddef.h>
#include "driver/gpio.h"

//-----------------------------------------------------------------------------
// Flip the pins later for port a
#define PORTA_1 GPIO_NUM_18
#define PORTA_2 GPIO_NUM_17
#define PORTA_3 GPIO_NUM_16
#define PORTA_4 GPIO_NUM_15
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

//-----------------------------------------------------------------------------

void fake6522_write(uint16_t addr, uint8_t byte);
uint8_t fake6522_read(uint16_t addr);

//-----------------------------------------------------------------------------
#endif //  FAKE6522_H
