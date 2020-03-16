#include <EEPROM.h>

/*
  This sketch resets the counter which is used to count digits during training.
  Artemis doesn't actually have EEPROM. Instead we are writing to a protected
  section of flash.
*/

void setup()
{
  Serial.begin(115200);
  Serial.println("Reset Counter");
  
  int value = 0;
  EEPROM.put(0, value);
  int response;
  EEPROM.get(0, response);
  Serial.printf("Location %d should be %d: %d\n\r", 0, value, response);
}

void loop()
{
  // Do nothing
}
