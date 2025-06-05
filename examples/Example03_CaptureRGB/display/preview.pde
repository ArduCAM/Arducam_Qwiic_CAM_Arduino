/*
  This sketch reads a raw Stream of RGB565 pixels
 from the Serial port and displays the frame on
 the window.
 
 Use with the Examples -> Example03_CaptureRGB.

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
float fps = 0;
int frameCount = 0;
long lastTime = 0;

// must match resolution used in the sketch
final int cameraWidth = (int)res.x;
final int cameraHeight = (int)res.y;
final int cameraBytesPerPixel = 2;
final int bytesPerFrame = cameraWidth * cameraHeight * cameraBytesPerPixel;

PImage myImage;
byte[] frameBuffer = new byte[bytesPerFrame];

int state = 0;
int read = 0;
int capture_done = 0;

void setup()
{
  //size(96, 96);
  surface.setSize((int)res.x, (int)res.y);
  // if you have only ONE serial port active
  myPort = new Serial(this, Serial.list()[0], BAUD_RATE);          // if you have only ONE serial port active

  // if you know the serial port name
  // myPort = new Serial(this, "COM10", BAUD_RATE);                   // Windows
  // myPort = new Serial(this, "/dev/ttyUSB0", BAUD_RATE);            // Linux
  // myPort = new Serial(this, "/dev/cu.usbmodem14401", BAUD_RATE);   // Mac

  // wait for full frame of bytes
  myPort.buffer(bytesPerFrame);  
  myImage = createImage(cameraWidth, cameraHeight, RGB);
  fill(255, 0, 0);

  // Set the size of the text
  textSize(16);
  textAlign(RIGHT, TOP);
  
  //capture
  myPort.write(0x10);
  //skip first frame
  delay(100);
  myPort.write(0x10);
}

void draw()
{
  if(capture_done == 1){
    capture_done = 0;
    myPort.write(0x10);
  }
  image(myImage, 0, 0,cameraWidth, cameraHeight);

  // Update frame rate display
  updateFPS();
  
  // Display the frame rate
  fill(255, 0, 0);
  text("FPS: " + nf(fps, 0, 1), width - 10, 10);
}

void updateFPS() {
  long currentTime = millis();
  long elapsed = currentTime - lastTime;
  
  if (elapsed >= 1000) {
    fps = frameCount * 1000.0 / elapsed;
    frameCount = 0;
    lastTime = currentTime;
  }
}

void serialEvent(Serial myPort) {
    if (read == 0) {
      int incoming = myPort.read();
      if (incoming == 0x55 && state == 0) {
        state = 1;
      } else if (incoming == 0xAA && state == 1) {
        read = 1;
        state = 0;
      } else {
        state = 0;
      }
    } 
    else if (read == 1) {
      // read the saw bytes in
      myPort.readBytes(frameBuffer);
     // access raw bytes via byte buffer
      ByteBuffer bb = ByteBuffer.wrap(frameBuffer);
      bb.order(ByteOrder.BIG_ENDIAN);
      int i = 0;
      while (bb.hasRemaining()) {
        short p = bb.getShort();
        int r = ((p >> 11) & 0x1f) << 3;
        int g = ((p >> 5) & 0x3f) << 2;
        int b = ((p >> 0) & 0x1f) << 3;
        myImage.pixels[i++] = color(r, g, b);
      }
      myImage.updatePixels();
      read = 0;
      frameCount++;
    }
  capture_done = 1;
}
