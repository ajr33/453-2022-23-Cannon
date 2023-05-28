#include <Wire.h>
#include <SoftwareSerial.h>

#define HC12Mode 1  //  For digital input 1 for normal transmission, 0 for command mode. 
                      //For analog, higher than 150 for normal and lower 50 for low

#define HC12StatePin 5 // The pin that is connected to the state pin for the HC-12.


#define HC12_RX 10
#define HC12_TX 11

SoftwareSerial HC12Serial(HC12_RX, HC12_TX);  //HC12 rx pin is connected to Nano's D11
                                              //HC12 tx pin is connected to Nano's D10

int ADXL345 = 0x53;
String x,y,z;

void setup() {
  pinMode(HC12StatePin, OUTPUT);
  digitalWrite(HC12StatePin, HC12Mode);

  HC12Serial.begin(9600);
  Serial.begin(9600);
  Wire.begin();
  Wire.beginTransmission(ADXL345);
  Wire.write(0x2D);

  Wire.write(8);
  Wire.endTransmission();
  delay(10);
}

void loop() {
  Wire.beginTransmission(ADXL345);

  Wire.write(0x32);
  Wire.endTransmission(false);
  Wire.requestFrom(ADXL345, 6, true);

  x = getWireRead();

  y = getWireRead();

  z = getWireRead();
  //Serial.println("X: " + x +"g   Y: " + y + "g   Z: " + z + "g");
  HC12Serial.println(x + "," + y + "," + z);
  Serial.println(z);
  //Serial.println(x + "," + y + "," + z);
  delay(50);

}


float getWireRead()
{
  float value = (Wire.read() | Wire.read() << 8);
  return value/256;
}