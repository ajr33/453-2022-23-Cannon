void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  Serial.println("Upload successful");
  
}

void loop() {
  // put your main code here, to run repeatedly:
  //int angle = analogRead(A0);
  int angle = map(analogRead(A0), 270, 750, -90, 90);
  Serial.println(angle);
  delay(1);
}
