#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <SoftwareSerial.h>
SoftwareSerial serial_arduino(D4, D3); //RX y TX

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

//Tópicos de actualización de estado de Home Assistant
char topicoGPS[] = "localizacion/mascota";
String state;
char caracter;

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
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "WemosMascota";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str(), "mqtt", "mqtt")) {
      Serial.println("connected");
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
  serial_arduino.begin(9600);

  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

}

void loop() {
  if (!client.connected()) {
    reconnect(); 
    
    delay(100);
    String inicio = "{\"longitude\": -3.787956,\"gps_accuracy\": 60,\"latitude\": 37.769145,\"battery_level\": 99.9}";
    actualizaEstado(inicio);
    delay(1000);  
  }
  client.loop();



  while (serial_arduino.available() > 0) {
    caracter = serial_arduino.read();
    state.concat(caracter);
    if (caracter == '#') {
      actualizaEstado(state.substring(0, state.length() - 1)); //Quitamos el carácter que indica el final del mensaje
      state = "";
      Serial.println();
    }
  }
}

void actualizaEstado(String state) {
  Serial.print("Actualización del estado actual: ");
  Serial.println(state);
  
  char json[100];
  state.toCharArray(json, 100);
  client.publish(topicoGPS, json);
}
