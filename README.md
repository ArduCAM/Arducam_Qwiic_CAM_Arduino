# Arducam Qwiic CAM Arduino Library

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

Arduino library for Arducam Qwiic series camera modules, enabling rapid image capture and basic computer vision applications.

## Key Features
- Plug-and-play Qwiic I2C interface (no soldering required)
- Supports popular boards: Arduino UNO/Mega/Leonardo/ESP32 variants
- Programmable resolutions (96x96 to 1024x768)
- Programmable pixel format (RGB565 Y8 JPEG)
- Image buffering and real-time streaming capabilities

## Hardware Requirements
- Arducam Qwiic CAM module
- Arduino-compatible board (UNO R4 WiFi is recommended)
- Qwiic cable or 4-pin I2C connector

## Getting Started
### Basic Wiring

```plaintext
Arduino        Qwiic CAM
3.3V  ------   VCC
GND   ------   GND
SDA   ------   SDA
SCL   ------   SCL
```

### Usage
1. Arduino IDE → *Sketch* → *Include Library* → *Manage Libraries...*
2. Search for "Arducam_Qwiic"
3. Install the latest version
4. Examples available under *File* → *Examples* → *Arducam_Qwiic*
