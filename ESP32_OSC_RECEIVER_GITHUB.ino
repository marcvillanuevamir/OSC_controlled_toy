/*---------------------------------------------------------------------------------------------
OSC receiver for ESP32 D1 Mini
POST - Carles Viarnès
L'Auditori - 26/04/2025
if needed, short IO0 and GND, press reset and run in Terminal: python3 -m esptool --chip esp32 erase_flash

--------------------------------------------------------------------------------------------- */
#ifdef ESP8266
#include <ESP8266WiFi.h>
#else
#include <WiFi.h>
#endif
#include <WiFiUdp.h>
#include <OSCMessage.h>
#include <OSCBundle.h>
#include <OSCData.h>

char ssid[] = "ssid";      // your network SSID (name)
char pass[] = "password";  // your network password

// Static IP configuration
IPAddress staticIP(192, 168, 10, 205);  // ESP32 static IP
IPAddress gateway(192, 168, 10, 1);     // IP Address of your network gateway (router)
IPAddress subnet(255, 255, 255, 0);     // Subnet mask

// SERIAL ON/OFF
#define SERIAL_ON false
#define SERIAL \
  if (SERIAL_ON) Serial

// A UDP instance to let us send and receive packets over UDP
WiFiUDP Udp;
//const IPAddress outIp(192, 168, 10, 133);  // remote IP (not needed for receive)
//const unsigned int outPort = 9999;        // remote port (not needed for receive)
const unsigned int localPort = 8888;  // local port to listen for UDP packets (here's where we send the packets)

OSCErrorCode error;
unsigned int mosfetState = LOW;
unsigned int ledVal = 0;

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
    delay(75);
  }
  digitalWrite(BUILTIN_LED, 0);
}

void setup() {
  pinMode(BUILTIN_LED, OUTPUT);
  pinMode(mosfetPin, OUTPUT);
  pinMode(ledPin, OUTPUT);

  SERIAL.begin(115200);
  blink();

  // Connect to WiFi network
  SERIAL.println();
  SERIAL.println();
  SERIAL.print("Connecting to ");
  SERIAL.println(ssid);
  WiFi.begin(ssid, pass);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    SERIAL.print(".");
  }
  SERIAL.println("");

  // Configuring static IP
  if (!WiFi.config(staticIP, gateway, subnet)) {
    SERIAL.println("Failed to configure Static IP");
  } else {
    SERIAL.println("Static IP configured!");
  }

  SERIAL.println("WiFi connected");
  SERIAL.println("IP address: ");
  SERIAL.println(WiFi.localIP());

  SERIAL.println("Starting UDP");
  Udp.begin(localPort);
  SERIAL.print("Local port: ");
  blink();
#ifdef ESP32
  SERIAL.println(localPort);
#else
  SERIAL.println(Udp.localPort());
#endif
}

void mosfet(OSCMessage &msg) {
  mosfetState = msg.getInt(0);
  //digitalWrite(BUILTIN_LED, mosfetState);
  digitalWrite(mosfetPin, mosfetState);
  SERIAL.print("/mosfet: ");
  SERIAL.println(mosfetState);
}

void led(OSCMessage &msg) {
  ledVal = msg.getInt(0);
  analogWrite(ledPin, ledVal);
  SERIAL.print("/led: ");
  SERIAL.println(ledVal);
}

void loop() {
  OSCMessage msg;
  int size = Udp.parsePacket();

  if (size > 0) {
    while (size--) {
      msg.fill(Udp.read());
    }
    if (!msg.hasError()) {
      msg.dispatch("/mosfet", mosfet);
      msg.dispatch("/led", led);
    } else {
      error = msg.getError();
      SERIAL.print("error: ");
      SERIAL.println(error);
    }
  }
}