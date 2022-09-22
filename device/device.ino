#include <Printers.h>
#include <XBee.h>
#include <stdio.h>

#define RSSI_A (18.0)
#define RSSI_N (2.0)

#define BEACON0 (XBeeAddress64(0x8877665544, 0x33221100))
#define BEACON1 (XBeeAddress64(0x8877665544, 0x33221100))
#define BEACON2 (XBeeAddress64(0x8877665544, 0x33221100))

#define BEACON0_POS (Vec2f{.x = 0.0, .y = 1.0})
#define BEACON1_POS (Vec2f{.x = 0.0, .y = 0.0})
#define BEACON2_POS (Vec2f{.x = 1.0, .y = 0.0})

struct Vec2f
{
  float x;
  float y;
}

struct
{
  uint8_t beacon0;
  uint8_t beacon1;
  uint8_t beacon2;

  uint8_t init;
  
} rssi;
XBee *xbee;

void setup() {
  Serial.begin(9800);
  
  xbee = new XBee();
  rssi.beacon0 = 0;
  rssi.beacon1 = 0;
  rssi.beacon2 = 0;
  rssi.init = 0;
  
  xbee->begin(Serial1);
}

float d(uint8_t rssi)
{
  float base = 1.0;
  float exponent = ((float)rssi + RSSI_A) / 10.0 * RSSI_N;

  return exp(10.0, exponent);
}

Vec2f position()
{
  auto beacon0 = d(rssi.beacon0);
  auto beacon1 = d(rssi.beacon1);
  auto beacon2 = d(rssi.beacon2);

   
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
      {
        rssi.beacon0 = response.getRssi();
        rssi.init |= 1;
      }
      else if(address == BEACON1)
      {
        rssi.beacon1 = response.getRssi();
        rssi.init |= 2;
      }
      else if(address == BEACON2)
      {
        rssi.beacon2 = response.getRssi();
        rssi.init |= 4;
      }

      Serial.print("Received beacon packet");
    }
  }

  if(rssi.init == 7)
  {
    
  }
}
