/* Copyright 2019 The TensorFlow Authors. All Rights Reserved.

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

#include "imu_handler.h"

#include <Arduino.h>
#include "ICM_20948.h"
#include "constants.h"

#define AD0_VAL   1

ICM_20948_I2C myICM;

// A buffer holding the last 200 sets of 3-channel values
float save_data[600] = {0.0};
// Most recent position in the save_data buffer
int begin_index = 0;
// True if there is not yet enough data to run inference
bool pending_initial_data = true;
// How often we should save a measurement during downsampling
int sample_every_n;
// The number of measurements since we last saved one
int sample_skip_counter = 1;

TfLiteStatus InitIMU(tflite::ErrorReporter* error_reporter) {
  // Wait until we know the serial port is ready
  while (!Serial) {
  }

  // Switch on the IMU
  Wire.begin();
  Wire.setClock(400000);
  myICM.begin( Wire, AD0_VAL );

  if ( myICM.status != ICM_20948_Stat_Ok ) {
    error_reporter->Report("Failed to initialize IMU");
    return kTfLiteError;
  }

  error_reporter->Report( myICM.statusString() );
  enableBurstMode();


  ICM_20948_smplrt_t mySmplrt;
  mySmplrt.g = 43; // 25 Hz
  myICM.setSampleRate( ICM_20948_Internal_Gyr, mySmplrt );
  Serial.println(myICM.statusString());

  ICM_20948_Status_e accDLPEnableStat = myICM.enableDLPF( ICM_20948_Internal_Acc, true );
  ICM_20948_Status_e gyrDLPEnableStat = myICM.enableDLPF( ICM_20948_Internal_Gyr, true );

  myICM.enableFifo(false);
  myICM.setFifoCfg();
  myICM.resetFifo(0x1f);
  myICM.enableFifoSlv(false);
  myICM.enableFifoAGT(false);
  delay(100);
  myICM.cleanupFifo();
  myICM.resetFifo(0x00);
  myICM.enableFifoAGT(true);
  myICM.setFifoMode(true);
  myICM.enableFifo(true);

  error_reporter->Report("Magic starts!");

  return kTfLiteOk;
}

bool ReadIMU(tflite::ErrorReporter* error_reporter, float* input, int length, bool reset_buffer) {
  // Clear the buffer if required, e.g. after a successful prediction
  if (reset_buffer) {
    memset(save_data, 0, 600 * sizeof(float));
    begin_index = 0;
    pending_initial_data = true;
  }

  myICM.getAGMT();
  float x = myICM.accX();
  float y = myICM.accY();
  float z = myICM.accZ();
  //Serial.printf("%f, %f, %f\n", x, y, z);

  // Write samples to our buffer, converting to milli-Gs
  // and flipping y and x order for compatibility with
  // model (sensor orientation is different on Arduino
  // Nano BLE Sense compared with SparkFun Edge)
  save_data[begin_index++] = x * 1000;
  save_data[begin_index++] = y * 1000;
  save_data[begin_index++] = z * 1000;

  // If we reached the end of the circle buffer, reset
  if (begin_index >= 600) {
    begin_index = 0;
  }


  // Check if we are ready for prediction or still pending more initial data
  if (pending_initial_data && begin_index >= 200) {
    pending_initial_data = false;
  }

  // Return if we don't have enough data
  if (pending_initial_data) {
    return false;
  }

  // Copy the requested number of bytes to the provided input tensor
  for (int i = 0; i < length; ++i) {
    int ring_array_index = begin_index + i - length;
    if (ring_array_index < 0) {
      ring_array_index += 600;
    }
    input[i] = save_data[ring_array_index];
  }

  return true;
}
