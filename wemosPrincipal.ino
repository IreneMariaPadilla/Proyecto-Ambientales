#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <SoftwareSerial.h>
SoftwareSerial BT(D4, D3);

const char* ssid = "vodafoneC770";
const char* password = "3B3E2J5N5T79VL";
//const char* ssid = "Casa-Mari (~^•^)~";
//const char* password = "luckyximonicojulieta99";
const char* mqtt_server = "192.168.0.57";

WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE  (50)
char msg[MSG_BUFFER_SIZE];

const int PIRPin = D5;
//const int ArduinoPIN = DX;
String state;
char caracter;
String despierta = "ON";

unsigned long tiempoActual = 0;
boolean primera = true;
int cooldown = 90000;//600000; //10 minutos en milisegundos

//Tópicos de configuración. Se suscribe el wemos y recibe de HA
String ConfigPesoComida = "configPesoComida";
String ConfigPesoAgua = "configPesoAgua";
double configPesoComida = 45;
double configPesoAgua = 20;

//Tópicos de actualización de estado de Home Assistant
char TopicTempAgua[] = "casa/tempAgua";
char TopicPesoAgua[] = "casa/pesoAgua";
char TopicPesoComida[] = "casa/pesoComida";
char TopicDistanciaAgua[] = "casa/distanciaAgua";
char TopicDistanciaComida[] = "casa/distanciaComida";

//Medidas de los distintos sensores
double tempAgua = 20.1;
double pesoAgua = 20;
double pesoComida = 45;
int distanciaAgua = 50;
int distanciaComida = 70;

#define separador ','

void setup_wifi() {
  delay(10);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Conectando a:\t");
  Serial.println(ssid);

  // Esperar a que nos conectemos
  while (WiFi.status() != WL_CONNECTED) {
    delay(200);
    Serial.print('.');
  }

  // Mostrar mensaje de exito y dirección IP asignada
  Serial.println();
  Serial.print("Conectado a:\t");
  Serial.println(WiFi.SSID());
  Serial.print("IP address:\t");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Ha llegado un mensaje [");
  Serial.print(topic);
  Serial.print("] ");
  char mensajeChars[30];
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
    mensajeChars[i] = (char)payload[i];
  }
  Serial.println();

  String mensaje = String(mensajeChars);
  mensaje = mensaje.substring(2, mensaje.length() - 2);
  String topico = String(topic);

  //SE MANDAN LOS PARÁMETROS DE CONFIGURACIÓN POR BLUETOOTH
  if (topico.equals(ConfigPesoAgua)) {
    Serial.println("Se ha actualizado la configuración del pesoAgua: ----- " +
                   String(configPesoAgua) + " --> " + mensaje);
    configPesoAgua = mensaje.toDouble();
  } else if (topico.equals(ConfigPesoComida)) {
    Serial.println("Se ha actualizado la configuración del pesoComida: ----- " +
                   String(configPesoComida) + " --> " + mensaje);
    configPesoComida = mensaje.toDouble();
  } else {
    //No se hace nada
  }

  BT.print(String(configPesoAgua) + separador + String(configPesoComida) + '#');
  Serial.println("Se ha mandado: ----- " + String(configPesoAgua) + separador + String(configPesoComida) + '#');


}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "WemosPrincipal";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str(), "mqtt", "mqtt")) {
      Serial.println("connected");
      char topic1[50]; ConfigPesoAgua.toCharArray(topic1, 50);
      char topic2[50]; ConfigPesoComida.toCharArray(topic2, 50);
      client.subscribe(topic1); //TOPICOS A LOS QUE SUSCRIBIRSE
      client.subscribe(topic2);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(9600);
  BT.begin(9600);

  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  pinMode(PIRPin, INPUT);
  //  pinMode(ArduinoPIN, OUTPUT); PIN PARA DESPERTAR ADUINO
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  int valor_PIR = digitalRead(PIRPin);
  if (valor_PIR == HIGH and (millis() > (tiempoActual + cooldown) or primera)) {
    primera = false;

    tiempoActual = millis();
    despertarArduino();
  }

  while (BT.available() > 0) {
    caracter = BT.read();
    state.concat(caracter);
    if (caracter == '#') {
      actualizaEstado(state.substring(0, state.length() - 1)); //Quitamos el carácter que indica el final del mensaje
      enviaEstadoActual();
      state = "";
      Serial.println();
    }
  }
}

void despertarArduino() {
  Serial.println("ARDUINO DESPERTADO");
  BT.print(despierta + '#');
}

void actualizaEstado(String state) {
  int inicio, fin;

  Serial.println("Actualización del estado actual: ");

  inicio = 0;
  fin = state.indexOf(separador, inicio);
  double auxAgua = tempAgua;
  tempAgua = state.substring(inicio, fin).toDouble();
  Serial.println("Temperatura Agua: " + String(auxAgua) + " --> " + String(tempAgua));


  inicio = fin + 1;
  fin = state.indexOf(separador, inicio);
  double auxPesoAgua = pesoAgua;
  pesoAgua = state.substring(inicio, fin).toDouble();
  Serial.println("Peso Agua: " + String(auxPesoAgua) + " --> " + String(pesoAgua));

  inicio = fin + 1;
  fin = state.indexOf(separador, inicio);
  double auxPesoComida = pesoComida;
  pesoComida = state.substring(inicio, fin).toDouble();
  Serial.println("Peso Comida: " + String(auxPesoComida) + " --> " + String(pesoComida));

  inicio = fin + 1;
  fin = state.indexOf(separador, inicio);
  double auxDistanciaAgua = distanciaAgua;
  distanciaAgua = state.substring(inicio, fin).toDouble();
  Serial.println("Distancia Agua: " + String(auxDistanciaAgua) + " --> " + String(distanciaAgua));
  distanciaAgua = 100 - (distanciaAgua * 50 / 15);

  inicio = fin + 1;
  fin = state.indexOf(separador, inicio);
  double auxDistanciaComida = distanciaComida;
  distanciaComida = state.substring(inicio, state.length()).toDouble();
  Serial.println("Distancia Comida: " + String(auxDistanciaComida) + " --> " + String(distanciaComida) + "\n");
  distanciaComida = 100 - (distanciaComida * 50 / 15);
}

void enviaEstadoActual() {
  char aux[50];
  String(tempAgua, 2).toCharArray(aux, 50);
  client.publish(TopicTempAgua, aux);

  String(pesoAgua, 1).toCharArray(aux, 50);
  client.publish(TopicPesoAgua, aux);

  String(pesoComida, 1).toCharArray(aux, 50);
  client.publish(TopicPesoComida, aux);

  String(distanciaAgua).toCharArray(aux, 50);
  client.publish(TopicDistanciaAgua, aux);

  String(distanciaComida).toCharArray(aux, 50);
  client.publish(TopicDistanciaComida, aux);
}
