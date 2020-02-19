#include "output_handler.h"
#include "Arduino.h"
#include <SPI.h>
#include <GD2.h>
#include "default_assets.h"

bool gd_initilized = false;

void HandleOutput(tflite::ErrorReporter* error_reporter, int kind) {
  // The first time this method runs, set up our LED
  static bool is_initialized = false;
  if (!is_initialized) {
    pinMode(LED_BUILTIN, OUTPUT);
    is_initialized = true;
  }
  // Toggle the LED every time an inference is performed
  static int count = 0;
  ++count;
  if (count & 1) {
    digitalWrite(LED_BUILTIN, HIGH);
  } else {
    digitalWrite(LED_BUILTIN, LOW);
  }
   
  if (!gd_initilized) {
    GD.begin(0);
    LOAD_ASSETS();
    gd_initilized = true;
    error_reporter->Report("Begin GD\n");
  }

  error_reporter->Report("%d", kind);
  
  char str[8];
  
  if (kind == -1) {
    strcpy(str, "???");
  } else {
    sprintf(str, "%d", kind);
  }

  
  GD.ClearColorRGB(0xff0000);
  GD.Clear();
  GD.cmd_text(GD.w / 2, GD.h / 2, COMICSANS_HANDLE, OPT_CENTER, str);
  GD.swap();
   
}
