#include <SoftwareSerial.h>
SoftwareSerial wemos(4, 3); //RX y TX

int xPosition = 0;
int yPosition = 0;
double latitud = 37.769145;
double longitud = -3.787956;
int numDecimales = 6;
double incremento = 0.00001;
//int buttonState = 0;
int xPin = A0;
int yPin = A1;
//int buttonPin = 2;

void setup(){
  pinMode(xPin, INPUT);
  pinMode(yPin, INPUT);
  Serial.begin(9600);
  wemos.begin(9600);
}
void loop() {
  xPosition = analogRead(xPin);
  yPosition = analogRead(yPin);
  Serial.print("X = ");
  Serial.println(xPosition);
  Serial.print("Y = ");
  Serial.println(yPosition);

  if(yPosition < 400){//hacia el oeste
    longitud -= incremento;
  }
  if(xPosition > 600){//hacia el norte
    latitud += incremento;
  }
  if(xPosition < 400){//hacia el sur
    latitud -= incremento;
  }
  if(yPosition > 600){//hacia el este
    longitud += incremento;
  }

  wemos.print("{\"longitude\": ");
  wemos.print(longitud, numDecimales);
  wemos.print(",\"latitude\": ");
  wemos.print(latitud, numDecimales);
  wemos.println("}#");

  Serial.print("{\"longitude\": ");
  Serial.print(longitud, numDecimales);
  Serial.print(",\"latitude\": ");
  Serial.print(latitud, numDecimales);
  Serial.println("}#");
  delay(100);
}

//{"longitude": -3.787956,"gps_accuracy": 60,"latitude": 37.769145,"battery_level": 99.9}
