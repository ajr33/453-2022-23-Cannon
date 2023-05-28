/*
Modified receiver code that parses the input data string into x, y, and z floats.
*/

#include <string.h>
#include <SoftwareSerial.h>
#include "Nextion.h"

#define rxPin 2
#define txPin 3

#define WAVEFORM_RANGE 200 // 5 in display - 255
#define TIMESCALE 3 // Determines the timescale of the waveform. Higher number -> More zoomed in. Must be at least 1.

SoftwareSerial accelIn = SoftwareSerial(rxPin, txPin);

// Declare waveform object, NexObject::NexObject(uint8_t pageid, uint8_t componentid, const char *objname);
NexWaveform waveform = NexWaveform(0, 2, "s1");

void setup() {
  // Serial.begin(9600); // debugging to serial monitor
  nexInit(); // communicating with nex display
  //pinMode(LED_BUILTIN, OUTPUT);
  accelIn.begin(9600);
  accelIn.setTimeout(20);
}

void loop() {
  // if(Serial.available() > 0)
  // {
  //   String input = Serial.readString();
  //   if(input == "1")
  //   {
  //     digitalWrite(LED_BUILTIN, HIGH);
  //     delay(500);
  //     digitalWrite(LED_BUILTIN, LOW);
  //   }
  
  if(accelIn.available() > 0)
  {
    String inData = accelIn.readString();
    // digitalWrite(LED_BUILTIN, HIGH);
        // Serial.print(inData);
  //   delay(500);
  //   digitalWrite(LED_BUILTIN, LOW);

    float x = inData.substring(0, inData.indexOf(',')).toFloat();

    String yz = inData.substring(inData.indexOf(',') + 1);

    float y = yz.substring(0, yz.indexOf(',')).toFloat();

    float z = yz.substring(yz.indexOf(',') + 1).toFloat();

    // Serial.println("X: " + String(x) + ", Y: " + String(y) + ", Z: " + String(z));
    
    float converted_z = ((WAVEFORM_RANGE / 4) * z) + (WAVEFORM_RANGE / 2); // convert data to waveform data range of 0 to 255.
    //float converted_z = ((255 / 4) * z) + 127.5; // convert data to waveform data range of 0 to 255.

    // Serial.println("z: " + String(z) + ", Converted z: " + String(converted_z)); // print to serial monitor - for testing

    // waveform.addValue(0, converted_z); // send value to waveform

    for(int i = 0; i < 3; i++) {
      waveform.addValue(0, converted_z); // send value to waveform
    }
  }

}
