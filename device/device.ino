#include <Printers.h>
#include <XBee.h>
#include <stdio.h>

#define BEACON0 (XBeeAddress64(0x8877665544, 0x33221100))
#define BEACON1 (XBeeAddress64(0x8877665544, 0x33221100))
#define BEACON2 (XBeeAddress64(0x8877665544, 0x33221100))

struct
{
  uint8_t beacon0;
  uint8_t beacon1;
  uint8_t beacon2;
} rssi;
XBee *xbee;

void setup() {
  Serial.begin(9800);
  
  xbee = new XBee();
  rssi.beacon0 = 0;
  rssi.beacon1 = 0;
  rssi.beacon2 = 0;
  
  xbee->begin(Serial1);
}

void loop() {
  if(xbee->readPacket(1000))
  {
    if(xbee->getResponse().getApiId() == Rx64Response::API_ID)
    {
      Rx64Response response;
      xbee->getResponse().getRx64Response(response);

      auto address = response.getRemoteAddress64();
      if(address == BEACON0)
        rssi.beacon0 = response.getRssi();
      else if(address == BEACON1)
        rssi.beacon1 = response.getRssi();
      else if(address == BEACON2)
        rssi.beacon2 = response.getRssi();

      Serial.print("Received beacon packet");
    }
  }
}
