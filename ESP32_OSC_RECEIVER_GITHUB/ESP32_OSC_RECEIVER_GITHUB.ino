/*---------------------------------------------------------------------------------------------
OSC receiver for ESP32 D1 Mini
POST - Carles Viarnès
L'Auditori - 26/04/2025
To  upload, short IO0 and GND and press reset
Last edit: 12/04/2025
--------------------------------------------------------------------------------------------- */
#ifdef ESP8266
#include <ESP8266WiFi.h>
#else
#include <WiFi.h>
#endif
#include <WiFiUdp.h>
#include <OSCMessage.h>
//#include <OSCBundle.h>
#include <OSCData.h>

// WiFi network
char ssid[] = "ssid";      // your network SSID (name)
char pass[] = "password";  // your network password

// Static IP configuration
const int id = 207;                    // specify the ID of the board you are programming
IPAddress staticIP(192, 168, 10, id);  // ESP32 static IP
IPAddress gateway(192, 168, 10, 1);    // IP Address of your network gateway (router)
IPAddress subnet(255, 255, 255, 0);    // Subnet mask
char address[10];                      // hold the id as OSC address

// SERIAL ON/OFF
#define SERIAL_ON false
#define SERIAL \
  if (SERIAL_ON) Serial

// UDP instance to let us send and receive packets over UDP
WiFiUDP Udp;
const IPAddress outIp(192, 168, 10, 123);  // remote IP (static IP of the computer)
const unsigned int outPort = 9999;         // remote port (computer)
const unsigned int localPort = 8888;       // local port to listen for UDP packets

OSCErrorCode error;
bool mosfetState = LOW;
unsigned int ledVal = 0;
bool reconnecting = false;

unsigned long previousMillis = 0;
unsigned long interval = 50000;

// GPIO PINS
const int mosfetPin = 32;
const int ledPin = 4;
#ifndef BUILTIN_LED
#ifdef LED_BUILTIN
#define BUILTIN_LED LED_BUILTIN
#else
#define BUILTIN_LED 13
#endif
#endif

void blink() {
  bool blink = 0;
  for (int i = 0; i < 6; i++) {
    blink = !blink;
    digitalWrite(BUILTIN_LED, blink);
    digitalWrite(ledPin, blink);
    delay(75);
  }
}

void setup() {
  pinMode(BUILTIN_LED, OUTPUT);
  pinMode(mosfetPin, OUTPUT);
  pinMode(ledPin, OUTPUT);
  snprintf(address, sizeof(address), "/%d", id);  // Format id into address

  SERIAL.begin(115200);
  blink();

  // Connect to WiFi network
  SERIAL.println();
  SERIAL.print("Connecting to ");
  SERIAL.println(ssid);
  WiFi.begin(ssid, pass);
  WiFi.setSleep(true);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    SERIAL.print(".");
  }
  SERIAL.println("");

  // Configure static IP
  WiFi.config(staticIP);
  SERIAL.println("WiFi connected");
  SERIAL.println("IP address: ");
  SERIAL.println(WiFi.localIP());

  SERIAL.println("Starting UDP");
  Udp.begin(localPort);
  SERIAL.print("Local port: ");
#ifdef ESP32
  SERIAL.println(localPort);
#else
  SERIAL.println(Udp.localPort());
#endif
  blink();
}

void mosfet(OSCMessage &msg) {
  mosfetState = msg.getInt(0);
  digitalWrite(mosfetPin, mosfetState);
  SERIAL.print("/mosfet: ");
  SERIAL.println(mosfetState);
  //sendOSC("/mosfet/ok");
}

void led(OSCMessage &msg) {
  ledVal = msg.getInt(0);
  analogWrite(ledPin, ledVal);
  SERIAL.print("/led: ");
  SERIAL.println(ledVal);
  //sendOSC("/led/ok");
}

void ping(OSCMessage &msg) {
  sendOSC("/ping/ok");
}

bool sendOSC(const char *message) {
  OSCMessage msg(address);
  msg.add(message);
  SERIAL.print("Sending OSC Message: ");
  SERIAL.println(message);
  Udp.beginPacket(outIp, outPort);
  msg.send(Udp);
  bool success = Udp.endPacket();  // Capture success state
  msg.empty();
  return success;
}

void loop() {

  // Receive OSC messages
  OSCMessage msg;
  int size = Udp.parsePacket();
  if (size > 0) {
    while (size--) {
      msg.fill(Udp.read());
    }
    if (!msg.hasError()) {
      msg.dispatch("/mosfet", mosfet);
      msg.dispatch("/led", led);
      msg.dispatch("/ping", ping);
    } else {
      error = msg.getError();
      SERIAL.print("error: ");
      SERIAL.println(error);
      sendOSC("/report/error");
    }
  }

  // if WiFi is down, try reconnecting
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    if (WiFi.status() != WL_CONNECTED) {
      reconnecting = true;
      SERIAL.print(millis());
      SERIAL.println(" Reconnecting to WiFi...");
      WiFi.disconnect();
      WiFi.reconnect();
      previousMillis = currentMillis;
    }
  }
  if (reconnecting && WiFi.status() == WL_CONNECTED) {
    SERIAL.println("Reconnected!");
    sendOSC("/report/reconnected");
    reconnecting = false;
  }
  delay(10);
}