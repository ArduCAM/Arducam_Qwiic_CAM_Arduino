# Arducam Qwiic CAM Arduino Library

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

Arduino library for Arducam Qwiic series camera modules. It provides image capture, camera parameter control, browser preview, and PC host-side image preview over the Qwiic/I2C interface.

This library includes two main example workflows:

- **WiFi Preview** — browser-based camera control and JPEG live preview
- **USART Host Protocol** — binary serial protocol for PC host software control and image display

## Features

- **Qwiic I2C interface** — plug-and-play connection through Qwiic/I2C
- **Multi-platform** — works with Arduino UNO R4 WiFi, Portenta C33, and other I2C-capable boards
- **Multiple resolutions** — 96×96, 128×128, 320×240, 320×320, 640×480, 1280×720, 1600×1200, 1920×1080, 2592×1944
- **Pixel formats** — JPEG, RGB565, Y8 grayscale
- **Camera controls** — quality, brightness, contrast, saturation, EV, white balance, color effects, sharpness
- **WiFi preview example** — browser-based live preview and camera parameter control
- **USART host protocol example** — PC host software control, image capture, and JPEG stream preview through USB serial
- **Low RAM transfer** — image data is read from the camera FIFO in small blocks and forwarded to WiFi or serial output

## Camera Specs

| Format | Resolutions | Type |
|--------|-------------|------|
| JPEG | 96×96, 128×128, 320×240, 320×320, 640×480, 1280×720, 1600×1200, 1920×1080, 2592×1944 | Color |
| RGB565 | 96×96, 128×128, 320×240 | Color |
| Y8 | 96×96, 128×128, 320×240 | Gray |

> Note: RGB565 and Y8 raw formats are recommended for small-resolution testing. For high-resolution preview or streaming, JPEG is recommended.


## Hardware Requirements

- Arducam Qwiic CAM module
- Arduino-compatible board, such as Arduino UNO R4 WiFi
- Qwiic cable or 4-pin I2C connector
- For WiFi preview: a phone, tablet, or PC with a browser
- For USART host demo: PC host software or a binary serial tool

## Wiring

Connect the Qwiic CAM to your board through a Qwiic cable:

| Arduino | Qwiic CAM |
|---------|-----------|
| 3.3V | VCC |
| GND | GND |
| SDA | SDA |
| SCL | SCL |

For the USART host demo, also connect the Arduino board to the PC through USB. The USB port is used as the serial communication interface between the board and the PC host software.

## Installation

**Arduino Library Manager:**

1. Open Arduino IDE.
2. Go to *Sketch → Include Library → Manage Libraries...*.
3. Search for `Arducam_Qwiic`.
4. Install the latest version.
5. Examples appear under *File → Examples → Arducam_Qwiic*.

## Examples

| Example | Description |
|---------|-------------|
| [CameraWebServer](examples/CameraWebServer/README.md) | WiFi web UI for browser-based live preview and camera control |
| [full_featured](examples/full_featured/README.md) | USART/Serial host-protocol demo for PC software control and image display |


