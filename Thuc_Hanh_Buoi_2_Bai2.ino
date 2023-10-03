#include <DHT.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

#define RELAY_PIN 14
#define DHT_PIN 5

#define DHT_TYPE DHT11

DHT dht(DHT_PIN, DHT_TYPE);

// Wi-Fi Credentials
const char *ssid = "AshHogan";
const char *password = "11111112";

// MQTT Broker Details
String device_id = "Device0001";
const char *mqtt_server = "broker.hivemq.com";
const int mqtt_port = 1883;
const char *mqtt_user = "nguyenhoangviet";
const char *mqtt_password = "123456";
const char *mqtt_clientId = "Deivce_Device0001";
const char *topic_publish = "SensorData"; //Chủ đề mà ESP8266 sử dụng để gửi dữ liệu đến môi giới MQTT
const char *topic_subscribe = "CommandRequest"; //Chủ đề mà ESP8266 sử dụng để nhận lệnh điều khiển từ xa từ môi giới MQTT

WiFiClient esp_client; //Một đối tượng WiFiClient được sử dụng để kết nối với môi giới MQTT
void callback(char *topic, byte *payload, unsigned int length);
PubSubClient mqtt_client(mqtt_server, mqtt_port, callback, esp_client);//Một đối tượng PubSubClient được sử dụng để giao tiếp với môi giới MQTT

// Data Sending Time
unsigned long CurrentMillis, PreviousMillis, DataSendingTime = (unsigned long)1000 * 10;

// Variable
byte lightStatus; // trạng thái đèn bật hoặc tắt
void setup()
{
  Serial.begin(9600);
  pinMode(RELAY_PIN, OUTPUT);//chỉ định làm chân xuất
  dht.begin();
  delay(5000);
  Serial.println("\n\nWelcome to JP Learning\n");
  setup_wifi();
  mqtt_connect();
  digitalWrite(RELAY_PIN, LOW);
}

void loop()
{
  // Đọc nhiệt độ và độ ẩm từ cảm biến DHT11
  float DHT_Temperature = dht.readTemperature();
  float DHT_Humidity = dht.readHumidity();
  if (isnan(DHT_Temperature) || isnan(DHT_Humidity))   // nếu nhiệt độ, độ ẩm đọc không phải là số
  {
    Serial.println("\n\nFailed to read from DHT11 sensor!");
    delay(1000);
  }
  else
  {
    Serial.println("\n\nDHT Temperature: " + String(DHT_Temperature) + " °C");
    Serial.println("DHT Humidity: " + String(DHT_Humidity) + " %");
    Serial.println("Light: " + String(lightStatus) + " " + String(lightStatus == 1 ? "ON" : "OFF"));
    delay(1000);

    // Devices State Sync Request
    CurrentMillis = millis();
    if (CurrentMillis - PreviousMillis > DataSendingTime)
    {
      PreviousMillis = CurrentMillis;

      // Publish Temperature Data (pkt: packet)
      String pkt = "{";
      pkt += "\"device_id\": \"" + device_id + "\", ";
      pkt += "\"type\": \"Temperature\", ";
      pkt += "\"value\": " + String(DHT_Temperature) + "";
      pkt += "}";
      mqtt_publish((char *)pkt.c_str());  // (char *)pkt.c_str(): chuyển đổi thành mảng ký tự

      // Publish Humidity Data
      String pkt2 = "{";
      pkt2 += "\"device_id\": \"" + device_id + "\", ";
      pkt2 += "\"type\": \"Humidity\", ";
      pkt2 += "\"value\": " + String(DHT_Humidity) + "";
pkt2 += "}";
mqtt_publish((char *)pkt2.c_str());
    }
  }

  if (!mqtt_client.loop())
    mqtt_connect();
}

void setup_wifi()
{
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println("\"" + String(ssid) + "\"");

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

//connect to mqtt
void mqtt_connect()
{
  // Loop until we're reconnected
  while (!mqtt_client.connected())
  {
    Serial.println("Attempting MQTT connection...");
    // Attempt to connect
    //    if (mqtt_client.connect(mqtt_clientId, mqtt_user, mqtt_password)) {
    if (mqtt_client.connect(mqtt_clientId))  // if the connection is successful
    {
      Serial.println("MQTT Client Connected");
      // Subscribe
      mqtt_subscribe(topic_subscribe); //Đăng ký với môi giới MQTT để nhận dữ liệu trên chủ đề topic_subscribe (gui tu mqtt den esp8266)
    }
    else
    {
      Serial.print("failed, rc=");
      Serial.print(mqtt_client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}
void mqtt_publish(char *data) //gửi dữ liệu đến môi giới MQTT trên chủ đề đã chỉ định
{
  mqtt_connect();
  Serial.println("Publish Topic: \"" + String(topic_publish) + "\"");
  if (mqtt_client.publish(topic_publish, data))
    Serial.println("Publish \"" + String(data) + "\" ok");
  else
    Serial.println("Publish \"" + String(data) + "\" failed");
}
void mqtt_subscribe(const char *topic)  //đăng ký với môi giới MQTT để nhận dữ liệu trên chủ đề đã chỉ định
{
  if (mqtt_client.subscribe(topic))
    Serial.println("Subscribe \"" + String(topic) + "\" ok");
  else
    Serial.println("Subscribe \"" + String(topic) + "\" failed");
}
void callback(char *topic, byte *payload, unsigned int length)
{
  //1. In ra thông tin của thông điệp được nhận, bao gồm chủ đề và dữ liệu.
  String command;
  Serial.print("\n\nMessage arrived [");
  Serial.print(topic);
  Serial.println("] ");
  for (int i = 0; i < length; i++)
    command += (char)payload[i];

  if (command.length() > 0)
    Serial.println("Command receive is : " + command);

  //2. Deserial hóa dữ liệu thông điệp thành một đối tượng JSON.
  // DynamicJsonDocument doc(1024);
  // deserializeJson(doc, command);
  // JsonObject obj = doc.as<JsonObject>();

  // //3. Lấy ra các giá trị của các khóa "device_id", "type" và "value" từ đối tượng JSON.
  // String id = obj[String("device_id")];
  // String type = obj[String("type")];
  // String value = obj[String("value")];
  // Serial.println("\nCommand device_id is : " + id);
  // Serial.println("Command type is : " + type);
  // Serial.println("Command value is : " + value);

  if (command == "ON")
  {
lightStatus = 1;
digitalWrite(RELAY_PIN, HIGH);
      Serial.println("\nLight ON by Application");
  }else
    {
      lightStatus = 0;
      digitalWrite(RELAY_PIN, LOW);
      Serial.println("\nLight OFF by Application");
    }
    mqtt_publish((char *)command.c_str());
  
}
