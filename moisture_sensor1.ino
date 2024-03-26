/*
obsługa pojemnościowego sensora wilgotności
na arduino
*/

int sensor, sensorNorm;
int pot, potNorm;
int histereza;

#define sensorPin 0 //sensor do A0
#define potPin 3 //potencjometr do A2
#define redPin 10 // czerwony pin
#define grnPin 11 // zielony pin
#define zaworPin 8 // sterowanie zaworem

void setup() {
  histereza = 5;
  Serial.begin(9600); 
  pinMode(sensorPin,INPUT);
  pinMode(potPin,INPUT);
  pinMode(redPin,OUTPUT);
  pinMode(grnPin,OUTPUT);
  pinMode(zaworPin,OUTPUT);
}

void loop() {
  
  sensor = analogRead(sensorPin); 
  sensorNorm=map(sensor,270,600,0,400);
  pot = analogRead(potPin); 
  potNorm=map(pot,50,1000,0,1500);
  Serial.print("Sensor = ");
  Serial.print(sensor);
  Serial.print("   ");
  Serial.println(sensorNorm); //print the value to serial port
  Serial.print("Potencjometr = ");
  Serial.print(pot);
  Serial.print("   ");
  Serial.println(potNorm); //print the value to serial port
  if (sensorNorm>potNorm)  {  // sensor > pot  - sucho - podlewać
    
      digitalWrite(redPin,HIGH);  //czerwony LED
      digitalWrite(grnPin,LOW);
      digitalWrite(zaworPin,HIGH);
      Serial.println("Sucho!");
    }
  if (sensorNorm<(potNorm - histereza)) {  // sensor < pot - mokro - dobrze podlane
    
      digitalWrite(redPin,LOW);  // zielony LED
      digitalWrite(grnPin,HIGH);
      digitalWrite(zaworPin,LOW);
      Serial.println("Mokro!");
    }
  Serial.print("podlewanie przy - ");  
  Serial.println(potNorm - histereza);
  Serial.println("");
  delay(5000);
}
