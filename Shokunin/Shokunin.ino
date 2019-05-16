#include <Adafruit_ADS1015.h>

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <elapsedMillis.h>

#include "configuration.h" // This is the configuration file with passwords and stuff

#define MQTT_SERVER "mqtt.pndsn.com"
#define MQTT_CHANNEL "pubnub-sensor-network"
#define MQTT_CLIENT_ID "demo/sub-c-5f1b7c8e-fbee-11e3-aa40-02ee2ddab7fe/knichols"

// TO keep track of time 
elapsedMillis timeSinceCreated;
elapsedMillis timeSinceRead;
unsigned int thirtySecInterval = 30000;
unsigned int tenSecInterval = 10000;

// Initialize the Ethernet client object
WiFiClient espClient;
PubSubClient client(espClient);

void callback(char* topic, byte* payload, unsigned int length) {
 
  Serial.print("Message arrived in topic: ");
  Serial.println(topic);
 
  Serial.print("Message:");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
 
  Serial.println();
  Serial.println("-----------------------");
 
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial.printf(" ESP8266 Chip id = %08X\n", ESP.getChipId());

  setup_wifi();
  
  client.setServer(MQTT_SERVER, 1883);
  client.setCallback(callback);

  // Make sure we're connected to MQTT
  if (!client.connected()) {
    reconnect();
  }

  // Now subscribe
  client.subscribe(MQTT_CHANNEL);
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

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(MQTT_CLIENT_ID)) {
      Serial.println("connected");
      // Once connected, publish an announcement...
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void loop() {
  
  if (timeSinceRead > tenSecInterval) 
  {       
    // We periodically check that the client is connected, and if not,
    // then we reconnect.
    if (!client.connected()) {
      reconnect();
    }
    timeSinceRead = 0;
  }

  client.loop();
}
