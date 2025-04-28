/*
  Example 2: Capture JPEG

  This example demonstrates how to use the Arducam Qwiic CAM.
  This demo was made for Arducam Qwiic Camera.
  It allows you to stream the camera image over Serial.

  License: MIT License (https://en.wikipedia.org/wiki/MIT_License)
  Web: http://www.ArduCAM.com
  Date: 2025/4/28

  Hardware Connections:
  QWIIC -. QWIIC

  The resolutions supported by images in JPEG format are as follows:
  CAM_IMAGE_MODE_QVGA   
  CAM_IMAGE_MODE_VGA    
  CAM_IMAGE_MODE_HD     
  CAM_IMAGE_MODE_UXGA   
  CAM_IMAGE_MODE_96X96  
  CAM_IMAGE_MODE_128X128
  CAM_IMAGE_MODE_320X320

 */
#include "Arducam_Qwiic_CAM.h"

uint32_t imageLength = 0;
uint8_t length;
uint8_t imageBuf[READ_IMAGE_LENGTH];
Arducam_Qwiic_CAM myCAM;

void setup() {
  Serial.begin(115200);
  while (!Serial);  // wait for serial port to connect

  // camera init
  if (myCAM.begin()) {
    Serial.println("camera init failed!");
    while (true);
  }
  Serial.println("camera init success!");

  //camera detect
  while(1){
    QWIIC_WIRE.beginTransmission(myCAM.deviceAddress);
    if(QWIIC_WIRE.endTransmission() == 0){
      Serial.println("camera detect");
      break;
    }else{
      Serial.println("camera not detect");
      delay(100);
    }
  }

}

void loop() {

  myCAM.takePicture(CAM_IMAGE_MODE_QVGA, CAM_IMAGE_PIX_FMT_JPG);
  cameraGetPicture();
  delay(10);
}

void cameraGetPicture()
{
  // send image data
  while(myCAM.unreceivedLength) {
    length = myCAM.readImageBuff(imageBuf, READ_IMAGE_LENGTH);
    Serial.write(imageBuf, length);
  }
}



