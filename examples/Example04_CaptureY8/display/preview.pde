/*
  This sketch reads a raw Stream of Y8 pixels
 from the Serial port and displays the frame on
 the window.
 
 Use with the Examples -> Example04_CaptureY8
 
 */

import processing.serial.*;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;

Serial myPort;

// must match resolution used in the sketch
final int cameraWidth = 96;
final int cameraHeight = 96;
final int cameraBytesPerPixel = 1;
final int bytesPerFrame = cameraWidth * cameraHeight * cameraBytesPerPixel;

PImage myImage;
byte[] frameBuffer = new byte[bytesPerFrame];

int state = 0;
int read = 0;
int capture_done = 0;

void setup()
{
  size(96, 96);

  // if you have only ONE serial port active
  myPort = new Serial(this, Serial.list()[0], 115200);          // if you have only ONE serial port active

  // if you know the serial port name
  // myPort = new Serial(this, "COM10", 115200);                    // Windows
  // myPort = new Serial(this, "/dev/ttyUSB0", 115200);             // Linux
  // myPort = new Serial(this, "/dev/cu.usbmodem14401", 115200);    // Mac

  // wait for full frame of bytes
  myPort.buffer(bytesPerFrame);  
  myImage = createImage(cameraWidth, cameraHeight, GRAY);
  fill(255, 0, 0);
  
  //capture
  myPort.write(0x10);
}

void draw()
{
  if(capture_done == 1){
    capture_done = 0;
    myPort.write(0x10);
  }
  image(myImage, 0, 0, myImage.width, myImage.height);
}
 //<>//
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
         // read 16-bit pixel
        short p = bb.getShort();
        int p1 = (p>>8)&0xFF;
        int p2 = p&0xFF;
        // convert RGB565 to RGB 24-bit
        int r = p1;
        int g = p1;
        int b = p1;
        // set pixel color
        myImage .pixels[i++] = color(r, g, b);
        r = p2;
        g = p2;
        b = p2;
        // set pixel color
        myImage .pixels[i++] = color(r, g, b);
      }
      myImage.updatePixels();
      read = 0;
    }
  capture_done = 1;
}
