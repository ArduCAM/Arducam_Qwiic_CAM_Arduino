/* //<>//
  This sketch reads a raw Stream of Y8 pixels
 from the Serial port and displays the frame on
 the window.
 
 Use with the Examples -> Example05_PersonDetection
*/

import processing.serial.*;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;

// Set the baud rate to 921600 or 115200
final int BAUD_RATE = 115200;
//final int BAUD_RATE = 921600;

// Set the resolution of the display, need match resolution
int currentMode = 0;        // 0:96x96 1:128x128
//int currentMode = 1;

final PVector[] RESOLUTIONS = {
  new PVector(96, 96),      // 96x96
  new PVector(128, 128),    // 128x128
};
PVector res = RESOLUTIONS[currentMode];

Serial myPort;

// must match resolution used in the sketch
final int cameraWidth = (int)res.x;
final int cameraHeight = (int)res.y;
final int cameraBytesPerPixel = 1;
final int bytesPerFrame = cameraWidth * cameraHeight * cameraBytesPerPixel;

PImage myImage;
byte[] frameBuffer = new byte[bytesPerFrame];
byte[] scoreData = new byte[2];
byte[] currentScore = new byte[2];
boolean hasScore = false;

int state = 0;
int read = 0;
int result = 0;

void setup() {
  //size(96, 96);
  surface.setSize((int)res.x, (int)res.y);
  // if you have only ONE serial port active
  myPort = new Serial(this, Serial.list()[0], BAUD_RATE);      
  
  // if you know the serial port name
  // myPort = new Serial(this, "COM10", BAUD_RATE);                    // Windows
  // myPort = new Serial(this, "/dev/ttyUSB0", BAUD_RATE);             // Linux
  // myPort = new Serial(this, "/dev/cu.usbmodem14401", BAUD_RATE);    // Mac

  // wait for full frame of bytes
  myPort.buffer(bytesPerFrame);  
  myImage = createImage(cameraWidth, cameraHeight, GRAY);
  fill(255, 0, 0);
  textSize(16);
}

void draw() {
  image(myImage, 0, 0, myImage.width, myImage.height);

  if (hasScore) {
    int integerPart = currentScore[0] & 0xFF;
    int decimalPart = currentScore[1] & 0xFF;
   
    String decimalStr = nf(decimalPart, 2);
    String percentage = integerPart + "." + decimalStr + "%";

    fill(255, 0, 0);
    textAlign(RIGHT, TOP);
    text(percentage, width - 2, 0);
  }
}

void serialEvent(Serial myPort) {
  if (read == 0) {
    int incoming = myPort.read();
    if (incoming == 0x55 && state == 0) {
      state = 1;
    } else if (incoming == 0xAA && state == 1) {
      read = 1;
    } else if (incoming == 0xBB && state == 1) {
      state = 0;
      result = 1;
      read = 2; 
      myPort.buffer(2);
    }
  } else if (read == 1) {
    // read image data
    myPort.readBytes(frameBuffer);
    ByteBuffer bb = ByteBuffer.wrap(frameBuffer);
    bb.order(ByteOrder.BIG_ENDIAN);
    int i = 0;
    while (bb.hasRemaining()) {
      short p = bb.getShort();
      int p1 = (p >> 8) & 0xFF;
      int p2 = p & 0xFF;
      myImage.pixels[i++] = color(p1, p1, p1);
      myImage.pixels[i++] = color(p2, p2, p2);
    }
    myImage.updatePixels();
    read = 0;
  } else if (read == 2) {
    // read score data
    myPort.readBytes(scoreData);
    if (scoreData != null && scoreData.length == 2) {
      currentScore[0] = scoreData[0];
      currentScore[1] = scoreData[1];
      hasScore = true;
      // restore the image data buffer
      myPort.buffer(bytesPerFrame);
      read = 0;
      result = 0;
    }
  }
}
