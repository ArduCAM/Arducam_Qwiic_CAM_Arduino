/*
  full_featured: Serial-based Camera Control Demo

  A serial host protocol demo for the Arducam Qwiic CAM.
  Supports single capture and JPEG stream preview.

  Hardware Connections:
    QWIIC --> QWIIC

  Serial:
    Baudrate: 921600

  Protocol:
    Host to Camera:
      0x55 [command] [parameter bytes...] 0xAA

    Camera to Host:
      0x55 0xAA [packet type] [payload length, little-endian] [payload] 0x55 0xBB

  Capture:
    Format: JPEG / RGB565 / Y8
    Resolution: 96x96 / 128x128 / QVGA / VGA / HD / UXGA / FHD / WQXGA2
    Quality: High / Default / Low

  Video:
    Stream mode is implemented by repeatedly calling takePicture().
    Supports JPEG stream preview:
      1: 320x240
      2: 640x480

  Controls:
    Brightness: -4 ~ +4
    Contrast: -3 ~ +3
    Saturation: -3 ~ +3
    EV: -3 ~ +3
    WB Mode: Auto / Sunny / Office / Cloudy / Home
    Color FX: None / Bluish / Reddish / B&W / Sepia / Negative / Greenish / Over Exp / Solarize
    Sharpness: Auto / 1 ~ 8

  Notes:
    This is a binary protocol. Arduino Serial Monitor may show unreadable characters.
    Use the matching C# host software or a binary serial tool to send commands.
    JPEG/RGB565/Y8 data is streamed directly from FIFO to Serial to reduce RAM usage.
    The host software trims JPEG data by FF D8 ... FF D9 before decoding.

  License: MIT License (https://en.wikipedia.org/wiki/MIT_License)
  Web: http://www.ArduCAM.com
*/

#include "Arducam_Qwiic_CAM.h"
#include <string.h>

Arducam_Qwiic_CAM myCAM;

#define SERIAL_BAUD              921600
#define READ_IMAGE_LENGTH        255
#define COMMAND_BUF_SIZE         32
#define COMMAND_TIMEOUT_MS       200
#define CAPTURE_RETRY_COUNT      3

#define FORCE_RECONFIG_ON_FORMAT_CHANGE   1

#define DISCARD_FIRST_CAPTURE_AFTER_START 1

#define FORCE_RECONFIG_MODE ((CAM_IMAGE_MODE)0)

#define RESET_CAMERA                0xFF
#define SET_PICTURE_RESOLUTION      0x01
#define SET_VIDEO_RESOLUTION        0x02
#define SET_BRIGHTNESS              0x03
#define SET_CONTRAST                0x04
#define SET_SATURATION              0x05
#define SET_EV                      0x06
#define SET_WHITEBALANCE            0x07
#define SET_SPECIAL_EFFECTS         0x08
#define GET_CAMERA_INFO             0x0F
#define TAKE_PICTURE                0x10
#define SET_SHARPNESS               0x11
#define DEBUG_WRITE_REGISTER        0x12
#define STOP_STREAM                 0x21
#define GET_FRM_VER_INFO            0x30
#define GET_SDK_VER_INFO            0x40
#define SET_IMAGE_QUALITY           0x50

#define PACKET_IMAGE                0x01
#define PACKET_CAMERA_INFO          0x02
#define PACKET_FW_VERSION           0x03
#define PACKET_SDK_VERSION          0x05
#define PACKET_STATUS               0x06
#define PACKET_TEXT                 0x07
#define PACKET_STARTUP              0x08

uint8_t commandBuff[COMMAND_BUF_SIZE];
uint8_t commandLength = 0;
bool receivingCommand = false;
uint32_t lastCommandByteMs = 0;

uint8_t imageBuf[READ_IMAGE_LENGTH];

CAM_IMAGE_MODE currentPictureMode = CAM_IMAGE_MODE_QVGA;
CAM_IMAGE_PIX_FMT currentPixelFormat = CAM_IMAGE_PIX_FMT_JPG;
IMAGE_QUALITY currentImageQuality = DEFAULT_QUALITY;

bool streamActive = false;
CAM_IMAGE_MODE currentStreamMode = CAM_IMAGE_MODE_QVGA;

bool forceReconfigBeforeNextCapture = false;

bool discardFirstCaptureAfterStart = true;

void handleSerialProtocol(void);
void processCommand(const uint8_t* command, uint8_t length);

bool initCamera(void);
bool takePictureWithRetry(CAM_IMAGE_MODE mode, CAM_IMAGE_PIX_FMT fmt);
bool takeOnePicture(CAM_IMAGE_MODE mode, CAM_IMAGE_PIX_FMT fmt);
void forceReconfigBeforeCaptureIfNeeded(CAM_IMAGE_MODE mode, CAM_IMAGE_PIX_FMT fmt);
void discardFirstCaptureIfNeeded(CAM_IMAGE_MODE mode, CAM_IMAGE_PIX_FMT fmt);

void sendCurrentPicture(void);
void sendStreamFrame(void);
void stopStreamAndReply(void);
void sendRawFifoPayload(uint32_t imageLength);

bool protocolVideoParamToMode(uint8_t param, CAM_IMAGE_MODE* mode);
bool protocolPictureParamToMode(uint8_t param, CAM_IMAGE_MODE* mode);
bool protocolPixelFormatToFmt(uint8_t fmt, CAM_IMAGE_PIX_FMT* pixelFmt);

void sendFrameHeader(uint8_t packetType, uint32_t length);
void sendFrameTail(void);
void writeU32LE(uint32_t value);
void writeByte(uint8_t value);
void writeBytes(const uint8_t* data, uint32_t length);
void sendDataPack(uint8_t packetType, const char* msg);
void sendCameraInfo(void);
void sendFirmwareVersion(void);
void sendSdkVersion(void);
void sendStreamOff(void);

bool isValidQuality(uint8_t quality);

void setup() {
  Serial.begin(SERIAL_BAUD);

  if (!initCamera()) {
    sendDataPack(PACKET_TEXT, "Qwiic CAM init failed");
    return;
  }

  myCAM.setImageQuality(currentImageQuality);

  discardFirstCaptureAfterStart = true;
  forceReconfigBeforeNextCapture = false;

  sendDataPack(PACKET_TEXT, "Hello Arduino UNO R4 WiFi!");
  sendDataPack(PACKET_STARTUP, "Qwiic CAM start!");
}

void loop() {
  handleSerialProtocol();

  if (streamActive) {
    sendStreamFrame();
  }
}

bool initCamera(void) {
  if (myCAM.begin() != CAM_ERR_NONE) {
    return false;
  }

  for (uint8_t i = 0; i < 5; i++) {
    myCAM.writeReg(ARDUCHIP_TEST1, 0x55);
    uint8_t data = myCAM.readReg(ARDUCHIP_TEST1);
    if (data == 0x55) {
      return true;
    }
    delay(20);
  }

  return false;
}

void handleSerialProtocol(void) {
  while (Serial.available() > 0) {
    uint8_t b = (uint8_t)Serial.read();
    lastCommandByteMs = millis();

    if (!receivingCommand) {
      if (b == 0x55) {
        receivingCommand = true;
        commandLength = 0;
      }
      continue;
    }

    if (b == 0xAA) {
      if (commandLength > 0) {
        processCommand(commandBuff, commandLength);
      }
      receivingCommand = false;
      commandLength = 0;
      continue;
    }

    if (commandLength < COMMAND_BUF_SIZE) {
      commandBuff[commandLength++] = b;
    } else {
      receivingCommand = false;
      commandLength = 0;
      sendDataPack(PACKET_TEXT, "Command buffer overflow");
    }
  }

  if (receivingCommand && (millis() - lastCommandByteMs > COMMAND_TIMEOUT_MS)) {
    receivingCommand = false;
    commandLength = 0;
  }
}

void processCommand(const uint8_t* command, uint8_t length) {
  uint8_t cmd = command[0];

  switch (cmd) {
    case SET_PICTURE_RESOLUTION: {
      if (length < 2) break;

      uint8_t protocolMode = command[1] & 0x0F;
      uint8_t protocolFmt = (command[1] & 0x70) >> 4;

      CAM_IMAGE_MODE mappedMode;
      CAM_IMAGE_PIX_FMT mappedFmt;

      if (!protocolPictureParamToMode(protocolMode, &mappedMode)) {
        sendDataPack(PACKET_TEXT, "Resolution unsupported by Qwiic CAM");
        break;
      }

      if (!protocolPixelFormatToFmt(protocolFmt, &mappedFmt)) {
        sendDataPack(PACKET_TEXT, "Format unsupported by Qwiic CAM");
        break;
      }

#if FORCE_RECONFIG_ON_FORMAT_CHANGE
      if (mappedFmt != currentPixelFormat) {
        forceReconfigBeforeNextCapture = true;
      }
#endif

      currentPictureMode = mappedMode;
      currentPixelFormat = mappedFmt;

      break;
    }

    case SET_VIDEO_RESOLUTION: {
      if (length >= 2) {
        CAM_IMAGE_MODE mode;
        if (protocolVideoParamToMode(command[1], &mode)) {
#if FORCE_RECONFIG_ON_FORMAT_CHANGE
          if (currentPixelFormat != CAM_IMAGE_PIX_FMT_JPG ||
              currentPictureMode != mode) {
            forceReconfigBeforeNextCapture = true;
          }
#endif

          currentStreamMode = mode;
          currentPictureMode = mode;
          currentPixelFormat = CAM_IMAGE_PIX_FMT_JPG;
          streamActive = true;
        }
      }
      break;
    }

    case SET_BRIGHTNESS:
      if (length >= 2) myCAM.setBrightness((CAM_BRIGHTNESS_LEVEL)command[1]);
      break;

    case SET_CONTRAST:
      if (length >= 2) myCAM.setContrast((CAM_CONTRAST_LEVEL)command[1]);
      break;

    case SET_SATURATION:
      if (length >= 2) myCAM.setSaturation((CAM_SATURATION_LEVEL)command[1]);
      break;

    case SET_EV:
      if (length >= 2) myCAM.setEV((CAM_EV_LEVEL)command[1]);
      break;

    case SET_WHITEBALANCE:
      if (length >= 2) myCAM.setAutoWhiteBalanceMode((CAM_WHITE_BALANCE)command[1]);
      break;

    case SET_SPECIAL_EFFECTS:
      if (length >= 2) myCAM.setColorEffect((CAM_COLOR_FX)command[1]);
      break;

    case GET_CAMERA_INFO:
      sendCameraInfo();
      break;

    case TAKE_PICTURE:
      if (takePictureWithRetry(currentPictureMode, currentPixelFormat)) {
        sendCurrentPicture();
      } else {
        sendDataPack(PACKET_TEXT, "Capture failed");
      }
      break;

    case SET_SHARPNESS:
      if (length >= 2) myCAM.setSharpness((CAM_SHARPNESS_LEVEL)command[1]);
      break;

    case DEBUG_WRITE_REGISTER:
      if (length >= 5) {
        uint8_t reg = command[3];
        uint8_t value = command[4];
        myCAM.writeReg(reg, value);
      }
      break;

    case STOP_STREAM:
      stopStreamAndReply();
      break;

    case GET_FRM_VER_INFO:
      sendFirmwareVersion();
      break;

    case GET_SDK_VER_INFO:
      sendSdkVersion();
      break;

    case RESET_CAMERA:
      streamActive = false;
      myCAM.reset();
      delay(200);

      discardFirstCaptureAfterStart = true;
      forceReconfigBeforeNextCapture = true;

      sendDataPack(PACKET_STARTUP, "Camera reset");
      break;

    case SET_IMAGE_QUALITY:
      if (length >= 2 && isValidQuality(command[1])) {
        currentImageQuality = (IMAGE_QUALITY)command[1];
        myCAM.setImageQuality(currentImageQuality);
      }
      break;

    default:
      sendDataPack(PACKET_TEXT, "Unknown command");
      break;
  }
}

bool takeOnePicture(CAM_IMAGE_MODE mode, CAM_IMAGE_PIX_FMT fmt) {
  CamStatus status = myCAM.takePicture(mode, fmt);
  uint32_t len = myCAM.getTotalLength();

  return (status == CAM_ERR_NONE && len > 0);
}

void discardFirstCaptureIfNeeded(CAM_IMAGE_MODE mode, CAM_IMAGE_PIX_FMT fmt) {
#if DISCARD_FIRST_CAPTURE_AFTER_START
  if (!discardFirstCaptureAfterStart) {
    return;
  }

  discardFirstCaptureAfterStart = false;

  myCAM.takePicture(mode, fmt);
  delay(80);
#else
  (void)mode;
  (void)fmt;
#endif
}

void forceReconfigBeforeCaptureIfNeeded(CAM_IMAGE_MODE mode, CAM_IMAGE_PIX_FMT fmt) {
#if FORCE_RECONFIG_ON_FORMAT_CHANGE
  if (!forceReconfigBeforeNextCapture) {
    return;
  }

  forceReconfigBeforeNextCapture = false;

  myCAM.takePicture(FORCE_RECONFIG_MODE, fmt);
  delay(80);
#else
  (void)mode;
  (void)fmt;
#endif
}

bool takePictureWithRetry(CAM_IMAGE_MODE mode, CAM_IMAGE_PIX_FMT fmt) {

  discardFirstCaptureIfNeeded(mode, fmt);
  forceReconfigBeforeCaptureIfNeeded(mode, fmt);

  for (uint8_t i = 0; i < CAPTURE_RETRY_COUNT; i++) {
    if (takeOnePicture(mode, fmt)) {
      return true;
    }
    delay(50);
  }

  return false;
}

void sendCurrentPicture(void) {
  uint32_t imageLength = myCAM.getTotalLength();
  if (imageLength == 0) {
    sendDataPack(PACKET_TEXT, "Image length is zero");
    return;
  }

  sendFrameHeader(PACKET_IMAGE, imageLength);
  sendRawFifoPayload(imageLength);
  sendFrameTail();
}

void sendRawFifoPayload(uint32_t imageLength) {
  uint32_t remaining = imageLength;
  while (remaining > 0) {
    uint8_t block = (remaining > READ_IMAGE_LENGTH) ? READ_IMAGE_LENGTH : (uint8_t)remaining;
    uint32_t n = myCAM.readImageBuf(imageBuf, block);
    if (n == 0) {
      break;
    }
    writeBytes(imageBuf, n);
    remaining -= n;
  }
}

void sendStreamFrame(void) {
  handleSerialProtocol();
  if (!streamActive) return;

#if FORCE_RECONFIG_ON_FORMAT_CHANGE
  if (currentPixelFormat != CAM_IMAGE_PIX_FMT_JPG ||
      currentPictureMode != currentStreamMode) {
    forceReconfigBeforeNextCapture = true;
  }
#endif

  currentPixelFormat = CAM_IMAGE_PIX_FMT_JPG;
  currentPictureMode = currentStreamMode;

  if (takePictureWithRetry(currentStreamMode, CAM_IMAGE_PIX_FMT_JPG)) {
    sendCurrentPicture();
  }

  handleSerialProtocol();
}

void stopStreamAndReply(void) {
  streamActive = false;
  sendStreamOff();
}

void sendCameraInfo(void) {
  char info[260];
  uint8_t sensorId = myCAM.readReg(CAM_REG_SENSOR_ID);
  uint8_t yearId = myCAM.readReg(CAM_REG_YEAR_ID);
  uint8_t monthId = myCAM.readReg(CAM_REG_MONTH_ID);
  uint8_t dayId = myCAM.readReg(CAM_REG_DAY_ID);

  snprintf(info, sizeof(info),
           "ReportCameraInfo\r\n"
           "Camera Type:Arducam Qwiic CAM\r\n"
           "Interface:I2C/Qwiic\r\n"
           "Sensor ID:0x%02X\r\n"
           "Firmware Date:20%02u-%02u-%02u\r\n"
           "Support Format:JPEG/RGB565/Y8\r\n"
           "Support Resolution:96x96,128x128,QVGA,320x320,VGA,HD,UXGA,FHD,WQXGA2\r\n"
           "Y8 Output:8-bit Y8 from STM32 firmware\r\n",
           sensorId, yearId, monthId, dayId);

  uint32_t len = strlen(info);
  sendFrameHeader(PACKET_CAMERA_INFO, len);
  writeBytes((const uint8_t*)info, len);
  sendFrameTail();
}

void sendFirmwareVersion(void) {
  uint8_t ver[4] = {0x25, 0x06, 0x15, 0x10};
  sendFrameHeader(PACKET_FW_VERSION, sizeof(ver));
  writeBytes(ver, sizeof(ver));
  sendFrameTail();
}

void sendSdkVersion(void) {
  const char sdk[] = "2.0.0\r\n";
  sendFrameHeader(PACKET_SDK_VERSION, strlen(sdk));
  writeBytes((const uint8_t*)sdk, strlen(sdk));
  sendFrameTail();
}

void sendStreamOff(void) {
  const char msg[] = "streamoff";
  sendFrameHeader(PACKET_STATUS, strlen(msg));
  writeBytes((const uint8_t*)msg, strlen(msg));
  sendFrameTail();
}

void sendDataPack(uint8_t packetType, const char* msg) {
  uint32_t len = strlen(msg) + 2;
  sendFrameHeader(packetType, len);
  writeBytes((const uint8_t*)msg, strlen(msg));
  writeByte('\r');
  writeByte('\n');
  sendFrameTail();
}

void sendFrameHeader(uint8_t packetType, uint32_t length) {
  writeByte(0x55);
  writeByte(0xAA);
  writeByte(packetType);
  writeU32LE(length);
}

void sendFrameTail(void) {
  writeByte(0x55);
  writeByte(0xBB);
}

void writeU32LE(uint32_t value) {
  writeByte((uint8_t)(value & 0xFF));
  writeByte((uint8_t)((value >> 8) & 0xFF));
  writeByte((uint8_t)((value >> 16) & 0xFF));
  writeByte((uint8_t)((value >> 24) & 0xFF));
}

void writeByte(uint8_t value) {
  Serial.write(value);
}

void writeBytes(const uint8_t* data, uint32_t length) {
  while (length > 0) {
    size_t chunk = (length > 64) ? 64 : (size_t)length;
    Serial.write(data, chunk);
    data += chunk;
    length -= chunk;
  }
}

bool protocolVideoParamToMode(uint8_t param, CAM_IMAGE_MODE* mode) {
  if (mode == NULL) return false;

  switch (param) {
    case 1:
      *mode = CAM_IMAGE_MODE_QVGA;
      return true;
    case 2:
      *mode = CAM_IMAGE_MODE_VGA;
      return true;
    default:
      return false;
  }
}

bool protocolPictureParamToMode(uint8_t param, CAM_IMAGE_MODE* mode) {
  if (mode == NULL) return false;

  switch (param) {
    case CAM_IMAGE_MODE_QVGA:
      *mode = CAM_IMAGE_MODE_QVGA;
      return true;
    case CAM_IMAGE_MODE_VGA:
      *mode = CAM_IMAGE_MODE_VGA;
      return true;
    case CAM_IMAGE_MODE_HD:
      *mode = CAM_IMAGE_MODE_HD;
      return true;
    case CAM_IMAGE_MODE_UXGA:
      *mode = CAM_IMAGE_MODE_UXGA;
      return true;
    case CAM_IMAGE_MODE_FHD:
      *mode = CAM_IMAGE_MODE_FHD;
      return true;
    case CAM_IMAGE_MODE_WQXGA2:
      *mode = CAM_IMAGE_MODE_WQXGA2;
      return true;
    case CAM_IMAGE_MODE_96X96:
      *mode = CAM_IMAGE_MODE_96X96;
      return true;
    case CAM_IMAGE_MODE_128X128:
      *mode = CAM_IMAGE_MODE_128X128;
      return true;
    case CAM_IMAGE_MODE_320X320:
      *mode = CAM_IMAGE_MODE_320X320;
      return true;
    default:
      return false;
  }
}

bool protocolPixelFormatToFmt(uint8_t fmt, CAM_IMAGE_PIX_FMT* pixelFmt) {
  if (pixelFmt == NULL) return false;

  switch (fmt) {
    case 1:
      *pixelFmt = CAM_IMAGE_PIX_FMT_JPG;
      return true;
    case 2:
      *pixelFmt = CAM_IMAGE_PIX_FMT_RGB565;
      return true;
    case 3:
      *pixelFmt = CAM_IMAGE_PIX_FMT_Y8;
      return true;
    default:
      return false;
  }
}

bool isValidQuality(uint8_t quality) {
  return (quality == HIGH_QUALITY ||
          quality == DEFAULT_QUALITY ||
          quality == LOW_QUALITY);
}