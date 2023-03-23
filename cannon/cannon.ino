// Make sure that the Nextion Library is part of include path.
// You can download from here: https://github.com/itead/ITEADLIB_Arduino_Nextion

#include <Nextion.h>

// Nextion Display Initialization
NexNumber nex_angle = NexNumber(0,5,"angle");
NexNumber nex_pressure = NexNumber(0,4,"pressure");
NexNumber nex_velocity = NexNumber(0,6,"velocity");


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

#define LaserDistance 245000  // Distance in mircometers

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
    nex_pressure.setValue(cleanPressure);

    int angle = map(analogRead(A0), 275, 303, 12, 30);
    angle = constrain(angle, 0, 60);
    // Serial.print("Angle: ");
    // Serial.println(angle);
    nex_angle.setValue(angle);
    delay(50);


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
        nex_velocity.setValue(round(MpS));

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