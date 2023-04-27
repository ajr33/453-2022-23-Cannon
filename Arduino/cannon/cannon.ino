/* 
Arduino code for Cannon team 2022-2023.
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


#define LaserDistance 310000  // Distance in mircometers (31 cm)
#define ShouldAccountForError true

// Nextion Display Initialization
NexNumber nex_current_pressure = NexNumber(0,1,"pressure");
NexNumber nex_current_angle = NexNumber(0,2,"angle");

// Previous values when fired.
// NexNumber vel_behind = NexNumber(0,3,"velocity");
NexText nex_prev_velocity = NexText(0,5,"float_vel");
NexNumber nex_prev_pressure = NexNumber(0,3,"old_pressure");
NexNumber nex_prev_angle = NexNumber(0,4,"old_angle");



#define StartTransmitter  5 // VCC for first laser.
#define EndTransmitter    6 // VCC for second laser. 

// Laser Receiver inputs.
#define StartReceiver 2 
#define EndReceiver 3

// Can remove these gpio as power later.
// Angle value when barrel is all the way down approx. 275
// angle value when barrel is all the way high approx. 303
//#define angleGND 52
//#define angleVCC 22



volatile unsigned long startTime;
volatile unsigned long endTime;
volatile unsigned long totalTime = 0;

unsigned long falseReadingDelay = 1000000;  // In microseconds = 1 second 
unsigned long falseReadingCheck;
volatile bool resetVelocity;

bool idleReadings = true;
bool velocityReadings = false;

char velocity[20];
char start[20];
char end[20];

char velocity_str[8]; // buffer for velocity string

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
#define IdleLED     9   // Red.
#define VelocityLED 10   // Green.
#define DeadLED     11   // Blue.

#define PressureLED 22
#define AngleLED    23
#define FireLED     24
// Previous values.
int prevPressure;
int prevAngle;

// Function declarations.
void RGB_Check(int amount = 3, unsigned long timePerBlink = 250);
void BlinkLED(int LED, unsigned long timePerBlink = 150);



// Interrupts.
// States.
volatile bool sTrigger, eTrigger = false;


// ISRs.
void StartLaserIsr()
{
  // Make sure state was not already set.
  if (!sTrigger && velocityReadings)
  {
    startTime = micros();
    sTrigger = true;
  }
}


void EndLaserIsr()
{
  // Make sure state was not already set.
  if ( !eTrigger && velocityReadings)
  {
    endTime = micros();
    eTrigger = true;
  }
  
}

// Nextion display is connected to Serial2.
//#define NexSerial Serial2

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
  pinMode(PressureLED, INPUT_PULLUP);
  pinMode(AngleLED, INPUT_PULLUP);
  pinMode(FireLED, INPUT_PULLUP);

  digitalWrite(IdleLED, HIGH);
  digitalWrite(VelocityLED, HIGH);
  digitalWrite(DeadLED, HIGH);
  


  // Interrupts for the laser receivers.
  // attachInterrupt(digitalPinToInterrupt(StartReceiver), StartLaserIsr, RISING);
  // attachInterrupt(digitalPinToInterrupt(EndReceiver), EndLaserIsr, RISING);

  // UART/Serial
  Serial3.setTimeout(100);
  Serial.begin(9600);
  Serial3.begin(9600);
  Serial.println("Starting...");

  // Serial.println("Test nextion");
  
  

  // Serial2.begin(9600);

  // Serial2.print(F("velocity.val=30"));
  // // Serial2.print(F("h"));
  // // Serial2.print(F("\""));
  // Serial2.print(F("\xFF\xFF\xFF"));

  //nex_prev_velocity.setText("hello");
  // delay(1000);
  // nex_prev_velocity.setText("h");
  // nex_prev_angle.setValue(32);
  // delay(1000);
  // nex_prev_velocity.setText("he");
  // nex_prev_angle.setValue(33);
  // delay(1000);
  // nex_prev_angle.setValue(34);
  // nex_prev_velocity.setText("hel");
  // delay(1000);
  // nex_prev_velocity.setText("hell");
  // nex_prev_angle.setValue(35);

  // Illuminate each LED to make sure that they're working properly.  
  // RGB_Check();
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
  
  while (velocityReadings)
  {
    // Serial.println("velo read");
    // while (!sTrigger) // First laser is not triggered. 
    // {
    // Serial.println("start read");

    //   falseReadingCheck = micros();
    //   if ((falseReadingCheck - velocityStart) > falseReadingDelay) 
    //   {
    //     resetVelocity = true;
    //     Serial.println("Reset");
    //     break;
    //   }
    // }

    // if (!resetVelocity)
    // {
      startTime = micros();
      // while (!eTrigger) // Wait for the end laser to be triggered.
      while (!digitalRead(EndReceiver))
      {
        // // Serial.println("end read");

        falseReadingCheck = micros();
        if ((falseReadingCheck - startTime) > falseReadingDelay) 
        {
          resetVelocity = true;
          Serial.println("Reset");
          break;
        }
      }
      endTime = micros();

    // }
      
    if (!resetVelocity)
    {
      
      // Calculate the velocity.
      totalTime = endTime - startTime;
      float MpS = (float)LaserDistance / (float)totalTime;

      if (ShouldAccountForError)
      {
        MpS += cleanPressure * .43; // account for error based on pressure. This means that the higher the pressure, the greater the line will change to match the actual velocity. 

      } 
      // Serial.print("Velocity: ");
      // Serial.println(MpS);

      totalTime = 0;
      dtostrf(MpS, 0, 1, velocity_str);   // convert float to string for displaying on screen

      sprintf(velocity, "Velocity: %.2f",MpS);
      sprintf(start, "Start: %lu", startTime);
      sprintf(end, "End: %lu\n", endTime);
      Serial.print(start);
      Serial.print(", ");
      Serial.print(end);
      Serial.println(velocity_str);
      Serial.println("_________");
      //nex_prev_velocity.setValue(round(MpS));   
      
      nex_prev_velocity.setText(velocity_str);
      nex_prev_pressure.setValue(cleanPressure);
      nex_prev_angle.setValue(angle);

      
      delay(5000);  // wait 5 seconds after launch
      SetIdleReadings();
      //continue;
    }
    else
    {
      // At this point the projectile has potentially timed out.

      // If there is any pressure in the system still, the cannon didn't fire.
      if (map(analogRead(pressureIn), 80, 400, 0, 100) > 2){ continue; }
      Serial.println("Velocity Failed");
      Serial.print("Start Trigger: ");
      Serial.println(sTrigger);

      Serial.print("End Trigger: ");
      Serial.println(eTrigger);

      Serial.print("Start time: ");
      Serial.println(startTime);

      Serial.print("End time: ");
      Serial.println(endTime);

      BlinkIdleLED(10);
      Serial.println("Velocity timed out.");
      SetIdleReadings();
    }

    
  //   if (digitalRead(StartReceiver)) { // Read from the fisrt laser.
  //     startTime = micros();
  //     falseReadingCheck = startTime;
  //     while (!digitalRead(EndReceiver)) {

  //       // Serial.print("false: ");
  //       // Serial.println(falseReadingCheck);
  //       // Serial.print("start: ");
  //       // Serial.println(startTime);
  //       // Serial.print("delay: ");
  //       // Serial.println(falseReadingDelay);

  //       if ((falseReadingCheck - startTime) > falseReadingDelay) {
  //         resetVelocity = true;
  //         Serial.println("Reset");
  //         break;
  //       }
  //       falseReadingCheck = micros();
  //     }
  //     endTime = micros();
  //     totalTime = endTime - startTime;

  //     // PrintBoolReadings(1);
  //     // SetIdleReadings();
  //     // PrintBoolReadings(2);



  //     if (!resetVelocity && totalTime > 0) {
  //       //Serial.println(totalTime);

  //       // Set LEDs to idicate that a projectile has went through both lasers.
  //       digitalWrite(IdleLED, HIGH);
  //       digitalWrite(DeadLED, HIGH);
  //       digitalWrite(VelocityLED, LOW); 

  //       float MpS = (float)LaserDistance / (float)totalTime;

  //       if (ShouldAccountForError)
  //       {
  //         MpS += cleanPressure * .1; // account for error based on pressure. This means that the higher the pressure, the greater the line will change to match the actual velocity. 
  //       } 
  //       // Serial.print("Velocity: ");
  //       // Serial.println(MpS);

  //       totalTime = 0;
  //       //sprintf(velocity, "Velocity: %.2f",MpS);
  //       // sprintf(start, "Start: %lu", startTime);
  //       // sprintf(end, "End: %lu\n", endTime);
  //       // Serial.print(start);
  //       // Serial.print(", ");
  //       // Serial.print(end);
  //       // Serial.println(velocity);
  //       // Serial.println("_________");
  //       //nex_prev_velocity.setValue(round(MpS));   
  //       dtostrf(MpS, 0, 1, velocity_str);   // convert float to string for displaying on screen
  //       nex_prev_velocity.setText(velocity_str);
  //       nex_prev_pressure.setValue(cleanPressure);
  //       nex_prev_angle.setValue(angle);

        
  //       delay(5000);  // wait 5 seconds after launch
  //       SetIdleReadings();


  //     } else {
  //       resetVelocity = false;
  //     }
  //   }
  //   // Keep track of the time when reading first laser.
  //   // If still within limit...
  //   else if ((falseReadingCheck - startTime) < falseReadingDelay) 
  //   {
  //     falseReadingCheck = micros();
  //     // Serial.println("Velocity trigger.");
  //     // Serial.print(" VeloStart: ");
  //     // Serial.print(velocityStart);
  //     // Serial.print(" FalseDelay: ");
  //     // Serial.print(falseReadingCheck);

  //   }
  //   else
  //   { // First laser timed out.
  //     // If there is any pressure in the system still, the cannon didn't fire.
  //     if (map(analogRead(pressureIn), 80, 400, 0, 100) > 2){ continue; }

  //     BlinkIdleLED(10);
  //     Serial.println("Velocity timed out.");
  //     SetIdleReadings();
  //   }
  }
}


void SetVelocityReadings() {
  velocityReadings = true;
  idleReadings = false;
}

void SetIdleReadings() {
  idleReadings = true;
  velocityReadings = false;

  resetVelocity = false;
  sTrigger = eTrigger = false;
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
  // Turn each LED off first. 
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


void SetVelocityLED()
{
  // Set LEDs to idicate that a projectile has went through both lasers.
  digitalWrite(IdleLED, HIGH);
  digitalWrite(DeadLED, HIGH);
  digitalWrite(VelocityLED, LOW); 
}