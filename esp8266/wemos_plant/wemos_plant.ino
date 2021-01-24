const int relayPin = D1;
const int analogInPin = A0;
int sensorValue = 0;

void setup() {
  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, LOW); // turn off
  Serial.begin(115200);
}

void loop() {
  // read the analog in value
  sensorValue = analogRead(analogInPin);

  // print the readings in the Serial Monitor
  Serial.print("sensor = ");
  Serial.println(sensorValue);
  
  // sensorValue - goes down with more humidity
  if (sensorValue > 500) {
    digitalWrite(relayPin, HIGH); // turn on
    delay(5000);
    digitalWrite(relayPin, LOW); // turn off
  }
  
  delay(2000);
   
}
