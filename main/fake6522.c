//-----------------------------------------------------------------------------
// fake6522.c
// 17.06.2025 Github.com/SMDHuman
//-----------------------------------------------------------------------------
#include "fake6522.h"

#include <stdint.h>
#include <stddef.h>
#include "driver/gpio.h"

#include "fake6502.h"
#include "fakemem.h"

//-----------------------------------------------------------------------------
static const uint64_t porta_gpio_nums[] = {
    PORTA_1, PORTA_2, PORTA_3, PORTA_4,
    PORTA_5, PORTA_6, PORTA_7, PORTA_8
};
static const uint64_t portb_gpio_nums[] = {
    PORTB_1, PORTB_2, PORTB_3, PORTB_4,
    PORTB_5, PORTB_6, PORTB_7, PORTB_8
};

//-----------------------------------------------------------------------------
static void io_port_write_direction(uint16_t addr, uint8_t byte, uint64_t *port_gpio_nums){
  uint64_t port_gpio_out_mask = 0;
  uint64_t port_gpio_in_mask = 0;
  // Create a mask for PORTA GPIOs
  for(int i = 0; i < 8; i++) {
    if(byte & (1 << i)) {
      port_gpio_out_mask |= (1ULL << port_gpio_nums[i]); // Set bit for PORTA GPIOs
    }else{
      port_gpio_in_mask |= (1ULL << port_gpio_nums[i]); // Clear bit for PORTA GPIOs
    }
  }
  // Set direction for PORTA GPIOs
  gpio_config_t in_config = {
    .pin_bit_mask = port_gpio_in_mask,
    .mode = GPIO_MODE_INPUT,
    .pull_up_en = GPIO_PULLUP_DISABLE,
    .pull_down_en = GPIO_PULLUP_DISABLE,
    .intr_type = GPIO_INTR_DISABLE
  };
  gpio_config_t out_config = {
    .pin_bit_mask = port_gpio_out_mask,
    .mode = GPIO_MODE_OUTPUT,
    .pull_up_en = GPIO_PULLUP_DISABLE,
    .pull_down_en = GPIO_PULLDOWN_DISABLE,
    .intr_type = GPIO_INTR_DISABLE
  };
  gpio_config(&in_config); // Configure PORTA GPIOs as input
  gpio_config(&out_config); // Configure PORTA GPIOs as output
}
//-----------------------------------------------------------------------------
static void io_port_write_values(uint16_t addr, uint8_t byte, uint64_t *port_gpio_nums) {
  // Write values to PORT GPIOs
  for(int i = 0; i < 8; i++) {
    if(byte & (1 << i)) {
      gpio_set_level(port_gpio_nums[i], 1); // Set GPIO high
    } else {
      gpio_set_level(port_gpio_nums[i], 0); // Set GPIO low
    }
  }
}
//-----------------------------------------------------------------------------
static uint8_t io_port_read_values(uint16_t addr, uint64_t *port_gpio_nums) {
  uint8_t value = 0;
  // Read values from PORTA GPIOs
  for(int i = 0; i < 8; i++) {
    if(gpio_get_level(port_gpio_nums[i])) {
      value |= (1 << i); // Set bit if GPIO is high
    } else {
      value &= ~(1 << i); // Clear bit if GPIO is low
    }
  }
  return value; // Return the read value
}
//-----------------------------------------------------------------------------
void fake6522_write(uint16_t addr, uint8_t byte) {
  uint8_t rs =  addr & 0xFF; // Get the register select bits from the address
  switch(rs) {
    case 0x00: // PORTB Value Register
      io_port_write_values(addr, byte, portb_gpio_nums); // Write to PORTB values
      break;
    case 0x01: // PORTA Value Register
      io_port_write_values(addr, byte, porta_gpio_nums); // Write to PORTA values
      break;
    case 0x02: // PORTB Direction Register
      io_port_write_direction(addr, byte, portb_gpio_nums); // Write to PORTB direction
      break;
    case 0x03: // PORTA Direction Register
      io_port_write_direction(addr, byte, porta_gpio_nums); // Write to PORTA direction
      break;
    default:
      // Handle other registers if needed
      break;
  }
}
//-----------------------------------------------------------------------------
uint8_t fake6522_read(uint16_t addr) {
  uint8_t rs =  addr & 0xFF; // Get the register select bits from the address
  switch(rs) {
    case 0x00: // PORTB Value Register
      return io_port_read_values(addr, portb_gpio_nums); // Read from PORTB values
    case 0x01: // PORTA Value Register
      return io_port_read_values(addr, porta_gpio_nums); // Read from PORTA values
    default:
      return 0; // Handle other registers if needed
  }
}

