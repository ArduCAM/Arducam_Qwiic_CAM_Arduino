/*
  Example 4: Capture Y8

  This example demonstrates how to use the Arducam Qwiic CAM.
  This demo was made for Arducam Qwiic Camera.
  It allows you to stream the camera image over Serial.

  License: MIT License (https://en.wikipedia.org/wiki/MIT_License)
  Web: http://www.ArduCAM.com
  Date: 2025/4/28

  Hardware Connections:
  QWIIC -. QWIIC

  The resolutions supported by images in Y8 format are as follows:
  CAM_IMAGE_MODE_96X96  
  CAM_IMAGE_MODE_128X128

 */
#include "Arducam_Qwiic_CAM.h"

#define SET_CAPTURE 0x10

uint32_t imageLength = 0;
uint8_t length;
uint8_t imageBuf[READ_IMAGE_LENGTH];
Arducam_Qwiic_CAM myCAM;

void setup() {
  
  Serial.begin(115200);
  // camera init
  if (myCAM.begin()) {
    Serial.println("camera init failed!");
    while (true);
  }
  Serial.println("camera init success!");

}

void loop() {

  if(Serial.available()){
    if(Serial.read() == SET_CAPTURE){
      myCAM.takePicture(CAM_IMAGE_MODE_96X96, CAM_IMAGE_PIX_FMT_Y8);
      cameraGetPicture();
    }
  }
  
}

void cameraGetPicture()
{
  // send header
  Serial.write(0x55);
  Serial.write(0xAA);
  
  // send image data
  while(myCAM.unreceivedLength) {
    length = myCAM.readImageBuff(imageBuf, READ_IMAGE_LENGTH);
    Serial.write(imageBuf, length);
  }
}



