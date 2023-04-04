/* 
Arduino code for Cannon team.
Course:       CSE 450/453.
Instructor:   Kris Schindler.
Team:         
  Anthony Roberts
  Jeffery Naranjo
  Jennifer Tsang

Details:
  This code is for the Arduino to receive input data and display
  on a Nextion display. Values of the current pressure & angle are constantly 
  shown & with the previous values shown after the projectile was fired. 
  The velocity in meters/second is shown for the last fire as well.

Important notes:
  Make sure that the Nextion Library is part of include path.
  You can download from here: https://github.com/itead/ITEADLIB_Arduino_Nextion

Any questions about this code feel free to reach out to Anthony.
  Email:  ajr33@buffalo.edu
*/

#include <Nextion.h>

// Nextion Display Initialization
NexNumber nex_current_pressure = NexNumber(0,1,"pressure");
NexNumber nex_current_angle = NexNumber(0,2,"angle");

// Previous values when fired.
NexNumber nex_prev_velocity = NexNumber(0,3,"velocity");
NexNumber nex_prev_pressure = NexNumber(0,4,"old_pressure");
NexNumber nex_prev_angle = NexNumber(0,5,"old_angle");



#define StartTransmitter  2 // VCC for first laser.
#define EndTransmitter    3 // VCC for second laser. 

// Laser Receiver inputs.
#define StartReceiver 10 
#define EndReceiver 11

// Can remove these gpio as power later.
// Angle value when barrel is all the way down approx. 275
// angle value when barrel is all the way high approx. 303
//#define angleGND 52
//#define angleVCC 22

#define LaserDistance 250000  // Distance in mircometers (25 cm)

volatile unsigned long startTime;
volatile unsigned long endTime;
volatile unsigned long totalTime = 0;

unsigned long falseReadingDelay = 500000;  // In microseconds = .5 seconds
unsigned long falseReadingCheck;
volatile bool resetVelocity;

bool idleReadings = true;
bool velocityReadings = false;

char velocity[20];
char start[20];
char end[20];

byte heroBuf[2];

const byte arduinoSetupFire[] = { 'F', 0x2 };
const byte arduinoReadyToFire[] = { 'F', 0x1 };

// Pressure
int pressureIn = A1;
int cleanPressure;


// Angle
int angle;

// Runs once on Arduino.
void setup() {
  // For second receiver ground.
  // pinMode(12, OUTPUT);
  // digitalWrite(12, LOW);
  // put your setup code here, to run once:
  // Nextion display initialization.
  nexInit();

  pinMode(StartTransmitter, OUTPUT);
  pinMode(EndTransmitter, OUTPUT);
  pinMode(StartReceiver, INPUT);
  pinMode(EndReceiver, INPUT);

  digitalWrite(StartTransmitter, HIGH);
  digitalWrite(EndTransmitter, HIGH);

  // Power for angle sensor
  // pinMode(angleGND, OUTPUT);
  // pinMode(angleVCC, OUTPUT);
  // digitalWrite(angleGND, LOW);
  // digitalWrite(angleVCC, HIGH);

  //attachInterrupt(digitalPinToInterrupt(StartReceiver), startVelocity, RISING);
  Serial3.setTimeout(100);

  Serial.begin(9600);
  Serial3.begin(9600);
  Serial.println("Starting...");
}

// Continously runs on Arduino.
void loop() {
  while (idleReadings) {
    //PrintBoolReadings(3);
    int dPressure = analogRead(pressureIn);
    cleanPressure = map(dPressure, 80, 400, 0, 100);
    cleanPressure = constrain(cleanPressure, 0, 100);
    // Serial.print("Pressure: ");
    // Serial.println(cleanPressure);
    nex_current_pressure.setValue(cleanPressure);
    // Min angle = 42 degrees
    // Max angle = 58 - 59 degrees0
    // sensor at 0 = 255
    // sensor at 180 = 740
    angle = map(analogRead(A0), 230, 783, 0, 180);
    angle = constrain(angle, 0, 180);
    Serial.print("Angle: ");
    Serial.println(analogRead(A0));
    nex_current_angle.setValue(angle);
    delay(50);

    // Check data from HERO.
    if (Serial3.available() > 0) {
      Serial3.readBytes(heroBuf, 2);
      if (memcmp(heroBuf, arduinoSetupFire, 2) == 0) {
        SetVelocityReadings();
      }
    }
  }


  unsigned long velocityStart = micros(); // Timeout to wait for reading from first laser.
  falseReadingCheck = velocityStart;
  // Send to Hero that Arduino is ready for fire.
  if (Serial3.availableForWrite()) {
    Serial3.write(arduinoReadyToFire, 2);
  }
  while (velocityReadings){
    if (digitalRead(StartReceiver)) {

      startTime = micros();
      falseReadingCheck = startTime;
      while (!digitalRead(EndReceiver)) {

        // Serial.print("false: ");
        // Serial.println(falseReadingCheck);
        // Serial.print("start: ");
        // Serial.println(startTime);
        // Serial.print("delay: ");
        // Serial.println(falseReadingDelay);

        if ((falseReadingCheck - startTime) > falseReadingDelay) {
          resetVelocity = true;
          Serial.println("Reset");
          break;
        }
        falseReadingCheck = micros();
      }
      endTime = micros();
      totalTime = endTime - startTime;

      // PrintBoolReadings(1);
      SetIdleReadings();
      // PrintBoolReadings(2);



      if (!resetVelocity && totalTime > 0) {
        Serial.println(totalTime);
        float MpS = (float)LaserDistance / (float)totalTime;
        MpS += cleanPressure * .1; // account for error based on pressure. This means that the higher the pressure, the greater the line will change to match the actual velocity. 
        Serial.print("Velocity: ");
        Serial.println(MpS);

        totalTime = 0;
        //sprintf(velocity, "Velocity: %.2f",MpS);
        sprintf(start, "Start: %lu", startTime);
        sprintf(end, "End: %lu\n", endTime);
        Serial.print(start);
        Serial.print(", ");
        Serial.print(end);
        //Serial.println(MpS,DEC);
        Serial.println(velocity);
        Serial.println("_________");
        //while (digitalRead(EndReceiver));
        nex_prev_velocity.setValue(round(MpS));
        nex_prev_pressure.setValue(cleanPressure);
        nex_prev_angle.setValue(angle);

        delay(5000);  // wait 5 seconds after launch
      } else {
        resetVelocity = false;
      }
    }
    else if ((velocityStart - falseReadingCheck) < falseReadingDelay)
    {
      velocityStart = micros();
      // Serial.println("Velocity trigger.");
      // Serial.print(" VeloStart: ");
      // Serial.print(velocityStart);
      // Serial.print(" FalseDelay: ");
      // Serial.print(falseReadingCheck);

    }
    else{
      Serial.println("Velocity timed out.");
      SetIdleReadings();
    }
  }
}


void SetVelocityReadings() {
  velocityReadings = true;
  idleReadings = false;
}

void SetIdleReadings() {
  idleReadings = true;
  velocityReadings = false;
}

void PrintBoolReadings(int id) {
  Serial.print("ID: ");
  Serial.print(id);
  Serial.print(" Idle: ");
  Serial.print(idleReadings);
  Serial.print(" Velo: ");
  Serial.println(velocityReadings);
}