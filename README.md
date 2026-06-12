# Arducam Qwiic CAM Arduino Library

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

Arduino library for Arducam Qwiic series camera modules, enabling rapid image capture and basic computer vision applications over I2C.

## Features

- **Qwiic I2C interface** — plug-and-play, no soldering required
- **Multi-platform** — works with Arduino UNO R4 WiFi, Portenta C33, and any I2C-capable board
- **Multiple resolutions** — 96×96, 128×128, 320×240, 320×320, 640×480, 1280×720, 1600×1200
- **Pixel formats** — RGB565, Y8 (grayscale), JPEG
- **Image buffering & streaming** — capture single frames or stream over WiFi

## Camera Specs

| Format  | Resolutions                                                  | Type  |
|---------|--------------------------------------------------------------|-------|
| JPEG    | 96×96, 128×128, 320×240, 320×320, 640×480, 1280×720, 1600×1200 | Color |
| RGB565  | 96×96, 128×128                                               | Color |
| Y8      | 96×96, 128×128                                               | Gray  |

## Hardware Requirements

- Arducam Qwiic CAM module
- Arduino-compatible board
- Qwiic cable or 4-pin I2C connector

## Wiring

| Arduino  | Qwiic CAM |
|----------|-----------|
| 3.3V     | VCC       |
| GND      | GND       |
| SDA      | SDA       |
| SCL      | SCL       |

## Installation

**Arduino Library Manager:**
1. *Sketch → Include Library → Manage Libraries...*
2. Search for `Arducam_Qwiic`
3. Install the latest version
4. Examples appear under *File → Examples → Arducam_Qwiic*

## Examples

| Example | Description |
|---------|-------------|
| [CameraWebServer](examples/CameraWebServer/README.md) | WiFi web UI for live preview and camera control (resolution, quality, effects) |

