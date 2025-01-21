#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

const char* ssid = "Wokwi-GUEST";
const char* password = "";
const char* mqtt_server = "test.mosquitto.org"; 

String my_payload;
String my_topic;

WiFiClient WOKWI_Client;
PubSubClient client(WOKWI_Client);

const float BETA = 3950; 

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  String conc_payload;
  for (int i = 0; i < length; i++) {
    conc_payload += ((char)payload[i]);
  }
  my_payload = conc_payload;
  my_topic = topic;
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("WOKWI_Client")) {
      Serial.println("connected");
      client.subscribe("ESP32_RECEBE");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void setup() {
  pinMode(2, OUTPUT);     
  pinMode(15, OUTPUT);
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void Conexao_WIFI() {
  if (WiFi.status()) {
    digitalWrite(2, HIGH);
  } else {
    digitalWrite(2, LOW);
  }
}

void Conexao_MQTT() {
  if (client.connected()) {
    digitalWrite(15, HIGH);
  } else {
    digitalWrite(15, LOW);
  }
}


void Publica_dados() {
  int Valor_POT = analogRead(34);
  int Pot_Percent = map(Valor_POT, 0, 4095, 0, 100);

  int analogValue = analogRead(35);
  float celsius = 1 / (log(1 / (4095. / analogValue - 1)) / BETA + 1.0 / 298.15) - 273.15;

  celsius = round(celsius * 10) / 10.0;

  StaticJsonDocument<200> doc;
  doc["Potenciometro"] = Pot_Percent;
  doc["Temperatura"] = celsius;

  char jsonBuffer[200];
  serializeJson(doc, jsonBuffer);

  client.publish("TOPICO_WOKWI", jsonBuffer);
  
  delay(1000);
}

void loop() {
  reconnect();
  client.loop();
  Conexao_WIFI();
  Conexao_MQTT();
  Publica_dados();
}
