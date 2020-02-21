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

// A buffer holding the last 100 sets of 6-channel values
float save_data[600] = {0.0};
// Most recent position in the save_data buffer
int begin_index = 0;
// True if there is not yet enough data to run inference
bool pending_initial_data = true;

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
   
  // enable low pass filter
  myICM.enableDLPF( ICM_20948_Internal_Acc, true );
  myICM.enableDLPF( ICM_20948_Internal_Gyr, true );

  // disable fifo at the beginning, later enable it
  myICM.enableFifo(false);
  myICM.setFifoCfg();

  // reset fifo
  myICM.resetFifo(0x1f);
  myICM.enableFifoSlv(false);
  myICM.enableFifoAGT(false);
  delay(10);
  myICM.cleanupFifo();
  myICM.resetFifo(0x00);

  // enable fifo
  myICM.enableFifoAGT(true);
  myICM.setFifoMode(true);
  myICM.enableFifo(true);

  error_reporter->Report("IMU Initialized");
  delay(100);
  return kTfLiteOk;
}

bool ReadIMU(tflite::ErrorReporter* error_reporter, float* input, int length, bool reset_buffer) {
  // Clear the buffer if required, e.g. after a successful prediction
  if (reset_buffer) {
    memset(save_data, 0, 600 * sizeof(float));
    begin_index = 0;
    pending_initial_data = true;
  }

  int fifo_count = myICM.getFifoCount();
  int samples    =  fifo_count / 14; // 6 (acc) + 6 (gyr)  + 2 (temp)
  Serial.printf("samples=%d\n", samples);
  if (samples == 0) {
    return false;
  }
  for (int i = 0; i < samples; i++) {
    myICM.getAGMT(true); // pass true to get data from FIFO

    float ax = myICM.accX();
    float ay = myICM.accY();
    float az = myICM.accZ();
    float gx = myICM.gyrX();
    float gy = myICM.gyrY();
    float gz = myICM.gyrZ();
    //Serial.printf("[%f, %f, %f,%f, %f, %f]\n", ax, ay, az, gx, gy, gz);

    // Write samples to  buffer
    save_data[begin_index++] = ax;
    save_data[begin_index++] = ay;
    save_data[begin_index++] = az;
    save_data[begin_index++] = gx;
    save_data[begin_index++] = gy;
    save_data[begin_index++] = gz;

    // If we reached the end of the circle buffer, reset
    if (begin_index >= 600) {
      begin_index = 0;
    }

   // delay(1);
  }

  // Check if we are ready for prediction or still pending more initial data
  if (pending_initial_data && begin_index >= 216) {
    pending_initial_data = false;
  }

  // Return if we don't have enough data
  if (pending_initial_data) {
    return false;
  }

  //Serial.printf("begin_index=%d\n", begin_index);

  // Copy the requested number of bytes to the provided input tensor
  for (int i = 0; i < length; ++i) {
    int ring_array_index = begin_index + i - length;
    if (ring_array_index < 0) {
      ring_array_index += 600;
    }
    
    input[i] = save_data[ring_array_index];
    
//    Serial.print(input[i]);
//    
//    if ( (i + 1) % 6 == 0 ) {
//       Serial.println("");
//    } else {
//      Serial.print(", ");
//    }
  }

  return true;
}
