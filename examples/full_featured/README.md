# full_featured

A serial/USART-based camera control demo for the Arducam Qwiic CAM. It uses a binary host communication protocol over the USB serial port. A PC host application can control the camera, capture JPEG/RGB565/Y8 images, and display JPEG stream preview.

## Wiring

Connect the Qwiic CAM to your board via a Qwiic cable (I2C):

| Arduino  | Qwiic CAM |
|----------|-----------|
| 3.3V     | VCC       |
| GND      | GND       |
| SDA      | SDA       |
| SCL      | SCL       |

Connect the Arduino board to the PC through USB. The USB port is used as the serial/USART communication interface between the board and the host software.

## Quick Start

1. Open full_featured.ino in the Arduino IDE.
2. Select the correct Arduino board.
3. Select the correct serial port.
4. Upload the sketch to the Arduino board.
5. Close the Arduino Serial Monitor if it is open.
6. Run Arducam Qwiic CAM Host.exe on the PC.
7. Select the correct COM port, for example COM6.
8. Set the baudrate to 921600.
9. Click Open to connect to the board.
10. Select the required video or capture resolution.
11. Click Video to start JPEG stream preview, or use the Capture tab to take a single image.

> This demo uses a binary serial protocol. Do not use the Arduino Serial Monitor to view image data, because binary image packets may appear as unreadable characters.

## Serial Protocol

### Host to Camera

The host sends commands to the camera in this format:

```text
0x55 [command] [parameter bytes...] 0xAA
```

Example:

```text
55 01 11 AA
```

This sets the capture format to JPEG and the resolution to 320×240.

### Camera to Host

The camera returns data to the host in this format:

```text
0x55 0xAA [packet type] [payload length, little-endian, 4 bytes] [payload] 0x55 0xBB
```

Example image packet:

```text
55 AA 01 [length] [image data] 55 BB
```

JPEG payload may include FIFO padding after the JPEG end marker. The host software extracts the real JPEG data from `FF D8` to `FF D9` before decoding.

## Supported Commands

| Command | Function |
|--------:|----------|
| `0x01` | Set capture format and resolution |
| `0x02` | Start JPEG stream preview |
| `0x03` | Set brightness |
| `0x04` | Set contrast |
| `0x05` | Set saturation |
| `0x06` | Set EV |
| `0x07` | Set white balance |
| `0x08` | Set color effect |
| `0x0F` | Get camera information |
| `0x10` | Take picture |
| `0x11` | Set sharpness |
| `0x21` | Stop stream preview |
| `0x30` | Get firmware version |
| `0x40` | Get SDK version |
| `0x50` | Set JPEG quality |
| `0xFF` | Reset camera |

## Capture Settings

| Control       | Range / Options |
|---------------|-----------------|
| Format        | JPEG / RGB565 / Y8 |
| JPEG Resolution | 96×96 / 128×128 / 320×240 / 320×320 / 640×480 / 1280×720 / 1600×1200 / 1920×1080 / 2592×1944 |
| RGB565 Resolution | 96×96 / 128×128 / 320×240 |
| Y8 Resolution | 96×96 / 128×128 / 320×240 |
| Quality       | High / Default / Low |
| Brightness    | −4 ~ +4 |
| Contrast      | −3 ~ +3 |
| Saturation    | −3 ~ +3 |
| EV            | −3 ~ +3 |
| White Balance | Auto / Sunny / Office / Cloudy / Home |
| Color FX      | None / Bluish / Reddish / B&W / Sepia / Negative / Greenish / Over Exposure / Solarize |
| Sharpness     | Auto / 1 ~ 8 |

## Stream Preview

The stream preview command is implemented by repeatedly calling `takePicture()` and sending each JPEG frame as one protocol image packet.

| Parameter | Stream Resolution |
|----------:|-------------------|
| `1` | 320×240 |
| `2` | 640×480 |

Start stream example:

```text
55 02 01 AA
```

Stop stream example:

```text
55 21 AA
```

When the stream stops, the camera returns:

```text
55 AA 06 09 00 00 00 73 74 72 65 61 6D 6F 66 66 55 BB
```

The payload is:

```text
streamoff
```

## Y8 Output

Y8 data is expected to be true 8-bit grayscale data from the camera module / STM32 firmware. This demo treats Y8 as 8-bit Y8 and does not perform Y16-to-Y8 conversion on Arduino.

For example, 320×240 Y8 is sent as:

```text
320 × 240 × 1 = 76800 bytes
```

instead of:

```text
320 × 240 × 2 = 153600 bytes
```

## Dependencies

- `Arducam_Qwiic_CAM` library
- C# host software for serial control and image display

## Troubleshooting

- **No response from the host software?** Check that the correct COM port is selected and the baudrate is set to `921600`.
- **COM port open failed?** Close Arduino Serial Monitor or any other serial tool that may be using the same COM port.
- **Image data appears as garbled text?** This is normal in a text serial monitor because the protocol is binary. Use the C# host software or a binary serial tool.
- **JPEG image contains extra `A5` bytes?** This is FIFO padding. The host software trims JPEG data from `FF D8` to `FF D9`.
- **Large JPEG captures are slow?** Use a lower resolution or set JPEG quality to Low.
- **RGB565/Y8 high-resolution capture is unavailable?** RGB565 and Y8 are limited to 96×96, 128×128, and 320×240 in this demo to keep transfer time and memory usage reasonable.
