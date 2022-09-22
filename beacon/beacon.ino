#include <Printers.h>
#include <XBee.h>
#include <stdio.h>

#define TARGET (XBeeAddress64(0x8877665544, 0x33221100))
XBee *xbee;

void setup() {
  Serial.begin(9800);
  
  xbee = new XBee();
  xbee->begin(Serial1);
}

void loop() {
  uint8_t data[] = { 0xe6, 0x21 };
  auto address = TARGET;
  auto packet = Tx64Request(address, data, sizeof(data));
  
  xbee->send(packet);
}
