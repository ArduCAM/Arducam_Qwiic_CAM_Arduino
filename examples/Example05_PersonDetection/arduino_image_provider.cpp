/* Copyright 2022 The TensorFlow Authors. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/

#if defined(ARDUINO) && !defined(ARDUINO_ARCH_RP2040) && !defined(ARDUINO_PORTENTA_C33)
#define ARDUINO_EXCLUDE_CODE
#endif

#ifndef ARDUINO_EXCLUDE_CODE

#include <algorithm>
#include <type_traits>

#include "image_provider.h"
#include "model_settings.h"
#include "tensorflow/lite/micro/micro_log.h"
#include "tensorflow/lite/micro/micro_utils.h"
#include "test_over_serial/test_over_serial.h"

#include "Arduino.h"
#include "Arducam_Qwiic_CAM.h"

using namespace test_over_serial;

namespace {

constexpr size_t kQQVGA_width = 96;   // pixels
constexpr size_t kQQVGA_height = 96;  // pixels

uint8_t image_buffer[kQQVGA_height * kQQVGA_width];
constexpr size_t kImageBufferLength =
    std::extent<decltype(image_buffer)>::value;

Arducam_Qwiic_CAM myCAM;

// Begin the capture and wait for it to finish
TfLiteStatus PerformCapture() {
  // This function kept for future implementation
  // MicroPrintf("Starting capture");
  myCAM.takePicture(CAM_IMAGE_MODE_96X96, CAM_IMAGE_PIX_FMT_Y8);
  
  // MicroPrintf("Image captured");
  return kTfLiteOk;
}

// Read data from the camera module into a local buffer
TfLiteStatus ReadData() {
  // This function kept for future implementation
  uint32_t length = 0;
  uint32_t offset = 0;
  while(myCAM.unreceivedLength) {
    length = myCAM.readImageBuff(image_buffer + offset, READ_IMAGE_LENGTH);
    offset += length;

  }
  return kTfLiteOk;
}

// Decode the image, crop it, and convert it to grayscale
TfLiteStatus CropAndQuantizeImage(size_t image_width, size_t image_height,
                                  const TfLiteTensor* tensor) {
  // MicroPrintf("Cropping image and quantizing");

  // cropping parameters
  const size_t vert_top = (image_height - kNumRows) / 2;
  const size_t vert_bottom = vert_top + kNumRows - 1;
  const size_t horz_left = (image_width - kNumCols) / 2;
  const size_t horz_right = horz_left + kNumCols - 1;

  const uint8_t* p = image_buffer + (vert_top * image_width);
  p += horz_left;
  int8_t* image_data = tensor->data.int8;
  for (size_t line = vert_top; line <= vert_bottom; line++) {
    for (size_t row = horz_left; row <= horz_right; row++, p++) {
      *image_data++ = tflite::FloatToQuantizedType<int8_t>(
          p[0] / 255.0f, tensor->params.scale, tensor->params.zero_point);
    }
    // move to next line
    p += ((image_width - 1) - horz_right) + horz_left;
  }

  // MicroPrintf("Image cropped and quantized");
  return kTfLiteOk;
}

// Get an image from the camera module
TfLiteStatus GetCameraImage(const TfLiteTensor* tensor) {
 
  TfLiteStatus capture_status = PerformCapture();
  if (capture_status != kTfLiteOk) {
    MicroPrintf("PerformCapture failed");
    return capture_status;
  }

  TfLiteStatus read_data_status = ReadData();
  if (read_data_status != kTfLiteOk) {
    MicroPrintf("ReadData failed");
    return read_data_status;
  }
  
  // header for image transfer over serial port
  Serial.write(0x55);
  Serial.write(0xAA);
  // image data
  Serial.write(image_buffer, kImageBufferLength);

  TfLiteStatus decode_status =
      CropAndQuantizeImage(kQQVGA_width, kQQVGA_height, tensor);
  if (decode_status != kTfLiteOk) {
    MicroPrintf("CropAndQuantizeImage failed");
    return decode_status;
  }

  return kTfLiteOk;
}

TfLiteStatus GetTestImage(TestOverSerial& test, const TfLiteTensor* tensor) {
  volatile bool done = false;
  volatile bool aborted = false;
  volatile size_t image_width = 0, image_height = 0;

  InputHandler handler = [&aborted, &done, &image_width,
                          &image_height](const InputBuffer* const input) {
    if (0 == input->offset) {
      if ((kQQVGA_height * kQQVGA_width) == input->total) {
        image_width = kQQVGA_width;
        image_height = kQQVGA_height;
      } else if ((kNumCols * kNumRows) == input->total) {
        image_width = kNumCols;
        image_height = kNumRows;
      } else {
        // image dimensions are not supported, abort input processing
        aborted = true;
        return false;
      }
    }

    std::copy_n(input->data.uint8, input->length, &image_buffer[input->offset]);
    if (input->total == (input->offset + input->length)) {
      done = true;
    }
    return true;
  };

  while (!done) {
    test.ProcessInput(&handler);
    if (aborted) {
      MicroPrintf("Input processing aborted");
      return kTfLiteError;
    }
    // wait for a full image from serial port before processing
    if (done) {
      TfLiteStatus decode_status =
          CropAndQuantizeImage(image_width, image_height, tensor);
      if (decode_status != kTfLiteOk) {
        MicroPrintf("CropAndQuantizeImage failed");
        return decode_status;
      }
    }
  }

  return kTfLiteOk;
}

}  // namespace

// Get the camera module ready
TfLiteStatus InitCamera() {
  // This function kept for future implementation
  if(myCAM.begin()){
    MicroPrintf("camera not yet supported.");
    return kTfLiteError;
  }else{
    MicroPrintf("Qwiic camera initialized.");
  }
  return kTfLiteOk;
}

TfLiteStatus GetImage(const TfLiteTensor* tensor) {
    // get an image from the camera
    return GetCameraImage(tensor);
}

#endif  // ARDUINO_EXCLUDE_CODE
