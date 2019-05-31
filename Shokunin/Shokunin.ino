#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include "RunningAverage.h"
#include <elapsedMillis.h>
#include <ArduinoJson.h>
#define PubNub_BASE_CLIENT WiFiClient
#include <PubNub.h>
#include <Servo.h>

#include "configuration.h" // This is the configuration file with passwords and stuff

#define MQTT_SERVER "mqtt.pndsn.com"
#define MQTT_CHANNEL "pubnub-sensor-network"
#define MQTT_CLIENT_ID "demo/sub-c-5f1b7c8e-fbee-11e3-aa40-02ee2ddab7fe/knichols"

#define SERVO_PIN 2

StaticJsonDocument<256> doc;

// TO keep track of time 
elapsedMillis timeSinceCreated;
elapsedMillis timeSinceRead;
unsigned int thirtySecInterval = 30000;
unsigned int tenSecInterval = 10000;

// For the servo object
Servo myservo;


// Construct the moving averages classes
RunningAverage humidityRA(50);
RunningAverage ambient_temperatureRA(50);
RunningAverage radiation_levelRA(50);
RunningAverage photosensorRA(50);

// Initialize the Ethernet client object
WiFiClient espClient;


// Initialise a web server
ESP8266WebServer server(80);

// For PUBNUB
const static char pubkey[]  = "demo";        // your publish key
const static char subkey[]  = "sub-c-5f1b7c8e-fbee-11e3-aa40-02ee2ddab7fe";        // your subscribe key
const static char channel[] = "pubnub-sensor-network"; // channel to use

// To manage global state

enum Sensor {AMBIENT_TEMPERATURE, HUMIDITY, RADIATION_LEVEL, PHOTOSENSOR};
String global_sensor_group = "probe-1";
Sensor global_sensor = AMBIENT_TEMPERATURE;
String global_smoothing = "false";


void setup() {
  Serial.begin(115200);
  Serial.printf(" ESP8266 Chip id = %08X\n", ESP.getChipId());

  setup_wifi();

  server.on("/", handleRoot);
  server.on("/state", handleState);
  server.begin();
  PubNub.begin(pubkey, subkey);
  myservo.attach(SERVO_PIN);
  
}

void handleRoot() {
  server.send(200, "text/plain", "Please GET /state?probe_group=probe-10&smoothing=true&sensor=humidity");
  Serial.println("Root request");
}

void handleState() {

  String probe_group = server.arg("probe_group");
  String smoothing = server.arg("smoothing");
  String sensor = server.arg("sensor");

  if (probe_group.length() > 8 || smoothing.length() > 5 || sensor.length() > 18) {
    server.send(403, "text/plain", "One of the values you sent was wrong.");
  } else {
    global_sensor_group = probe_group;
    global_smoothing = smoothing;
    server.send(200, "text/plain", "OK");
    
    if (sensor == "temperature") {
      Serial.println("SETTING OUTPUT TO: " + global_sensor_group + " / AMBIENT_TEMPERATURE");
      global_sensor = AMBIENT_TEMPERATURE;
    } else if (sensor == "humidity") {
      Serial.println("SETTING OUTPUT TO: " + global_sensor_group + " /  HUMIDITY");
      global_sensor = HUMIDITY;
    } else if (sensor == "radiation") {
      Serial.println("SETTING OUTPUT TO: " + global_sensor_group + " /  RADIATION");
      global_sensor = RADIATION_LEVEL;
    } else if (sensor == "photosensor") {
      Serial.println("SETTING OUTPUT TO: " + global_sensor_group + " /  PHOTOSENSOR");
      global_sensor = PHOTOSENSOR;
    }
  }
}

void handleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}

void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

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

void parse_sensors_move_servo(String msg) {
  
  deserializeJson(doc, msg);
  JsonObject root = doc.as<JsonObject>(); // get the root object
  
  long timestamp = root["timestamp"];
  float ambient_temperature = root["ambient_temperature"];
  float humidity = root["humidity"];
  const char* sensor_uuid = root["sensor_uuid"];
  float radiation_level = root["radiation_level"];
  float photosensor = root["photosensor"];
  
  String sensor_group = String(sensor_uuid).substring(0, 7);

  if (sensor_group == global_sensor_group && global_smoothing == "true") {
    
    humidityRA.addValue(humidity);
    ambient_temperatureRA.addValue(ambient_temperature);
    radiation_levelRA.addValue(radiation_level);
    photosensorRA.addValue(photosensor);

    Serial.print(sensor_uuid);
    Serial.print(" - SMOOTHING - ");
    
    float valuesToDisplay[4] = {humidityRA.getAverage(), ambient_temperatureRA.getAverage(), radiation_levelRA.getAverage(), photosensorRA.getAverage()};
    setServoPosition(valuesToDisplay);
    
  } else if (sensor_group == global_sensor_group && global_smoothing == "false")  {
    Serial.print(sensor_uuid);
    Serial.print(" - NOT SMOOTHING - ");

    float valuesToDisplay[4] = {humidity, ambient_temperature, radiation_level, photosensor};
    setServoPosition(valuesToDisplay);
  } else if (String("all") == global_sensor_group && global_smoothing == "true")  {
    
    humidityRA.addValue(humidity);
    ambient_temperatureRA.addValue(ambient_temperature);
    radiation_levelRA.addValue(radiation_level);
    photosensorRA.addValue(photosensor);
    
    Serial.print(sensor_uuid);
    Serial.print(" - SMOOTHING - ");
    
    float valuesToDisplay[4] = {humidityRA.getAverage(), ambient_temperatureRA.getAverage(), radiation_levelRA.getAverage(), photosensorRA.getAverage()};
    setServoPosition(valuesToDisplay);
  } else if (String("all") == global_sensor_group && global_smoothing == "false")  {
    
    Serial.print(sensor_uuid);
    Serial.print(" - NOT SMOOTHING - ");

    float valuesToDisplay[4] = {humidity, ambient_temperature, radiation_level, photosensor};
    setServoPosition(valuesToDisplay);
  } else if (String("probe-10") == global_sensor_group && global_smoothing == "false" && isAlpha(sensor_group.charAt(6)))   {

    Serial.print(sensor_uuid);
    Serial.print(" - NOT SMOOTHING - ");

    float valuesToDisplay[4] = {humidity, ambient_temperature, radiation_level, photosensor};
    setServoPosition(valuesToDisplay);
  } else if (String("probe-10") == global_sensor_group && global_smoothing == "true" && isAlpha(sensor_group.charAt(6)))   {

    humidityRA.addValue(humidity);
    ambient_temperatureRA.addValue(ambient_temperature);
    radiation_levelRA.addValue(radiation_level);
    photosensorRA.addValue(photosensor);
    
    Serial.print(sensor_uuid);
    Serial.print(" - SMOOTHING - ");
    
    float valuesToDisplay[4] = {humidityRA.getAverage(), ambient_temperatureRA.getAverage(), radiation_levelRA.getAverage(), photosensorRA.getAverage()};
    setServoPosition(valuesToDisplay);
  }

}

void setServoPosition(float* values){
  
    float value;
    float pos;
  
    switch (global_sensor) {
    case AMBIENT_TEMPERATURE:
      pos = 180 - (values[1] / 35 * 180);
      Serial.print(values[1]);
      Serial.print(" - MOVING SERVO TO: ");
      Serial.println(pos);
      myservo.write(pos);
      break;
    case HUMIDITY:
      pos = 180 - (values[0] / 90 * 180);
      Serial.print(values[0]);
      Serial.print(" - MOVING SERVO TO: ");
      Serial.println(pos);
      myservo.write(pos);
      break;
    case RADIATION_LEVEL:
      pos = 180 - (values[2] / 225 * 180);
      Serial.print(values[2]);
      Serial.print(" - MOVING SERVO TO: ");
      Serial.println(pos);
      myservo.write(pos);
      break;
    case PHOTOSENSOR:
      pos = 180 - (values[3] / 900 * 180);
      Serial.print(values[3]);
      Serial.print(" - MOVING SERVO TO: ");
      Serial.println(pos);
      myservo.write(pos);
      break;
    default:
      // statements
      Serial.println("Default case reached");
      break;
    }
}


void loop() {
  server.handleClient();

  /* Wait for sensor data. */
  PubSubClient *sclient = PubNub.subscribe(MQTT_CHANNEL);
  if (!sclient) return; // error
  String msg;
  SubscribeCracker ritz(sclient);
  while (!ritz.finished()) {
      ritz.get(msg);
      if (msg.length() > 0) {
        // Values to display
          parse_sensors_move_servo(msg);
      }
  }
  sclient->stop();

}
