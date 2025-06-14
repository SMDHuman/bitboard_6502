# Bitboard Bir 6502 Emulator

This project aims to create a simple 6502 emulator running on a microcontroller, with a display to visualize CPU registers and variables. I utilized my custom board called "Bitboard Bir" for this implementation.

## About Bitboard Bir 
<img src="/docs/Bitboard_Graphics1.jpg" width="200" title="Bitboard Graphics" style="float: right;">

Bitboard Bir is a custom ESP32-S3 based development board I designed. It features: 
- 1.3 inch ST7789 display (on-board)
- 24 GPIO pins
- CP210x USB interface

While this board is no longer in active development, it serves perfectly for this project.


## Project Goals

- Implement a functional 6502 emulator on ESP32-S3
- Create a visual interface for CPU state monitoring
- Learn ESP-IDF development framework
- Improve my 6502 assembly programming skills

This implementation may not perfectly match your specific needs, but I hope you can find useful components to adapt for your own projects.

## Installation

### Prerequisites
- ESP-IDF v5.0 or later
- A compatible ESP32-S3 board (or modify the display driver for your hardware)
- Basic understanding of CMake build systems

### Setup
1. Clone this repository:
  ```bash
  git clone https://github.com/yourusername/bitboard_6502.git
  cd bitboard_6502
  ```

2. Configure the project:
  ```bash
  idf.py set-target esp32s3
  idf.py menuconfig
  ```

3. Build and flash:
  ```bash
  idf.py build
  idf.py -p PORT flash
  ```

## Features
- Full MOS 6502 CPU emulation
- Real-time register visualization
- Memory inspector
- Basic debugger functionality

## Flashing Assembly to Board
This feature is still in development. Currently, there are no capabilities for configuration or uploading data over USB serial.