#define StartLaser 12
#define EndLaser 10

#define OutLaser 2

#define LaserDistance 135000 // distance in mircometers

unsigned long startTime;
unsigned long endTime;

char velocity[20];
char start[20];
char end[20];


void setup() {
  // put your setup code here, to run once:
  pinMode(StartLaser, INPUT);
  pinMode(EndLaser, INPUT);
  pinMode(OutLaser, OUTPUT);
  digitalWrite(OutLaser, HIGH);
  Serial.begin(9600);
  Serial.println("Starting...");
}

void loop() {
  // put your main code here, to run repeatedly:
  if(digitalRead(StartLaser))
  {
    startTime = micros();
    while(!digitalRead(EndLaser));
    endTime = micros();

    unsigned long totalTime = endTime - startTime;
    //Serial.println(totalTime);
    float MpS = (float)LaserDistance / (float)totalTime;  
    Serial.print("Velocity: ");
    Serial.println(MpS);

    //sprintf(velocity, "Velocity: %.2f",MpS);
    sprintf(start, "Start: %lu",startTime);
    sprintf(end, "End: %lu\n",endTime);
    Serial.print(start);
    Serial.print(", ");
    Serial.print(end);
    //Serial.println(MpS,DEC);
    Serial.println(velocity);
    Serial.println("_________");
    while(digitalRead(EndLaser));
  }
  else {
  //Serial.println("no first");
  }
  
}
