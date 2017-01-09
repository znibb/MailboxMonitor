#include <Arduino.h>
#include <ESP8266mDNS.h>
#include <ArduinoOTA.h>
#include <PubSubClient.h>

// Load user credentials
#include "credentials.h"

// Pin definitions
#define BUTTON_PIN D1
#define LED_PIN D2

// Parameters
const int mqttReconnectDelay = 5000;

// Variables
byte counter = 0;
long lastMsg = 0;
char msg[50];
volatile bool ledState = false;

// Declare mqtt client object
WiFiClient espClient;
PubSubClient client(espClient);

// Function declaration
void buttonEventHandler();
void callback(char*, byte*, unsigned int);
void reconnect();
void setupOTA();
void setupWifi();

void setup() {
  // Setup serial connection
  Serial.begin(115200);
  Serial.println("Booting");

  // Set pin modes
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  pinMode(BUTTON_PIN, INPUT);

  // Setup ISR
  attachInterrupt(BUTTON_PIN, buttonEventHandler, RISING);

  // Setup connectivity
  setupWifi();
  setupOTA();
  client.setServer(IPAddress(MQTT_IP), MQTT_PORT);
  client.setCallback(callback);
}

void loop(){
  // Run OTA handler to check for incoming OTA connections
  ArduinoOTA.handle();

  // Make sure we are connected and maintain broker connection
  if(!client.connected()){
    reconnect();
  }
  client.loop();

  long now = millis();
  if(now - lastMsg > 2000){
    lastMsg = now;
    ++counter;
    if(counter > 255){
      counter = 0;
    }
    snprintf_P(msg, 50, "%ld", counter);
    if(client.publish("hass/test", msg, true)){
      Serial.print("Published message: ");
      Serial.println(msg);
    }
    else{
      Serial.println("Failed publish");
    }
  }

  if(ledState){
    digitalWrite(LED_PIN, HIGH);
  }
  else{
    digitalWrite(LED_PIN, LOW);
  }
}

// Interupt Service Routine
void buttonEventHandler(){
  ledState = !ledState;
}

// Callback function for MQTT
// Parses and handles communication TO the unit
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}

// Reconnect function
// Handles reconnection to the MQTT broker on startup or or connection loss
void reconnect(){
  // Loop until connected
  while(!client.connected()){
    Serial.println("Attempting connection to MQTT broker");
    if(client.connect(HOSTNAME, MQTT_USER, MQTT_PASSWORD)){
      Serial.println("Connected");
    }
    else{
      Serial.print("Failed, return code='");
      Serial.print(client.state());
      Serial.print("', trying again in ");
      Serial.print(mqttReconnectDelay/1000);
      Serial.println(" s.");
      delay(mqttReconnectDelay);
    }
  }
}

// Setup OTA functionality
void setupOTA(){
  // Set OTA parameters
  ArduinoOTA.setPort(OTA_PORT);
  ArduinoOTA.setHostname(HOSTNAME);
  ArduinoOTA.setPassword((const char *)OTA_PASSWORD);

  // OTA event function calls
  ArduinoOTA.onStart([]() {
    Serial.println("Start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });

  // Start OTA service
  ArduinoOTA.begin();
}

// Establish a wifi connection
void setupWifi(){
  // Select mode
  WiFi.mode(WIFI_STA);

  // Begin connection to network
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  // Wait for connection to establish
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }

  // Print IP
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}
