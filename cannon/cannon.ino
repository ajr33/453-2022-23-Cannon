
#define StartTransmitter  2 // VCC for first laser.
#define EndTransmitter    3 // VCC for second laser. 

#define StartReceiver 9 
#define EndReceiver 10

// Can remove these gpio as power later.
// Angle value when barrel is all the way down approx. 275
// angle value when barrel is all the way high approx. 303
#define angleGND 52
#define angleVCC 22

#define LaserDistance 135000  // Distance in mircometers

volatile unsigned long startTime;
volatile unsigned long endTime;
volatile unsigned long totalTime = 0;

unsigned long falseReadingDelay = 500000;  // In microseconds
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

// Runs once on Arduino.
void setup() {
  // put your setup code here, to run once:
  pinMode(StartTransmitter, OUTPUT);
  pinMode(EndTransmitter, OUTPUT);
  pinMode(StartReceiver, INPUT);
  pinMode(EndReceiver, INPUT);

  digitalWrite(StartTransmitter, HIGH);
  digitalWrite(EndTransmitter, HIGH);

  // Power for angle sensor
  pinMode(angleGND, OUTPUT);
  pinMode(angleVCC, OUTPUT);
  digitalWrite(angleGND, LOW);
  digitalWrite(angleVCC, HIGH);

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
    int cleanPressure = map(dPressure, 80, 400, 0, 100);
    cleanPressure = constrain(cleanPressure, 0, 100);
    // Serial.print("Pressure: ");
    // Serial.println(cleanPressure);

    int angle = map(analogRead(A0), 275, 303, 12, 30);
    angle = constrain(angle, 0, 60);
    //Serial.println(analogRead(A0));
    Serial.print("Angle: ");
    Serial.println(angle);
    delay(10);


    if (Serial3.available() > 0) {
      Serial3.readBytes(heroBuf, 2);
      if (memcmp(heroBuf, arduinoSetupFire, 2) == 0) {
        SetVelocityReadings();
      }
    }
  }



  // Send to Hero that Arduino is ready for fire.
  if (Serial3.availableForWrite()) {
    Serial3.write(arduinoReadyToFire, 2);
  }
  while (velocityReadings) {
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

      PrintBoolReadings(1);
      SetIdleReadings();
      PrintBoolReadings(2);



      if (!resetVelocity && totalTime > 0) {
        Serial.println(totalTime);
        float MpS = (float)LaserDistance / (float)totalTime;
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

        delay(5000);  // wait 5 seconds after launch
      } else {
        resetVelocity = false;
      }
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