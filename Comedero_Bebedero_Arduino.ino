//Incluimos todas las librerias necesarias
#include <Servo.h>
#include <SoftwareSerial.h>
#include <HX711_ADC.h>
#include <OneWire.h>
#include <DallasTemperature.h>


//definimos las constantes
int numMedidas = 5; //numero de medidas que se van a tomar para calcular la media

//Definimos los pines DIGITALES
//PIN 0 
//PIN 1
int trigPienso = 2;
int echoPienso = 3;
//PIN 4
int trigAgua = 5;
int echoAgua = 6;
int pinReleBomba = 7;
int pinServo = 8;
int pinTempAgua = 9;
//PIN 12
//PIN 13

//Definimos los pines ANALÓGICOS
const int dout_comida = A0;
const int sck_comida = A1;
const int dout_agua = A2;
const int sck_agua = A3;

//________________________________________________________________Variables
bool despierta  = false;
long duracion, distancia, distanciaComida, distanciaAgua = 0; //variables para el ultrasonico
double confPesoAgua = 5;
double confPesoComida = 5;//cantidad de comida y agua que debe haber en el cuenco
float tempAgua;
String state;
char caracter;
bool echarComida = false;
bool echarAgua = false;
int pos = 0;//posición inicial del servo
double pesoAgua = 0;
double pesoComida = 0;

//Variables Pesos
const float valorCal_comida = -419.55;
const float valorCal_agua = -467.09;
unsigned long t1 = 0;
unsigned long t2 = 0;
float acumuladorComida = 0;
float acumuladorAgua = 0;
int medidasComida = 0;
int medidasAgua = 0;
String separador = ",";
static boolean newDataReadyComida = 0;
static boolean newDataReadyAgua = 0;
const int serialPrintInterval = 2000; //increase value to slow down serial print activity

//Inicializamos los sensores
Servo servo;
SoftwareSerial BT(10, 11);   // Definimos los pines RX y TX del Arduino conectados al Bluetooth
HX711_ADC celda_comida(dout_comida, sck_comida);
HX711_ADC celda_agua(dout_agua, sck_agua);
OneWire oneWireObjeto(pinTempAgua);
DallasTemperature sensorDS18B20(&oneWireObjeto);

//________________________________________________________________SETUP
void setup() {

  // put your setup code here, to run once:
  Serial.begin(9600);
  BT.begin(9600);

  //SERVO
  servo.attach(pinServo);

  //ULTRASONICO
  pinMode(trigPienso, OUTPUT);
  pinMode(echoPienso, INPUT);
  pinMode(trigAgua, OUTPUT);
  pinMode(echoAgua, INPUT);
  pinMode(pinReleBomba, OUTPUT);

  //CELDAS DE CARGA
  celda_comida.begin();
  celda_agua.begin();

  //TEMP AGUA
  sensorDS18B20.begin();

  unsigned long stabilizingtime = 2000;
  boolean _tare = true;

  celda_comida.start(stabilizingtime, _tare);
  celda_agua.start(stabilizingtime, _tare);

  celda_comida.setCalFactor(valorCal_comida);
  celda_agua.setCalFactor(valorCal_agua);
}


//________________________________________________________________LOOP
void loop() {
  while (BT.available() > 0) { // Comprueba si hay datos en el puerto serie
    caracter = BT.read(); // Lee los datos del puerto serie
    state.concat(caracter);
    if (caracter == '#') {
      if (state.substring(0, state.length() - 1) == "ON") { //llega la señal para despertar el sistema
        despierta = true;
      }else{//llegan los datos establecidos por el usuario
        actualizaEstado(state.substring(0, state.length() - 1));
      }
      Serial.println(state);
      state = "";
    }

  }

  if (despierta) {
    //Comprobaciones de la cantidad de alimento en las tolvas
    //PIENSO
    distanciaComida = 0;
    for (int i = 0; i < numMedidas; i++) {
      digitalWrite(trigPienso, LOW);
      delayMicroseconds(2);
      digitalWrite(trigPienso, HIGH);
      delayMicroseconds(10);
      digitalWrite(trigPienso, LOW);
      duracion = pulseIn(echoPienso, HIGH);
      //Calculate distancia en cm según la velocidad del sonido.
      distancia = duracion / 58.2;
      distanciaComida += distancia;
    }

    distanciaComida = distanciaComida / numMedidas;
    Serial.print("DISTANCIA COMIDA ");
    Serial.println(distanciaComida);

    //AGUA
    distanciaAgua = 0;
    for (int i = 0; i < numMedidas; i++) {
      digitalWrite(trigAgua, LOW);
      delayMicroseconds(2);
      digitalWrite(trigAgua, HIGH);
      delayMicroseconds(10);
      digitalWrite(trigAgua, LOW);
      duracion = pulseIn(echoAgua, HIGH);
      //Calculate distancia en cm según la velocidad del sonido.
      distancia = duracion / 58.2;
      distanciaAgua += distancia;
    }

    distanciaAgua = distanciaAgua / numMedidas;
    Serial.print("DISTANCIA AGUA ");
    Serial.println(distanciaAgua);

    //Temperatura del agua
    sensorDS18B20.requestTemperatures();

    Serial.print("Temperatura sensor 0: ");
    tempAgua = sensorDS18B20.getTempCByIndex(0);
    Serial.print(tempAgua);
    Serial.println(" C");

    //Comprobaciones de los cuencos y si hay que poner más alimento
    //PIENSO
    for (int i = 0; i < 5; i++) {
      pesoComida = calcularPesoComida();
    }
    Serial.print("COMIDA EN CUENCO: ");
    Serial.println(pesoComida);
    if (pesoComida < confPesoComida) {
      echarComida = true;
    }

    //AGUA
    for (int i = 0; i < 5; i++) {
      pesoAgua = calcularPesoAgua();
    }
    Serial.print("AGUA EN CUENCO: ");
    Serial.println(pesoAgua);
    if (pesoAgua < confPesoAgua) {
      echarAgua = true;
    }

    //ECHAR EN LOS COMEDEROS
    //COMIDA
    if (echarComida) {
      Serial.println("echando comida...");
      while (calcularPesoComida() < confPesoComida) {
        for (pos = 0; pos <= 180; pos += 1) {
          //Serial.println(pos);
          servo.write(pos); //mueve el servo a la pos
          delay(1);
        }
        servo.write(0);
      }
      echarComida = false;
    }

    //AGUA
    if (echarAgua) {
      Serial.println("echando agua...");
      while (calcularPesoAgua() < confPesoAgua) {
        digitalWrite(pinReleBomba, HIGH);
        delay(5000);
        digitalWrite(pinReleBomba, LOW);
      }
      //digitalWrite(pinReleBomba, LOW);
      echarAgua = false;
    }

    //Enviamos los datos por bluetooth
    Serial.println(String(tempAgua) + "," + String(pesoAgua)  + "," + String(pesoComida) + "," + String(distanciaAgua) + "," + String(distanciaComida));
    BT.println(String(tempAgua) + "," + String(pesoAgua)  + "," + String(pesoComida) + "," + String(distanciaAgua) + "," + String(distanciaComida) + '#');
    despierta = false;
  }
}


//________________________________________________________________Funciones auxiliares
float calcularPesoComida() {
  unsigned long  t = millis();
  acumuladorComida = 0;
  medidasComida = 0;
  delay(1000);
  while (true) {
    if (celda_comida.update()) newDataReadyComida = true;
    if (newDataReadyComida) {
      acumuladorComida += celda_comida.getData();
      medidasComida++;
      if (millis() > t + serialPrintInterval) {
        break;
      }
    }
  }
  pesoComida = acumuladorComida / medidasComida;
  return pesoComida;
}

float calcularPesoAgua() {
  unsigned long  t = millis();
  acumuladorAgua = 0;
  medidasAgua = 0;
  delay(1000);
  while (true) {
    if (celda_agua.update()) newDataReadyAgua = true;
    if (newDataReadyAgua) {
      acumuladorAgua += celda_agua.getData();
      medidasAgua++;
      if (millis() > t + serialPrintInterval) {
        break;
      }
    }
  }
  pesoAgua = acumuladorAgua / medidasAgua;
  return pesoAgua;
}

void actualizaEstado(String datos){
  int inicio = 0;
  int fin = state.indexOf(separador, inicio);
  double aux = confPesoAgua;
  confPesoAgua = state.substring(inicio, fin).toDouble();
  Serial.println("Cambiada la configuración de AGUA: " + String(aux) + " --> " + String(confPesoAgua));
  inicio = fin + 1;
  fin = state.indexOf(separador, inicio);
  aux = confPesoComida;
  confPesoComida = state.substring(inicio, state.length()).toDouble();
  Serial.println("Cambiada la configuración de COMIDA: " + String(aux) + " --> " + String(confPesoComida) + "\n");
}
