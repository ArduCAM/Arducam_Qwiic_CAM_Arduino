
/*
* This file is part of the Arducam Qwiic project.
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
    deviceAddress = 0x0C;
}

CamStatus Arducam_Qwiic_CAM::reset(void)
{
    writeReg(CAM_REG_SENSOR_RESET, CAM_SENSOR_RESET_ENABLE); 
}

CamStatus Arducam_Qwiic_CAM::begin(void)
{
    Wire1.begin();
    Wire1.setClock(400000); // Set I2C clock speed to 400kHz
    delay(100);
}

CamStatus Arducam_Qwiic_CAM::takePicture(CAM_IMAGE_MODE mode, CAM_IMAGE_PIX_FMT pixel_format)
{
    uint16_t overtime = 0;
    if (currentPixelFormat != pixel_format){
        currentPixelFormat = pixel_format;
        writeReg(CAM_REG_FORMAT, pixel_format);
    }

    if (currentPictureMode != mode) {
        currentPictureMode = mode;
        writeReg(CAM_REG_CAPTURE_RESOLUTION, CAM_SET_CAPTURE_MODE | mode);
    }

    writeReg(ARDUCHIP_FIFO, FIFO_CLEAR_ID_MASK); // Clear FIFO
    writeReg(ARDUCHIP_FIFO, FIFO_START_MASK); // Start capture
    while (getBit(ARDUCHIP_TRIG, CAP_DONE_MASK) == 0){
        delay(1);
        overtime++;
        if (overtime > 2000) {
            Serial.print("Capture timeout\n");
            return CAM_ERR_NO_CALLBACK;
        }
    }
    totalLength = getTotalLength();
    burstFirstFlag = 0;
    return CAM_ERR_SUCCESS;
}

CamStatus Arducam_Qwiic_CAM::setAutoWhiteBalanceMode(CAM_WHITE_BALANCE mode)
{
    writeReg(CAM_REG_WHILEBALANCE_MODE_CONTROL, mode);
    return CAM_ERR_SUCCESS;
}

CamStatus Arducam_Qwiic_CAM::setColorEffect(CAM_COLOR_FX effect)
{
    writeReg(CAM_REG_COLOR_EFFECT_CONTROL, effect);
    return CAM_ERR_SUCCESS;
}

CamStatus Arducam_Qwiic_CAM::setSaturation(CAM_STAURATION_LEVEL level)
{
    writeReg(CAM_REG_SATURATION_CONTROL, level);
    return CAM_ERR_SUCCESS;
}

CamStatus Arducam_Qwiic_CAM::setEV(CAM_EV_LEVEL level)
{
    writeReg(CAM_REG_EV_CONTROL, level);
    return CAM_ERR_SUCCESS;
}
CamStatus Arducam_Qwiic_CAM::setContrast(CAM_CONTRAST_LEVEL level)
{
    writeReg(CAM_REG_CONTRAST_CONTROL, level);
    return CAM_ERR_SUCCESS;
}

CamStatus Arducam_Qwiic_CAM::setBrightness(CAM_BRIGHTNESS_LEVEL level)
{
    writeReg(CAM_REG_BRIGHTNESS_CONTROL, level);
    return CAM_ERR_SUCCESS;
}

CamStatus Arducam_Qwiic_CAM::setSharpness(CAM_SHARPNESS_LEVEL level)
{
    writeReg(CAM_REG_SHARPNESS_CONTROL, level);
    return CAM_ERR_SUCCESS;
}

CamStatus Arducam_Qwiic_CAM::setImageQuality(IMAGE_QUALITY quality)
{
    writeReg(CAM_REG_IMAGE_QUALITY, quality);
    return CAM_ERR_SUCCESS;
}

CamStatus Arducam_Qwiic_CAM::writeReg(uint8_t reg, uint8_t data)
{
    Wire1.beginTransmission(deviceAddress);
    Wire1.write(reg);
    Wire1.write(data);
    Wire1.endTransmission();
}

uint8_t Arducam_Qwiic_CAM::readReg(uint8_t reg)
{
    uint8_t data = 0;
    Wire1.beginTransmission(deviceAddress);
    Wire1.write(reg);
    Wire1.endTransmission();
    Wire1.requestFrom(deviceAddress, 1);
    while(Wire1.available()){
        data = Wire1.read();
    }
    return data;
}

uint8_t Arducam_Qwiic_CAM::readImageBuff(WiFiClient *client, uint8_t* buff, uint32_t length)
{
    uint32_t block_num = length / READ_IMAGE_LENGTH;
    uint32_t left_len = length % READ_IMAGE_LENGTH;
    uint32_t read_len = 0;
    uint8_t count = 0;

    for(uint32_t i = 0; i < block_num; i++) {
        Wire1.beginTransmission(deviceAddress);
        Wire1.write(BURST_FIFO_READ);
        Wire1.endTransmission();
        Wire1.requestFrom(deviceAddress, READ_IMAGE_LENGTH);
        count = 0;
        while(Wire1.available()){
            buff[count++] = Wire1.read();
            read_len++;
        }
        client->write(buff, READ_IMAGE_LENGTH);
    }

    if(left_len) {
        Wire1.beginTransmission(deviceAddress);
        Wire1.write(BURST_FIFO_READ);
        Wire1.endTransmission();
        Wire1.requestFrom(deviceAddress, left_len);
        count = 0;
        while(Wire1.available()){
            buff[count++] = Wire1.read();
            read_len++;
        }
    }
    client->write(buff, left_len);
    return read_len;
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
