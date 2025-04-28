/*
  Example 1: Video Streaming Web Server

  This example demonstrates how to use the Arducam Qwiic CAM with the Arduino Uno R4 WIFI.
  This demo was made for Arducam Qwiic Camera.
  It allows you to stream the camera image over WiFi.

  License: MIT License (https://en.wikipedia.org/wiki/MIT_License)
  Web: http://www.ArduCAM.com
  Date: 2025/4/28

  Hardware Connections:
  QWIIC --> QWIIC

  SSID: Arducam_Qwiic_CAM
  Password: 123456789
  IP: 192.168.4.1
 */
#include "Arducam_Qwiic_CAM.h"
#include <WiFiS3.h>

uint32_t imageLength = 0;
uint8_t imageBuf[READ_IMAGE_LENGTH];

Arducam_Qwiic_CAM myCAM;

// WiFi config
const char* ssid = "Arducam_Qwiic_CAM";       
const char* pass = "123456789"; 
WiFiServer server(80); 
int status = WL_IDLE_STATUS;
uint8_t length;

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

  // check for the WiFi module:
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Communication with WiFi module failed!");
    while (true);
  }

  // set the local IP address
  WiFi.config(IPAddress(192,168,4,1));
  Serial.print("Creating access point named: ");
  Serial.println(ssid);

  // create open network
  status = WiFi.beginAP(ssid, pass);
  if (status != WL_AP_LISTENING) {
    Serial.println("Creating access point failed");
    while (true);
  }
  // start the web server on port 80
  server.begin();
  printWiFiStatus();
}

void loop() {
  WiFiClient client = server.available();

  if (client) {
    Serial.println("Device connect!");
    
    // HTTP headers
    client.print(
      "HTTP/1.1 200 OK\r\n"
      "Content-Type: multipart/x-mixed-replace; boundary=frame\r\n"
      "Connection: keep-alive\r\n"
      "\r\n"
    );

    while (client.connected()) {
      delayMicroseconds(10);
      // capture
      myCAM.takePicture(CAM_IMAGE_MODE_QVGA, CAM_IMAGE_PIX_FMT_JPG);
      imageLength = myCAM.totalLength;

      //send frame header
      client.print(
        "--frame\r\n"
        "Content-Type: image/jpeg\r\n"
        "Content-Length: " + String(imageLength) + "\r\n"
        "\r\n"
      );
      
      // send image data
      while(myCAM.unreceivedLength) {
        length = myCAM.readImageBuff(imageBuf, READ_IMAGE_LENGTH);
        client.write(imageBuf, length);
      }
      client.print("\r\n");

      // refresh output buffer
      client.flush();
    
      if (client.available()) {
        while(client.available()) client.read();
        break;
      }
    }
  
    client.stop();
    Serial.println("Device disconnect!");
  }
}

void printWiFiStatus() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your WiFi shield's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print where to go in a browser:
  Serial.print("To see this page in action, open a browser to http://");
  Serial.println(ip);
}
