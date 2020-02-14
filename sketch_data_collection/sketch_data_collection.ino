#include <EEPROM.h>
#include <SPI.h>
#include <SdFat.h>
#include <GD2.h>
#include "ICM_20948.h"

#define MECH_CAP_BUTTON_PIN 36
#define TAG_BUTTON_START 201
#define TAG_BUTTON_STOP  202
#define AD0_VAL   1

ICM_20948_I2C myICM;
SdFat sd;
SdFile file;

char message1[10];
char message2[20];
char filename[20];
char digit = 48;
char action = 0;
int  count = 0;
int  file_count = 0;
String buf;

void touchscreen_get_input() {
  GD.get_inputs();

  byte key = GD.inputs.tag;
  if (key >= 48 && key <= 57) {
    digit = key;
  }

  switch (GD.inputs.tag) {
    case TAG_BUTTON_START:
      action = 1;
      break;
    case TAG_BUTTON_STOP:
      action = 0;
      break;
  }

  sprintf(message1, "Digit: %c", digit);

  GD.cmd_gradient(0, 0, 0x404044, 480, 480, 0x606068);
  GD.ColorRGB(0x707070);

  GD.LineWidth(4 * 16);
  GD.Begin(RECTS);

  GD.Vertex2ii(350, 8);
  GD.Vertex2ii(350 + 120, 128);

  GD.Vertex2ii(350, 136 + 8);
  GD.Vertex2ii(350 + 120, 136 + 128);

  GD.Vertex2ii(8, 136 + 8);
  GD.Vertex2ii(320, 136 + 128);

  GD.ColorRGB(0xffffff);

  GD.Tag(TAG_BUTTON_START);
  GD.cmd_button(350, 8, 120, 120, 29, 0,  "START");

  GD.Tag(TAG_BUTTON_STOP);
  GD.cmd_button(350, 144, 120, 120, 29, 0,  "STOP");

  GD.cmd_keys(8, 185, 300, 40, 30, 0 | OPT_CENTER | key, "0123456789");

  GD.cmd_text(8, 10, 31, 0, message1);

  if (action == 1) {
    GD.ColorRGB(0x00ffee);
  } else {
    GD.ColorRGB(0xffffff);
  }
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

  if (!sd.begin(A12, SD_SCK_MHZ(10))) {
    sd.initErrorHalt();
  }

  GD.begin(~GD_STORAGE);
  
}

void loop()
{
  touchscreen_get_input();
  bool mech_cap_btn_status = digitalRead(MECH_CAP_BUTTON_PIN);

  if (mech_cap_btn_status == HIGH) {
    if (count == 0) {
      strcpy(message2, "Status: Started");
      Serial.println(message2);
      buf = "";
    }

    if ( myICM.dataReady()) {
      myICM.getAGMT();
      //Serial.printf( "%04.2f\t%04.2f\t%04.2f\n", myICM.accX(), myICM.accY(), myICM.accZ() );
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

      count++;

      delay(30);
    } else {
      Serial.println("Waiting for data");
      delay(500);
    }
  }

  if (mech_cap_btn_status == LOW) {
    if (count > 0) {
      strcpy(message2, "Status: Stopped");

      GD.__end();

      Serial.println(message2);
      file_count++;
      sprintf(filename, "readings_%d_digit_%c.csv", file_count, digit);
//
//      if (!file.open(filename, O_WRONLY | O_CREAT | O_EXCL )) {
//        sd.errorHalt(filename);
//      }
//      file.print(buf);
//      file.close();

     Serial.print(buf);

      Serial.print("Saved to: ");
      Serial.println(filename);

      GD.resume();

      count = 0;
    }
  }
}
