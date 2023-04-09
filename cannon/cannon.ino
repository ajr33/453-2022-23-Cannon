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



#define StartTransmitter  5 // VCC for first laser.
#define EndTransmitter    6 // VCC for second laser. 

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

// Deadman Switch
#define Dead 12

// LEDs
// IO Pins D0, D1, & D2 will represent the status of the cannon.
// Note: The RGB LED is common anode, so each LED is active low.
#define IdleLED 2       // Red.
#define VelocityLED 3   // Green.
#define DeadLED 4       // Blue.

// Previous values.
int prevPressure;
int prevAngle;

// Function declarations.
void RGB_Check(int amount = 3, unsigned long timePerBlink = 250);
void BlinkLED(int LED, unsigned long timePerBlink = 150);


// Runs once on Arduino.
void setup() {
  // For second receiver ground.
  // pinMode(12, OUTPUT);
  // digitalWrite(12, LOW);
  // put your setup code here, to run once:
  // Nextion display initialization.
  nexInit();

  // Lasers
  pinMode(StartTransmitter, OUTPUT);
  pinMode(EndTransmitter, OUTPUT);
  pinMode(StartReceiver, INPUT);
  pinMode(EndReceiver, INPUT);

  digitalWrite(StartTransmitter, HIGH);
  digitalWrite(EndTransmitter, HIGH);

  // Deadman Swtich Input
  pinMode(Dead, INPUT);

  // LEDs
  pinMode(IdleLED, OUTPUT);
  pinMode(VelocityLED, OUTPUT);
  pinMode(DeadLED, OUTPUT);
  digitalWrite(IdleLED, HIGH);
  digitalWrite(VelocityLED, HIGH);
  digitalWrite(DeadLED, !digitalRead(Dead));


  // UART/Serial
  Serial3.setTimeout(100);
  Serial.begin(9600);
  Serial3.begin(9600);
  Serial.println("Starting...");

  // Illuminate each LED to make sure that they're working properly.  
  RGB_Check();
  // // Red.
  // digitalWrite(VelocityLED, HIGH);
  // digitalWrite(DeadLED, HIGH);
  // digitalWrite(IdleLED, LOW);
  // delay(1000);

  // // Green.
  // digitalWrite(IdleLED, HIGH);
  // digitalWrite(DeadLED, HIGH);
  // digitalWrite(VelocityLED, LOW);
  // delay(1000);
  
  // // Blue.
  // digitalWrite(VelocityLED, HIGH);
  // digitalWrite(IdleLED, HIGH);
  // digitalWrite(DeadLED, LOW);
  // delay(1000);

  // // White.
  // digitalWrite(VelocityLED, LOW);
  // digitalWrite(IdleLED, LOW);
  // digitalWrite(DeadLED, LOW);
  // delay(1000);

  // Start as idle, reading in the pressure & angle measurements
  // then displaying them on the nextion display.

  Serial.println("Ready");

  SetIdleReadings();
}

// Continously runs on Arduino.
void loop() {
  while (idleReadings) {
    // Update Deadman switch status.
    digitalWrite(DeadLED, !digitalRead(Dead));

    //PrintBoolReadings(3);
    int dPressure = analogRead(pressureIn);
    cleanPressure = map(dPressure, 80, 400, 0, 100);
    cleanPressure = constrain(cleanPressure, 0, 100);
    // Serial.print("Pressure: ");
    // Serial.println(cleanPressure);
    nex_current_pressure.setValue(cleanPressure);
    // TODO: Have the Hero board send a signal to arduino when user is pressurizing & changing angle.
    if(prevPressure < cleanPressure)
    {
      BlinkLED(VelocityLED);
    }
    prevPressure = cleanPressure;
    // Min angle = 42 degrees
    // Max angle = 58 - 59 degrees0
    // sensor at 0 = 255
    // sensor at 180 = 740
    angle = map(analogRead(A0), 230, 783, 0, 180);
    angle = constrain(angle, 0, 180);
    // Serial.print("Angle: ");
    // Serial.println(analogRead(A0));
    nex_current_angle.setValue(angle);
    prevAngle = angle;
    delay(10);

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
    if (digitalRead(StartReceiver)) { // Read from the fisrt laser.
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
      // SetIdleReadings();
      // PrintBoolReadings(2);



      if (!resetVelocity && totalTime > 0) {
        //Serial.println(totalTime);

        // Set LEDs to idicate that a projectile has went through both lasers.
        digitalWrite(IdleLED, HIGH);
        digitalWrite(DeadLED, HIGH);
        digitalWrite(VelocityLED, LOW); 

        float MpS = (float)LaserDistance / (float)totalTime;
        MpS += cleanPressure * .1; // account for error based on pressure. This means that the higher the pressure, the greater the line will change to match the actual velocity. 
        // Serial.print("Velocity: ");
        // Serial.println(MpS);

        totalTime = 0;
        //sprintf(velocity, "Velocity: %.2f",MpS);
        // sprintf(start, "Start: %lu", startTime);
        // sprintf(end, "End: %lu\n", endTime);
        // Serial.print(start);
        // Serial.print(", ");
        // Serial.print(end);
        // Serial.println(velocity);
        // Serial.println("_________");
        nex_prev_velocity.setValue(round(MpS));
        nex_prev_pressure.setValue(cleanPressure);
        nex_prev_angle.setValue(angle);

        
        delay(5000);  // wait 5 seconds after launch
        SetIdleReadings();


      } else {
        resetVelocity = false;
      }
    }
    // Keep track of the time when reading first laser.
    // If still within limit...
    else if ((velocityStart - falseReadingCheck) < falseReadingDelay) 
    {
      velocityStart = micros();
      // Serial.println("Velocity trigger.");
      // Serial.print(" VeloStart: ");
      // Serial.print(velocityStart);
      // Serial.print(" FalseDelay: ");
      // Serial.print(falseReadingCheck);

    }
    else{ // First laser timed out.
      BlinkIdleLED(10);
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
  // Set LEDs to match status.
  digitalWrite(IdleLED, LOW);
  digitalWrite(VelocityLED, HIGH);
}

void PrintBoolReadings(int id) {
  Serial.print("ID: ");
  Serial.print(id);
  Serial.print(" Idle: ");
  Serial.print(idleReadings);
  Serial.print(" Velo: ");
  Serial.println(velocityReadings);
}

// Blinks the RGB LED to make sure each one works as intended.
void RGB_Check(int amount = 3, unsigned long timePerBlink = 250)
{
  // Turn off all LEDs.
  digitalWrite(IdleLED, HIGH);
  digitalWrite(VelocityLED, HIGH);
  digitalWrite(DeadLED, HIGH);

  for (int i = 0; i < amount * 2; i++)
  {
    digitalWrite(IdleLED, !digitalRead(IdleLED));
    delay(timePerBlink);
  }

  for (int i = 0; i < amount * 2; i++)
  {
    digitalWrite(VelocityLED, !digitalRead(VelocityLED));
    delay(timePerBlink);
  }
  
  for (int i = 0; i < amount * 2; i++)
  {
    digitalWrite(DeadLED, !digitalRead(DeadLED));
    delay(timePerBlink);
  }

  // Show white at the end.
  digitalWrite(VelocityLED, LOW);
  digitalWrite(IdleLED, LOW);
  digitalWrite(DeadLED, LOW);
  delay(timePerBlink*6);
}

// Blinks the Idle LED the specified amount of times. 
void BlinkIdleLED(int amount)
{
  digitalWrite(IdleLED, HIGH);
  digitalWrite(VelocityLED, HIGH);
  digitalWrite(DeadLED, HIGH);


  for (int i=0; i < amount*2; i++)
  {
    delay(200);
    digitalWrite(IdleLED, !digitalRead(IdleLED));
  }
}

void BlinkLED(int LED, unsigned long timePerBlink = 150)
{
  if(LED == VelocityLED)
  {
    digitalWrite(LED & VelocityLED, LOW);
    delay(timePerBlink);
    digitalWrite(LED & VelocityLED, HIGH);
    delay(timePerBlink);
  }
  else if (LED == IdleLED)
  {
    digitalWrite(LED & IdleLED, LOW);
    delay(timePerBlink);
    digitalWrite(LED & IdleLED, HIGH);
    delay(timePerBlink);
    digitalWrite(LED & IdleLED, LOW);
  }
  else if (LED == DeadLED)
  {
    digitalWrite(LED & DeadLED, LOW);
    delay(timePerBlink);
    digitalWrite(LED & DeadLED, HIGH);
    delay(timePerBlink);
  }
}
