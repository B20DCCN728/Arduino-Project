/*
      Created by B20DCCN728 - NGUYEN HOANG VIET
*/

#include <Arduino.h>
#include "DHT.h"
#include <ESP8266WiFi.h>
#include <Ticker.h>
#include <AsyncMqttClient.h>

#define WIFI_SSID "Cảnh all in Sin88"
#define WIFI_PASSWORD "5032003@"

//ESP8266, Raspberri Pi Mosquitto MQTT Broker
//192.168.16.104
#define MQTT_HOST IPAddress(192, 168, 16, 104)
// For a cloud MQTT broker, type the domain name
//#define MQTT_HOST "example.com"
#define MQTT_PORT 4000

// Temperature MQTT Topics
#define MQTT_PUB_TEMP "tro/esp/dht/temperature"
#define MQTT_PUB_HUM "tro/esp/dht/humidity"
#define MQTT_PUB_LIG "tro/esp/lm393/lightvalue"
#define MQTT_PUB_VOL "tro/esp/lm393/voltage"
#define MQTT_SUB_LED_1 "tro/esp/led/led_1"
#define MQTT_SUB_LED_2 "tro/esp/led/led_2"

// Digital pin connected to the DHT sensor
#define DHTPIN 2  

// Uncomment whatever DHT sensor type you're using
//#define DHTTYPE DHT11   // DHT 11
// #define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321
#define DHTTYPE DHT11   // DHT 21 (AM2301)   

// Initialize DHT sensor
DHT dht(DHTPIN, DHTTYPE);

//Sensor Pin for Light Sensor
const int sensorPin = A0; 
const int LED_PIN_1 = D1;
const int LED_PIN_2 = D2;

// Variables to hold sensor readings
float temp;
float hum;
int lightValue;
float voltage;
float led_1;
float led_2;

AsyncMqttClient mqttClient;
Ticker mqttReconnectTimer;

WiFiEventHandler wifiConnectHandler;
WiFiEventHandler wifiDisconnectHandler;
Ticker wifiReconnectTimer;

unsigned long previousMillis = 0;   // Stores last time temperature was published
const long interval = 4000;        // Interval at which to publish sensor readings

void connectToWifi() {
  Serial.println("Connecting to Wi-Fi...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
}

void onWifiConnect(const WiFiEventStationModeGotIP& event) {
  Serial.println("Connected to Wi-Fi.");
  connectToMqtt();
}

void onWifiDisconnect(const WiFiEventStationModeDisconnected& event) {
  Serial.println("Disconnected from Wi-Fi.");
  mqttReconnectTimer.detach(); // ensure we don't reconnect to MQTT while reconnecting to Wi-Fi
  wifiReconnectTimer.once(2, connectToWifi);
}

void connectToMqtt() {
  Serial.println("Connecting to MQTT...");
  mqttClient.connect();
}

void onMqttConnect(bool sessionPresent) {
  Serial.println("Connected to MQTT.");
  Serial.print("Session present: ");
  Serial.println(sessionPresent);
  mqttClient.subscribe(MQTT_SUB_LED_1, 0);
  Serial.print("Connected to LED 1 topic");
  mqttClient.subscribe(MQTT_SUB_LED_2, 0);
  Serial.print("Connected to LED 2 topic");
}

void onMqttDisconnect(AsyncMqttClientDisconnectReason reason) {
  Serial.println("Disconnected from MQTT.");

  if (WiFi.isConnected()) {
    mqttReconnectTimer.once(2, connectToMqtt);
  }
}

void onMqttSubscribe(uint16_t packetId, uint8_t qos) {
  Serial.println("Subscribe acknowledged.");
  Serial.print("  packetId: ");
  Serial.println(packetId);
  Serial.print("  qos: ");
  Serial.println(qos);
}

void onMqttUnsubscribe(uint16_t packetId) {
  Serial.println("Unsubscribe acknowledged.");
  Serial.print("  packetId: ");
  Serial.println(packetId);
}

void onMqttPublish(uint16_t packetId) {
  Serial.print("Publish acknowledged.");
  Serial.print("  packetId: ");
  Serial.println(packetId);
}

//Receiving Messages 
void onMqttMessage(char* topic, char* payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total) {
  //Read messages from MQTT 
  //Repaire Null-terminator
  payload[len] = '\0';

  //Convert to String
  String message = String(payload);
  String topic_string = String(topic);

  Serial.println("Received message on topic " + String(topic) + ": " + message);

  //Data processing -> LED 1
  if (strcmp(topic, MQTT_SUB_LED_1) == 0) {

    Serial.println("Topic: " + topic_string + " have message: " + message + "");

    if (message == "ON") 

      digitalWrite(LED_PIN_1, HIGH);

    else if (message == "OFF") 

      digitalWrite(LED_PIN_1, LOW);

    else 

      Serial.println("Wrong syntax!!");
  //Data processing -> LED 2
  } else if (strcmp(topic, MQTT_SUB_LED_2) == 0) {

    Serial.println("Topic: " + topic_string + " have message: " + message + "");

    if (message == "ON") 

      digitalWrite(LED_PIN_2, HIGH);

    else if (message == "OFF") 

      digitalWrite(LED_PIN_2, LOW);

    else 

      Serial.println("Wrong syntax!!");

  } else {

    Serial.println("Not received data from MQTT broker, right now!!!");

  }
}

void setup() {
  Serial.begin(115200);
  Serial.println();

  pinMode(LED_PIN_1, OUTPUT);
  pinMode(LED_PIN_2, OUTPUT);

  dht.begin();
  
  wifiConnectHandler = WiFi.onStationModeGotIP(onWifiConnect);
  wifiDisconnectHandler = WiFi.onStationModeDisconnected(onWifiDisconnect);
  
  mqttClient.onConnect(onMqttConnect);
  mqttClient.onDisconnect(onMqttDisconnect);
  mqttClient.onSubscribe(onMqttSubscribe);
  mqttClient.onUnsubscribe(onMqttUnsubscribe);
  mqttClient.onPublish(onMqttPublish);
  mqttClient.setServer(MQTT_HOST, MQTT_PORT);
  // If your broker requires authentication (username and password), set them below
  //mqttClient.setCredentials("REPlACE_WITH_YOUR_USER", "REPLACE_WITH_YOUR_PASSWORD");
  mqttClient.onMessage(onMqttMessage);

  connectToWifi();
}

void loop() {
  unsigned long currentMillis = millis();
  // Every X number of seconds (interval = 10 seconds) 
  // it publishes a new MQTT message
  if (currentMillis - previousMillis >= interval) {
    // Save the last time a new reading was published
    previousMillis = currentMillis;
    // New DHT sensor readings
    hum = dht.readHumidity();
    // Read temperature as Celsius (the default)
    temp = dht.readTemperature();
    // Read temperature as Fahrenheit (isFahrenheit = true)
    //temp = dht.readTemperature(true);
    
    // Publish an MQTT message on topic esp/dht/temperature
    uint16_t packetIdPub1 = mqttClient.publish(MQTT_PUB_TEMP, 1, true, String(temp).c_str());                            
    Serial.printf("Publishing on topic %s at QoS 1, packetId: %i ", MQTT_PUB_TEMP, packetIdPub1);
    Serial.printf("Message: %.2f \n", temp);

    // Publish an MQTT message on topic esp/dht/humidity
    uint16_t packetIdPub2 = mqttClient.publish(MQTT_PUB_HUM, 1, true, String(hum).c_str());                            
    Serial.printf("Publishing on topic %s at QoS 1, packetId %i: ", MQTT_PUB_HUM, packetIdPub2);
    Serial.printf("Message: %.2f \n", hum);
    delay(2000);

    //Read light value as Luxz
    lightValue = analogRead(sensorPin);  
    //Read voltage as Vol
    voltage = lightValue * (3.3 / 1023);  
    // Publish an MQTT message on topic esp/lm393/lightvalue
    uint16_t packetIdPub3 = mqttClient.publish(MQTT_PUB_LIG, 1, true, String(lightValue).c_str());                          
    Serial.printf("Publishing on topic %s at QoS 1, packetId: %i ", MQTT_PUB_LIG, packetIdPub3);
    Serial.printf("Message: %d \n", lightValue);    

    // Publish an MQTT message on topic esp/lm393/voltage
    uint16_t packetIdPub4 = mqttClient.publish(MQTT_PUB_VOL, 1, true, String(voltage).c_str());                            
    Serial.printf("Publishing on topic %s at QoS 1, packetId: %i ", MQTT_PUB_VOL, packetIdPub4);
    Serial.printf("Message: %.2f \n", voltage);    

  }


}
