/*
  This sketch reads a raw Stream of jpeg pixels
 from the Serial port and displays the frame on
 the window.
 
 Use with the Examples -> Example02_CaptureJPEG
 
 */
import processing.serial.*;
import javax.imageio.ImageIO;
import java.awt.image.BufferedImage;
import java.io.ByteArrayInputStream;
import java.io.IOException;
import java.util.Arrays;

// Set the baud rate to 921600 or 115200
final int BAUD_RATE = 115200;
//final int BAUD_RATE = 921600;

// Set the resolution of the display, need match resolution
int currentMode = 0;        // 0:QVGA 1:VGA 2:HD 3:UXGA 4:96x96 5:128x128 6:320x320
//int currentMode = 1;
//int currentMode = 2;
//int currentMode = 3;
//int currentMode = 4;
//int currentMode = 5;
//int currentMode = 6;

final PVector[] RESOLUTIONS = {
  new PVector(320, 240),    // QVGA
  new PVector(640, 480),    // VGA
  new PVector(1280, 720),   // HD
  new PVector(1600, 1200),  // UXGA
  new PVector(96, 96),      // 96x96
  new PVector(128, 128),    // 128x128
  new PVector(320, 320)     // 320x320
};
PVector res = RESOLUTIONS[currentMode];

Serial myPort;
byte[] buffer = new byte[0];
float fps = 0;
int frameCount = 0;
long lastTime = 0;

void setup() {
  //size(320, 240); 
  surface.setSize((int)res.x, (int)res.y);
  myPort = new Serial(this, Serial.list()[0], BAUD_RATE); // if you have only ONE serial port active

  // if you know the serial port name
  // myPort = new Serial(this, "COM10", BAUD_RATE);                    // Windows
  // myPort = new Serial(this, "/dev/ttyUSB0", BAUD_RATE);             // Linux
  // myPort = new Serial(this, "/dev/cu.usbmodem14401", BAUD_RATE);    // Mac
  
  // Set the size of the text
  textSize(16);
  textAlign(RIGHT, TOP);
}

void draw() {
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

void serialEvent(Serial port) {
  try {
    byte[] newData = port.readBytes();
    if (newData != null) {
      buffer = concatArrays(buffer, newData);
      processBuffer();
    }
  } 
  catch (Exception e) {
    e.printStackTrace();
  }
}

void processBuffer() {
  while (true) {
    int[] markers = findMarkers();
    int start = markers[0];
    int end = markers[1];

    if (start == -1 || end == -1) break;
    byte[] imageData = Arrays.copyOfRange(buffer, start, end + 1);
    displayImage(imageData);
    buffer = Arrays.copyOfRange(buffer, end + 1, buffer.length);
  }
}

int[] findMarkers() {
  int start = -1;
  int end = -1;

  // Look for the header of the jpeg data
  for (int i = 0; i < buffer.length - 1; i++) {
    if (unsignedByte(buffer[i]) == 0xFF && unsignedByte(buffer[i+1]) == 0xD8) {
      start = i;
      break;
    }
  }

  if (start != -1) {
    // Look for the end of the jpeg data
    for (int j = start + 2; j < buffer.length - 1; j++) {
      if (unsignedByte(buffer[j]) == 0xFF && unsignedByte(buffer[j+1]) == 0xD9) {
        end = j + 1;
        break;
      }
    }
  }
  return new int[] {start, end};
}

void displayImage(byte[] data) {
  try {
    ByteArrayInputStream bais = new ByteArrayInputStream(data);
    BufferedImage bImg = ImageIO.read(bais);
    if (bImg != null) {
      int w = bImg.getWidth();
      int h = bImg.getHeight();
      
      // Display
      loadPixels();
      for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
          int argb = bImg.getRGB(x, y);
          int r = (argb >> 16) & 0xFF;
          int g = (argb >> 8) & 0xFF;
          int b = argb & 0xFF;
          pixels[y * width + x] = color(r, g, b);
        }
      }
      updatePixels();
      frameCount++;
    } else {
      println("Error: Image data cannot be parsed");
    }
  } 
  catch (IOException e) {
    println("IO error: " + e.getMessage());
  }
  catch (Exception e) {
    println("error: " + e.getMessage());
  }
}

byte[] concatArrays(byte[] a, byte[] b) {
  byte[] result = new byte[a.length + b.length];
  System.arraycopy(a, 0, result, 0, a.length);
  System.arraycopy(b, 0, result, a.length, b.length);
  return result;
}

int unsignedByte(byte b) {
  return b & 0xFF;
}
