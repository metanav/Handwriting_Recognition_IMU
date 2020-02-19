#include <EEPROM.h>

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
}
