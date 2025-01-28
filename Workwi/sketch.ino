#include <WiFi.h>
#include <PubSubClient.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

const char* ssid = "Wokwi-GUEST";
const char* password = "";
const char* mqtt_server = "test.mosquitto.org"; 
const char* weather_api_key = "5655d2744b4d43ab878132234252301";

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

void getWeatherData() {
  HTTPClient http;
  String url = "http://api.weatherapi.com/v1/current.json?key=" + String(weather_api_key) + "&q=auto:ip";
  http.begin(url);
  
  int httpCode = http.GET();
  if (httpCode > 0) {
    if (httpCode == HTTP_CODE_OK) {
      String payload = http.getString();
      StaticJsonDocument<1024> weatherDoc;
      deserializeJson(weatherDoc, payload);

      const char* location = weatherDoc["location"]["name"];
      const char* region = weatherDoc["location"]["region"];
      const char* country = weatherDoc["location"]["country"];
      float temp_c = weatherDoc["current"]["temp_c"];
      const char* condition = weatherDoc["current"]["condition"]["text"];

      StaticJsonDocument<200> doc;
      doc["Location"] = location;
      doc["Region"] = region;
      doc["Country"] = country;
      doc["Temperature"] = temp_c;
      doc["Condition"] = condition;

      char jsonBuffer[200];
      serializeJson(doc, jsonBuffer);

      client.publish("TOPICO_CLIMA", jsonBuffer);
      Serial.println("Weather data published:");
      Serial.println(jsonBuffer);
    }
  } else {
    Serial.print("Failed to connect to Weather API, HTTP Code: ");
    Serial.println(httpCode);
  }
  http.end();
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
  Serial.println("Sensor data published:");
  Serial.println(jsonBuffer);
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
  if (WiFi.status() == WL_CONNECTED) {
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

void loop() {
  reconnect();
  client.loop();
  Conexao_WIFI();
  Conexao_MQTT();
  Publica_dados();
  getWeatherData();
  delay(10000);  // Atualiza os dados a cada 10 segundos
}
