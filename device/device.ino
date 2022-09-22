#include <Printers.h>
#include <XBee.h>
#include <stdio.h>

uint8_t HEARTBEAT[] = { 0xe6, 0x21 };
#define BEACON0_HI (0x0013a200)
#define BEACON0_LO (0x403e0f30)

#define BEACON1_HI (0x0013a200)
#define BEACON1_LO (0x403e0f30)

#define BEACON2_HI (0x0013a200)
#define BEACON2_LO (0x403e0f30)

enum Beacon { BEACON0 = 0, BEACON1 = 2, BEACON2 = 4 }
struct
{
  uint8_t beacons[3];
  uint8_t *forBeacon(Beacon beacon)
  {
    return &this->beacons[beacon / 2];
  }
} rssi;
Beacon current_beacon;
XBee *xbee;

void setup() {
  Serial.begin(9800);
  
  xbee = new XBee();
  current_beacon = Beacon::BEACON0;
  rssi.beacons[0] = 0;
  rssi.beacons[1] = 0;
  rssi.beacons[2] = 0;
  
  xbee.begin(Serial1);
}

XBeeAddress64 getAddress(Beacon beacon)
{
  static uint32_t addresses[] = 
  {
    BEACON0_HI,
    BEACON0_LO,
    BEACON1_HI,
    BEACON1_LO,
    BEACON2_HI,
    BEACON2_LO,
  };
  return XBeeAddress64(addresses[beacon], addresses[beacon + 1]);
}

bool probe(Beacon beacon)
{
  auto address = getAddress(beacon);
  auto rssi_store = rssi.forBeacon(beacon);
  
  auto req = ZBTxRequest(address, HEARTBEAT, sizeof(HEARTBEAT));
  xbee.send(req);

  if(xbee.readPacket(1000))
  {
    if(xbee.getResponse().getApiId() == ZX_TX_STATUS_RESPONSE)
    {
      ZBTxStatusResponse response_status;
      xbee.getResponse().getZBTxStatusResponse(response_status);
      
      if (response_status.getDeliveryStatus() == SUCCESS) {
        *rssi.forBeacon(beacon) = xbee.getResponse()
        return true;
      } else {
        return false;
      }
  
  } else return false;
  }
}

void loop() {
  #define PROBE(curr, next) \
    case curr: \
      current_beacon = next; \
      break;
  
  switch(current_beacon)
  {
    PROBE(Beacon::BEACON0, Beacon::BEACON1)
    PROBE(Beacon::BEACON1, Beacon::BEACON2)
    PROBE(Beacon::BEACON2, Beacon::BEACON0)
  }
}
