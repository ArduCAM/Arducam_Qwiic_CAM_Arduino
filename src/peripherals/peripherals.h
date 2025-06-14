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

#ifndef PERIPHERALS_H_
#define PERIPHERALS_H_

#ifdef ARDUINO
#include <Arduino.h>
#include <Wire.h>
#else  // ARDUINO
#error "unsupported framework"
#endif  // ARDUINO

#if defined(ARDUINO_ARCH_RP2040) || defined(ARDUINO_PORTENTA_C33)
#include "utility.h"
#else
#error "unsupported board"
#endif

#endif  // PERIPHERALS_H_
