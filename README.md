# ESP32, LED Matrix Display with WiFi Control and Game of Life

## Overview
This project implements an interactive LED matrix display using ESP32 and MD_MAX72XX LED matrix modules. It combines scrolling text messages received via WiFi with Conway's Game of Life animations.

## Hardware Setup
- ESP32 microcontroller
- MD_MAX72XX LED matrix modules (4 devices in a chain)
- Connections:
  - VCC → 3.3V
  - GND → GND  
  - DIN → VSPI_MOSI (Pin 23)
  - CS → VSPI_SS (Pin 5)
  - CLK → VSPI_SCK (Pin 18)

## Key Components

### LED Display Control (`LedPanel.h/cpp`)
- Manages drawing on the LED matrix at pixel level
- Provides functions for:
  - Drawing points and lines
  - Special effects (spiral, wave, flash animations)
- Handles proper pixel mapping across multiple matrix modules

### Game of Life Implementation (`life.h/cpp`) 
- Classic cellular automaton simulation
- Features:
  - Configurable board size and wrap-around
  - Pattern detection for static/oscillating states
  - Pre-built patterns (gliders, blinkers, pulsar, glider gun)
  - Random board generation

### Main Program (`main.cpp`)
- Coordinates WiFi connectivity and display functionality
- Key features:
  - WiFi server for receiving text messages
  - Smooth text scrolling implementation
  - Automatic switching between text and Game of Life
  - Display effects between game iterations

## Core Functionality
1. On startup:
   - Initializes LED matrix
   - Connects to WiFi network
   - Displays device IP address
   
2. During operation:
   - Accepts text messages via web interface
   - Scrolls messages across display
   - Runs Game of Life simulations between messages
   - Shows transition effects when patterns stabilize

## Web Interface
- Simple HTML page for sending messages
- Accessible via ESP32's IP address
- Messages are displayed as scrolling text before returning to Game of Life

## Technical Details
- Display: 32x8 LED matrix (4 MAX7219 modules)
- Refresh rate: ~75ms for text scrolling
- Game of Life update interval: ~333ms
- Maximum message length: 255 characters
