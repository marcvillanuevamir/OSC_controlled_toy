# OSC_controlled_toy

This project hacks a battery-powered toy in order to turn it on/off remotely via WiFi. I used the WeMos D1 Mini ESP32 for its reduced size, so it could easily fit inside the toy, and the WeMos battery shield with a 18500 LiPo battery (3.7V, 1200mAh). The ESP32 connects to a WiFi network as a UDP client, and listens for OSC messages. I use a MOSFET N-channel transistor (IRL540N) as a switch attached to the toy's inner driver, which in turn controls the toy's motor/s. I also desoldered the wires that connected a LED to the toy's driver and connected them to a PWM pin of the ESP32, so I could also control that LED with OSC messages.
