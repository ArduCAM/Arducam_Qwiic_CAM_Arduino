/*
* This file is part of the Arducam Qwiic Camera project.
*
* Copyright 2026 Arducam Technology co., Ltd. All Rights Reserved.
*
* This work is licensed under the MIT license, see the file LICENSE for
* details.
*
*/

#include "Arducam_Qwiic_CAM.h"

Arducam_Qwiic_CAM::Arducam_Qwiic_CAM(void)
{
    totalLength = 0;
    unreceivedLength = 0;
    cameraId = 0;
    burstFirstFlag = 0;
    previewMode = 0;
    currentPixelFormat = CAM_IMAGE_PIX_FMT_NONE;
    currentPictureMode = CAM_IMAGE_MODE_NONE;
    deviceAddress = QWIIC_CAM_I2C_ADDRESS;
}

CamStatus Arducam_Qwiic_CAM::reset(void)
{
    CAM_RETURN_IF_ERR(writeReg(CAM_REG_SENSOR_RESET, CAM_SENSOR_RESET_ENABLE)); 
    CAM_RETURN_IF_ERR(waitI2cIdle());
    return CAM_ERR_NONE;
}

CamStatus Arducam_Qwiic_CAM::begin(void)
{
    QWIIC_WIRE.begin();
    QWIIC_WIRE.setClock(QWIIC_CAM_I2C_SPEED); // Set I2C clock speed
    return CAM_ERR_NONE;
}

CamStatus Arducam_Qwiic_CAM::takePicture(CAM_IMAGE_MODE mode, CAM_IMAGE_PIX_FMT pixel_format)
{
    CamStatus ret = CAM_ERR_NONE;


    if (currentPixelFormat != pixel_format){
        CAM_RETURN_IF_ERR(writeReg(CAM_REG_FORMAT, pixel_format));
        CAM_RETURN_IF_ERR(waitI2cIdle());
        currentPixelFormat = pixel_format;
    }

    if (currentPictureMode != mode) {
        CAM_RETURN_IF_ERR(writeReg(CAM_REG_CAPTURE_RESOLUTION, CAM_SET_CAPTURE_MODE | mode));
        CAM_RETURN_IF_ERR(waitI2cIdle());
        currentPictureMode = mode;
    }

    CAM_RETURN_IF_ERR(writeReg(ARDUCHIP_FIFO, FIFO_CLEAR_ID_MASK)); // Clear FIFO
    CAM_RETURN_IF_ERR(waitI2cIdle());
    CAM_RETURN_IF_ERR(writeReg(ARDUCHIP_FIFO, FIFO_START_MASK)); // Start capture
    CAM_RETURN_IF_ERR(waitI2cIdle());

    unsigned long startTime = millis();
    while(millis() - startTime < CAM_TIMEOUT_MS) {
        if(getBit(ARDUCHIP_TRIG, CAP_DONE_MASK)) {
            totalLength = ((readReg(FIFO_SIZE3) << 16) | (readReg(FIFO_SIZE2) << 8) | readReg(FIFO_SIZE1));
            unreceivedLength = totalLength;
            burstFirstFlag = 0;
            return CAM_ERR_NONE;
        }
        delay(1);
    }

    return CAM_ERR_TIMEOUT;
}

CamStatus Arducam_Qwiic_CAM::setAutoWhiteBalanceMode(CAM_WHITE_BALANCE mode)
{
    CAM_RETURN_IF_ERR(writeReg(CAM_REG_WHITEBALANCE_MODE_CONTROL, mode));
    CAM_RETURN_IF_ERR(waitI2cIdle());
    return CAM_ERR_NONE;
}

CamStatus Arducam_Qwiic_CAM::setColorEffect(CAM_COLOR_FX effect)
{
    CAM_RETURN_IF_ERR(writeReg(CAM_REG_COLOR_EFFECT_CONTROL, effect));
    CAM_RETURN_IF_ERR(waitI2cIdle());
    return CAM_ERR_NONE;
}

CamStatus Arducam_Qwiic_CAM::setSaturation(CAM_SATURATION_LEVEL level)
{
    CAM_RETURN_IF_ERR(writeReg(CAM_REG_SATURATION_CONTROL, level));
    CAM_RETURN_IF_ERR(waitI2cIdle());
    return CAM_ERR_NONE;
}

CamStatus Arducam_Qwiic_CAM::setEV(CAM_EV_LEVEL level)
{
    CAM_RETURN_IF_ERR(writeReg(CAM_REG_EV_CONTROL, level));
    CAM_RETURN_IF_ERR(waitI2cIdle());
    return CAM_ERR_NONE;
}

CamStatus Arducam_Qwiic_CAM::setContrast(CAM_CONTRAST_LEVEL level)
{
    CAM_RETURN_IF_ERR(writeReg(CAM_REG_CONTRAST_CONTROL, level));
    CAM_RETURN_IF_ERR(waitI2cIdle());
    return CAM_ERR_NONE;
}

CamStatus Arducam_Qwiic_CAM::setBrightness(CAM_BRIGHTNESS_LEVEL level)
{
    CAM_RETURN_IF_ERR(writeReg(CAM_REG_BRIGHTNESS_CONTROL, level));
    CAM_RETURN_IF_ERR(waitI2cIdle());
    return CAM_ERR_NONE;
}

CamStatus Arducam_Qwiic_CAM::setSharpness(CAM_SHARPNESS_LEVEL level)
{
    CAM_RETURN_IF_ERR(writeReg(CAM_REG_SHARPNESS_CONTROL, level));
    CAM_RETURN_IF_ERR(waitI2cIdle());
    return CAM_ERR_NONE;
}

CamStatus Arducam_Qwiic_CAM::setImageQuality(IMAGE_QUALITY quality)
{
    CAM_RETURN_IF_ERR(writeReg(CAM_REG_IMAGE_QUALITY, quality));
    CAM_RETURN_IF_ERR(waitI2cIdle());
    return CAM_ERR_NONE;
}

CamStatus Arducam_Qwiic_CAM::writeReg(uint8_t reg, uint8_t data)
{
    QWIIC_WIRE.beginTransmission(deviceAddress);
    QWIIC_WIRE.write(reg);
    QWIIC_WIRE.write(data);
    uint8_t ret = QWIIC_WIRE.endTransmission();
    if(!ret) {
        return CAM_ERR_NONE;
    }else {
        return CAM_ERR_NO_CALLBACK;
    }
}

uint8_t Arducam_Qwiic_CAM::readReg(uint8_t reg)
{
    uint8_t data = 0;
    QWIIC_WIRE.beginTransmission(deviceAddress);
    QWIIC_WIRE.write(reg);
    QWIIC_WIRE.endTransmission(false);
    QWIIC_WIRE.requestFrom(deviceAddress, (uint8_t)1);
    while (QWIIC_WIRE.available()) {
        data = QWIIC_WIRE.read();
    }
    return data;
}

uint32_t Arducam_Qwiic_CAM::readImageBuf(uint8_t* buf, uint32_t length)
{
    if (unreceivedLength == 0 || length == 0 || buf == NULL) {
        return 0;
    }

    if (length > unreceivedLength) {
        length = unreceivedLength;
    }

    // First read of a new frame: reset FIFO read pointer
    if (burstFirstFlag == 0) {
        CAM_RETURN_IF_ERR(writeReg(ARDUCHIP_FIFO, FIFO_RDPTR_RST_MASK));
        CAM_RETURN_IF_ERR(waitI2cIdle());
        burstFirstFlag = 1;
    }

    uint32_t totalRead = 0;

    // Read in chunks to stay within Wire library buffer limits
    while (totalRead < length) {
        uint32_t remaining = length - totalRead;
        uint8_t chunkSize = (remaining > I2C_BUFFER_SIZE) ? I2C_BUFFER_SIZE : (uint8_t)remaining;

        QWIIC_WIRE.beginTransmission(deviceAddress);
        QWIIC_WIRE.write(BURST_FIFO_READ);
        QWIIC_WIRE.endTransmission(false);

        uint8_t bytesReceived = QWIIC_WIRE.requestFrom(deviceAddress, chunkSize);
        uint8_t chunkRead = 0;
        while (QWIIC_WIRE.available() && chunkRead < bytesReceived) {
            buf[totalRead++] = QWIIC_WIRE.read();
            chunkRead++;
        }
    }

    if (totalRead > 0 && totalRead <= unreceivedLength) {
        unreceivedLength -= totalRead;
    }

    // All data received: clear FIFO write pointer and reset flags
    if (unreceivedLength == 0) {
        CAM_RETURN_IF_ERR(writeReg(ARDUCHIP_FIFO, FIFO_CLEAR_MASK));
        CAM_RETURN_IF_ERR(waitI2cIdle());
    }

    return totalRead;
}

uint8_t Arducam_Qwiic_CAM::readImageByte(void)
{
    return readReg(SINGLE_FIFO_READ);
}

uint8_t Arducam_Qwiic_CAM::getBit(uint8_t addr, uint8_t bit)
{
    return (readReg(addr) & bit);
}

uint32_t Arducam_Qwiic_CAM::getTotalLength(void)
{
    return totalLength;
}

CamStatus Arducam_Qwiic_CAM::clearFIFO(void)
{
    CAM_RETURN_IF_ERR(writeReg(ARDUCHIP_FIFO, FIFO_CLEAR_MASK));
    CAM_RETURN_IF_ERR(waitI2cIdle());
    unreceivedLength = 0;
    burstFirstFlag = 0;
    return CAM_ERR_NONE;
}

CamStatus Arducam_Qwiic_CAM::waitI2cIdle(void)
{
    unsigned long startMillis = millis();
    while((millis() - startMillis) < CAM_TIMEOUT_MS) {
        if(getBit(CAM_REG_SENSOR_STATE, CAM_REG_SENSOR_STATE_IDLE)) {
            return CAM_ERR_NONE;
        }else {
            delay(1);
        }
    }
    return CAM_ERR_TIMEOUT;
}

uint32_t Arducam_Qwiic_CAM::getUnreceivedLength() const
{
    return unreceivedLength;
}

uint8_t Arducam_Qwiic_CAM::getCameraId() const
{
    return cameraId;
}

bool Arducam_Qwiic_CAM::isBurstFirst() const
{
    return (burstFirstFlag != 0);
}

uint8_t Arducam_Qwiic_CAM::getPreviewMode() const
{
    return previewMode;
}

uint8_t Arducam_Qwiic_CAM::getCurrentPixelFormat() const
{
    return currentPixelFormat;
}

uint8_t Arducam_Qwiic_CAM::getCurrentPictureMode() const
{
    return currentPictureMode;
}

uint8_t Arducam_Qwiic_CAM::getDeviceAddress() const
{
    return deviceAddress;
}

void Arducam_Qwiic_CAM::setDeviceAddress(uint8_t addr)
{
    deviceAddress = addr;
}

void Arducam_Qwiic_CAM::setPreviewMode(uint8_t mode)
{
    previewMode = mode;
}

void Arducam_Qwiic_CAM::setBurstFirst(bool enable)
{
    burstFirstFlag = enable ? 1 : 0;
}
