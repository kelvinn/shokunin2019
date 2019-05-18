#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include "RunningAverage.h"
#include <elapsedMillis.h>
#include <ArduinoJson.h>
#define PubNub_BASE_CLIENT WiFiClient
#include <PubNub.h>

#include "configuration.h" // This is the configuration file with passwords and stuff

#define MQTT_SERVER "mqtt.pndsn.com"
#define MQTT_CHANNEL "pubnub-sensor-network"
#define MQTT_CLIENT_ID "demo/sub-c-5f1b7c8e-fbee-11e3-aa40-02ee2ddab7fe/knichols"

StaticJsonDocument<256> doc;

// TO keep track of time 
elapsedMillis timeSinceCreated;
elapsedMillis timeSinceRead;
unsigned int thirtySecInterval = 30000;
unsigned int tenSecInterval = 10000;

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
String global_sensor_group = "probe-1";
String global_sensor = "ambient_temperature";
String global_smoothing = "true";

void setup() {
  Serial.begin(115200);
  Serial.printf(" ESP8266 Chip id = %08X\n", ESP.getChipId());

  setup_wifi();

  server.on("/", handleRoot);
  server.on("/state", handleState);
  server.begin();
  PubNub.begin(pubkey, subkey);
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
    global_sensor = sensor;
    global_smoothing = smoothing;
    server.send(200, "text/plain", "OK");
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

void parse_sensors(String msg) {
  
  deserializeJson(doc, msg);
  JsonObject root = doc.as<JsonObject>(); // get the root object
  
  long timestamp = root["timestamp"];
  const char* ambient_temperature = root["ambient_temperature"];
  const char* humidity = root["humidity"];
  const char* sensor_uuid = root["sensor_uuid"];
  const char* radiation_level = root["radiation_level"];
  const char* photosensor = root["photosensor"];
  
  String sensor_group = String(sensor_uuid).substring(0, 7);

  if (sensor_group == global_sensor_group && global_smoothing == "true") {
    
    humidityRA.addValue(atof(humidity));
    ambient_temperatureRA.addValue(atof(ambient_temperature));
    radiation_levelRA.addValue(atof(radiation_level));
    photosensorRA.addValue(atof(photosensor));
    Serial.print(sensor_uuid);
    Serial.print(" - SMOOTHING - Humidity: ");
    Serial.print(humidityRA.getAverage(), 3);

    Serial.print(", Ambient Temperature: ");
    Serial.print(ambient_temperatureRA.getAverage(), 3);

    Serial.print(", Radiation Level: ");
    Serial.print(radiation_levelRA.getAverage(), 3);

    Serial.print(", Photosensor: ");
    Serial.println(photosensorRA.getAverage(), 3);

  } else if (sensor_group == global_sensor_group && global_smoothing == "false")  {
    Serial.print(sensor_uuid);
    Serial.print(" - NOT SMOOTHING - Humidity: ");
    Serial.print(humidity);

    Serial.print(", Ambient Temperature: ");
    Serial.print(ambient_temperature);

    Serial.print(", Radiation Level: ");
    Serial.print(radiation_level);

    Serial.print(", Photosensor: ");
    Serial.println(photosensor);
    
  } else if (String("all") == global_sensor_group && global_smoothing == "true")  {
    
    humidityRA.addValue(atof(humidity));
    ambient_temperatureRA.addValue(atof(ambient_temperature));
    radiation_levelRA.addValue(atof(radiation_level));
    photosensorRA.addValue(atof(photosensor));
    
    Serial.print(sensor_uuid);
    Serial.print(" - SMOOTHING - Humidity: ");
    Serial.print(humidityRA.getAverage(), 3);

    Serial.print(", Ambient Temperature: ");
    Serial.print(ambient_temperatureRA.getAverage(), 3);

    Serial.print(", Radiation Level: ");
    Serial.print(radiation_levelRA.getAverage(), 3);

    Serial.print(", Photosensor: ");
    Serial.println(photosensorRA.getAverage(), 3);
    
  } else if (String("all") == global_sensor_group && global_smoothing == "false")  {
    Serial.print(sensor_uuid);
    Serial.print(" - NOT SMOOTHING - Humidity: ");
    Serial.print(humidity);

    Serial.print(", Ambient Temperature: ");
    Serial.print(ambient_temperature);

    Serial.print(", Radiation Level: ");
    Serial.print(radiation_level);

    Serial.print(", Photosensor: ");
    Serial.println(photosensor);
  } else if (String("probe-10") == global_sensor_group && global_smoothing == "false" && isAlpha(sensor_group.charAt(6)))   {

    Serial.print(sensor_uuid);
    Serial.print(" - NOT SMOOTHING - Humidity: ");
    Serial.print(humidity);

    Serial.print(", Ambient Temperature: ");
    Serial.print(ambient_temperature);

    Serial.print(", Radiation Level: ");
    Serial.print(radiation_level);

    Serial.print(", Photosensor: ");
    Serial.println(photosensor);
  } else if (String("probe-10") == global_sensor_group && global_smoothing == "true" && isAlpha(sensor_group.charAt(6)))   {

    humidityRA.addValue(atof(humidity));
    ambient_temperatureRA.addValue(atof(ambient_temperature));
    radiation_levelRA.addValue(atof(radiation_level));
    photosensorRA.addValue(atof(photosensor));
    
    Serial.print(sensor_uuid);
    Serial.print(" - SMOOTHING - Humidity: ");
    Serial.print(humidityRA.getAverage(), 3);

    Serial.print(", Ambient Temperature: ");
    Serial.print(ambient_temperatureRA.getAverage(), 3);

    Serial.print(", Radiation Level: ");
    Serial.print(radiation_levelRA.getAverage(), 3);

    Serial.print(", Photosensor: ");
    Serial.println(photosensorRA.getAverage(), 3);
    
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
          parse_sensors(msg);
      }
  }
  sclient->stop();

}
