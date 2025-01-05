# Smart Light Control System

This project is an implementation of a smart light control system using an ESP32 microcontroller. The system can operate in two modes: automatic and manual. It uses a light sensor to detect ambient light levels and control an LED accordingly.

## Features

- **Automatic Mode**: The LED turns on or off based on the ambient light level.
- **Manual Mode**: The LED can be toggled manually using a button.
- **MQTT Integration**: The system connects to an MQTT broker to receive mode change commands and publish status updates.

## Components

- **ESP32 Microcontroller**
- **Light Sensor** (connected to pin 36)
- **LED** (connected to pin 2)
- **Button** (connected to pin 4)

## Setup

1. **WiFi Credentials**: Update the `ssid` and `password` variables in `src/main.cpp` with your WiFi network credentials.
2. **MQTT Broker**: Update the `mqtt_server`, `mqtt_port`, and `mqtt_topic` variables in `src/main.cpp` with your MQTT broker details.

## Installation

1. Install [PlatformIO](https://platformio.org/) in your preferred IDE (e.g., Visual Studio Code).
2. Clone this repository.
3. Open the project folder in your IDE.
4. Build and upload the firmware to your ESP32 board.

## Usage

- **Automatic Mode**: The system will automatically control the LED based on the ambient light level.
- **Manual Mode**: Press the button to toggle the LED.
- **MQTT Commands**:
  - Send "AUTO" to switch to automatic mode.
  - Send "MANUAL" to switch to manual mode.
  - Send "TOGGLE" to toggle the LED in manual mode.

## File Structure

- `src/main.cpp`: Main source code file.
- `platformio.ini`: PlatformIO project configuration file.
- `lib/`: Directory for project-specific libraries.
- `include/`: Directory for project header files.
- `test/`: Directory for unit tests.

## License

This project is licensed under the MIT License.