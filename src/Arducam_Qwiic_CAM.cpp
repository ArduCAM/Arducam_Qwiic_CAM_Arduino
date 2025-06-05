
/*
* This file is part of the Arducam Qwiic Camera project.
*
* Copyright 2025 Arducam Technology co., Ltd. All Rights Reserved.
*
* This work is licensed under the MIT license, see the file LICENSE for
* details.
*
*/

#include "Arducam_Qwiic_CAM.h"

Arducam_Qwiic_CAM::Arducam_Qwiic_CAM(void)
{
    burstFirstFlag = 0;
    previewMode = 0;
    currentPixelFormat = CAM_IMAGE_PIX_FMT_NONE;
    currentPictureMode = CAM_IMAGE_MODE_NONE; 
    deviceAddress = QWIIC_CAM_I2C_ADDRESS;
}

CamStatus Arducam_Qwiic_CAM::reset(void)
{
    writeReg(CAM_REG_SENSOR_RESET, CAM_SENSOR_RESET_ENABLE); 
    waitI2cIdle();
    return CAM_ERR_SUCCESS;
}

CamStatus Arducam_Qwiic_CAM::begin(void)
{
    QWIIC_WIRE.begin();
    QWIIC_WIRE.setClock(QWIIC_CAM_I2C_SPEED); // Set I2C clock speed to 400kHz

    // camera detect
    while(1){
        QWIIC_WIRE.beginTransmission(QWIIC_CAM_I2C_ADDRESS);
        if(QWIIC_WIRE.endTransmission() == 0){
            Serial.println("camera detect");
            break;
        }else{
            Serial.println("camera not detect");
            delay(100);
        }
    }

    reset(); // Reset camera
    return CAM_ERR_SUCCESS;
}

CamStatus Arducam_Qwiic_CAM::takePicture(CAM_IMAGE_MODE mode, CAM_IMAGE_PIX_FMT pixel_format)
{
    uint16_t overtime = 0;
    if (currentPixelFormat != pixel_format){
        currentPixelFormat = pixel_format;
        writeReg(CAM_REG_FORMAT, pixel_format);
        waitI2cIdle();
    }

    if (currentPictureMode != mode) {
        currentPictureMode = mode;
        writeReg(CAM_REG_CAPTURE_RESOLUTION, CAM_SET_CAPTURE_MODE | mode);
        waitI2cIdle();
    }

    writeReg(ARDUCHIP_FIFO, FIFO_CLEAR_ID_MASK); // Clear FIFO
    waitI2cIdle();
    writeReg(ARDUCHIP_FIFO, FIFO_START_MASK); // Start capture
    waitI2cIdle();
    while (getBit(ARDUCHIP_TRIG, CAP_DONE_MASK) == 0);
    totalLength = getTotalLength();
    unreceivedLength = totalLength;
    burstFirstFlag = 0;
    return CAM_ERR_SUCCESS;
}

CamStatus Arducam_Qwiic_CAM::setAutoWhiteBalanceMode(CAM_WHITE_BALANCE mode)
{
    writeReg(CAM_REG_WHILEBALANCE_MODE_CONTROL, mode);
    waitI2cIdle();
    return CAM_ERR_SUCCESS;
}

CamStatus Arducam_Qwiic_CAM::setColorEffect(CAM_COLOR_FX effect)
{
    writeReg(CAM_REG_COLOR_EFFECT_CONTROL, effect);
    waitI2cIdle();
    return CAM_ERR_SUCCESS;
}

CamStatus Arducam_Qwiic_CAM::setSaturation(CAM_STAURATION_LEVEL level)
{
    writeReg(CAM_REG_SATURATION_CONTROL, level);
    waitI2cIdle();
    return CAM_ERR_SUCCESS;
}

CamStatus Arducam_Qwiic_CAM::setEV(CAM_EV_LEVEL level)
{
    writeReg(CAM_REG_EV_CONTROL, level);
    waitI2cIdle();
    return CAM_ERR_SUCCESS;
}
CamStatus Arducam_Qwiic_CAM::setContrast(CAM_CONTRAST_LEVEL level)
{
    writeReg(CAM_REG_CONTRAST_CONTROL, level);
    waitI2cIdle();
    return CAM_ERR_SUCCESS;
}

CamStatus Arducam_Qwiic_CAM::setBrightness(CAM_BRIGHTNESS_LEVEL level)
{
    writeReg(CAM_REG_BRIGHTNESS_CONTROL, level);
    waitI2cIdle();
    return CAM_ERR_SUCCESS;
}

CamStatus Arducam_Qwiic_CAM::setSharpness(CAM_SHARPNESS_LEVEL level)
{
    writeReg(CAM_REG_SHARPNESS_CONTROL, level);
    waitI2cIdle();
    return CAM_ERR_SUCCESS;
}

CamStatus Arducam_Qwiic_CAM::setImageQuality(IMAGE_QUALITY quality)
{
    writeReg(CAM_REG_IMAGE_QUALITY, quality);
    waitI2cIdle();
    return CAM_ERR_SUCCESS;
}

CamStatus Arducam_Qwiic_CAM::writeReg(uint8_t reg, uint8_t data)
{
    QWIIC_WIRE.beginTransmission(deviceAddress);
    QWIIC_WIRE.write(reg);
    QWIIC_WIRE.write(data);
    QWIIC_WIRE.endTransmission();
}

uint8_t Arducam_Qwiic_CAM::readReg(uint8_t reg)
{
    uint8_t data = 0;
    QWIIC_WIRE.beginTransmission(deviceAddress);
    QWIIC_WIRE.write(reg);
    QWIIC_WIRE.endTransmission();
    QWIIC_WIRE.requestFrom(deviceAddress, 1);
    while(QWIIC_WIRE.available()){
        data = QWIIC_WIRE.read();
    }
    return data;
}

uint8_t Arducam_Qwiic_CAM::readImageBuff(uint8_t* buff, uint32_t length)
{
    uint32_t count = 0;
    if (unreceivedLength == 0 || (length == 0)) {
        return 0;
    }

    if (unreceivedLength < length) {
        length = unreceivedLength;
    }

    QWIIC_WIRE.beginTransmission(deviceAddress);
    QWIIC_WIRE.write(BURST_FIFO_READ);
    QWIIC_WIRE.endTransmission();
    QWIIC_WIRE.requestFrom(deviceAddress, length);
    while(QWIIC_WIRE.available()){
        buff[count++] = QWIIC_WIRE.read();
    }

    unreceivedLength -= length;
    return length;
}

uint8_t Arducam_Qwiic_CAM::readImageByte(void)
{
    uint8_t data = readReg(SINGLE_FIFO_READ);
    return data;
}

uint8_t Arducam_Qwiic_CAM::getBit(uint8_t addr, uint8_t bit)
{
    uint8_t temp;
    temp = readReg(addr);
    temp = temp & bit;
    return temp;
}

uint32_t Arducam_Qwiic_CAM::getTotalLength(void)
{
    uint32_t len1, len2, len3, length = 0;
    len1   = readReg(FIFO_SIZE1);
    len2   = readReg(FIFO_SIZE2);
    len3   = readReg(FIFO_SIZE3);
    length = ((len3 << 16) | (len2 << 8) | len1) & 0xffffff;
    return length;
}

void Arducam_Qwiic_CAM::waitI2cIdle(void)
{
    while(getBit(CAM_REG_SENSOR_STATE, CAM_REG_SENSOR_STATE_IDLE) == 0);
}
