/*
   This sketch is used for collecting handwritten digits data.
*/
#include <EEPROM.h>
#include <SPI.h>
#include <SdFat.h>
#include <GD2.h>
#include "ICM_20948.h"

#define MECH_CAP_BUTTON_PIN 36
#define TAG_BUTTON_CANCEL 201
#define AD0_VAL   1

// Global declaration
ICM_20948_I2C myICM;
SdFat sd;
SdFile file;

char filename[50];

// message to be shown on the LCD
char message1[30];
char message2[30];
char digit = 48;
bool cancel = false;
int  samples = 0;
int  file_count;
String buf;
int examples_count[10] = {0};

// This function is used to get input and update display user interface
void touchscreen_get_input() {
  GD.get_inputs();

  byte key = GD.inputs.tag;
  if (key >= 48 && key <= 57) {
    digit = key;
  }

  switch (GD.inputs.tag) {
    case TAG_BUTTON_CANCEL:
      cancel = true;
      break;
    default:
      cancel = false;
  }

  sprintf(message1, "Digit: %c Count=%d", digit, examples_count[digit -'0']);

  GD.cmd_gradient(0, 0, 0x404044, 480, 480, 0x606068);
  GD.ColorRGB(0x707070);
  GD.LineWidth(4 * 16);
  GD.Begin(RECTS);
  GD.Vertex2ii(350, 8);
  GD.Vertex2ii(350 + 120, 128);
  GD.Vertex2ii(8, 136 + 8);
  GD.Vertex2ii(350 + 120, 136 + 128);
  GD.ColorRGB(0xffffff);
  GD.Tag(TAG_BUTTON_CANCEL);
  GD.cmd_button(350, 8, 120, 120, 29, 0,  "CANCEL");
  GD.cmd_keys(8, 185, 450, 50, 31, 0 | OPT_CENTER | digit, "0123456789");
  GD.cmd_text(8, 10, 31, 0, message1);
  GD.ColorRGB(0xffffff);
  GD.cmd_text(8, 60, 31, 0, message2);
  GD.BlendFunc(SRC_ALPHA, ZERO);
  GD.swap();
}

void setup()
{
  Serial.begin(115200);

  while ( !Serial ) { };

  Wire.begin();
  Wire.setClock(400000);

  pinMode(MECH_CAP_BUTTON_PIN, INPUT_PULLUP);

  bool initialized = false;

  while ( !initialized ) {
    myICM.begin( Wire, AD0_VAL );

    Serial.print( F("Initialization of the sensor returned: ") );
    Serial.println( myICM.statusString() );

    if ( myICM.status != ICM_20948_Stat_Ok ) {
      Serial.println( "Trying again..." );
      delay(500);
    } else {
      initialized = true;
    }
  }

  enableBurstMode();

  //  ICM_20948_smplrt_t mySmplrt;
  //  mySmplrt.g = 21; // 50 Hz, 1.1 kHz/(1+GYRO_SMPLRT_DIV[7:0])
  //  myICM.setSampleRate( ICM_20948_Internal_Gyr, mySmplrt );
  Serial.println(myICM.statusString());
  myICM.enableDLPF( ICM_20948_Internal_Acc, true );
  myICM.enableDLPF( ICM_20948_Internal_Gyr, true );
  delay(100);

  if (!sd.begin(A12, SD_SCK_MHZ(20))) {
    sd.initErrorHalt();
  }

  GD.begin(~GD_STORAGE);

  EEPROM.get(0, file_count);
  if (file_count < 0 || file_count > 10000) {
    file_count = 0;
    EEPROM.put(0, file_count);
  }
  
  
}

void loop()
{
  touchscreen_get_input();

  bool mech_cap_btn_status = digitalRead(MECH_CAP_BUTTON_PIN);

  if (mech_cap_btn_status == HIGH) {
    if (samples == 0) {
      strcpy(message2, "Status: Started");
      Serial.println(message2);
      buf = "";
    }

    if ( myICM.dataReady() ) {
      myICM.getAGMT();
      Serial.printf( "%04.2f\t%04.2f\t%04.2f\n", myICM.accX(), myICM.accY(), myICM.accZ() );
      buf += String(myICM.accX(), 2);
      buf += F(",");
      buf += String(myICM.accY(), 2);
      buf += F(",");
      buf += String(myICM.accZ(), 2);
      buf += F(",");
      buf += String(myICM.gyrX(), 2);
      buf += F(",");
      buf += String(myICM.gyrY(), 2);
      buf += F(",");
      buf += String(myICM.gyrZ(), 2);
      buf += F("\n");

      samples++;

      delay(30);
    } else {
      Serial.println("Waiting for data");
      delay(500);
    }
  }

  if (mech_cap_btn_status == LOW) {
    if (samples > 0) {
      strcpy(message2, "Status: Saved");

      GD.__end();

      Serial.println(message2);
      EEPROM.get(0, file_count);
      
      sprintf(filename, "readings_%d_digit_%c.csv", file_count, digit);

      if (!file.open(filename, O_WRONLY | O_CREAT | O_EXCL )) {
        sd.errorHalt(filename);
      }

      file.print(buf);
      file.close();
      //Serial.print(buf);

      Serial.print("Saved to: ");
      Serial.println(filename);
      
      file_count++;
      EEPROM.put(0, file_count);

      GD.resume();

      samples = 0;
      examples_count[digit -'0']++;
    }
  }

  if (cancel == true) {
    if (sd.remove(filename)) {
      strcpy(message2, "Status: deleted.");
      examples_count[digit -'0']--;
    } else {
      Serial.print("Error: cannot delete "); Serial.println(filename);
    }
  }

}
