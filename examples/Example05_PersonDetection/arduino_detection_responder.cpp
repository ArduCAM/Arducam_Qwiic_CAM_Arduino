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

#if defined(ARDUINO) && !defined(ARDUINO_ARCH_RP2040)
#define ARDUINO_EXCLUDE_CODE
#endif

#ifndef ARDUINO_EXCLUDE_CODE

#include <cmath>

#include "Arduino.h"
#include "detection_responder.h"
#include "tensorflow/lite/micro/micro_log.h"

void RespondToDetection(float person_score, float no_person_score) {

  float person_score_frac, person_score_int;
  float no_person_score_frac, no_person_score_int;
  person_score_frac = std::modf(person_score * 100, &person_score_int);
  no_person_score_frac = std::modf(no_person_score * 100, &no_person_score_int);

  // header for score transfer over serial port
  Serial.write(0xBB);
  // person score
  Serial.write(person_score_int);
  Serial.write(person_score_frac * 100);
//   MicroPrintf("Person score: %d.%d%% No person score: %d.%d%%",
//               static_cast<int>(person_score_int),
//               static_cast<int>(person_score_frac * 100),
//               static_cast<int>(no_person_score_int),
//               static_cast<int>(no_person_score_frac * 100));
}

#endif  // ARDUINO_EXCLUDE_CODE
