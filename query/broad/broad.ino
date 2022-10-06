#include <Printers.h>
#include <XBee.h>
#include <stdio.h>

#define TARGET (XBeeAddress64(0x0, 0xffff))
XBee xbee = XBee();

void setup() {
  Serial.begin(115200);
  
  Serial1.begin(9600);
  xbee.setSerial(Serial1);
}

void loop() {
  uint8_t data[] = { 0x0, 0xffff };
  auto address = TARGET;
  auto packet = Tx64Request(address, data, sizeof(data));

  char str[128];

  Serial.println("Sent packet -");
  xbee.send(packet);
  Serial.println("Sent packet");
  Serial.flush();

  if(xbee.readPacket(5000))
  {
    Serial.println("OK?");
  } else if(xbee.getResponse().isError()) {
    snprintf(str, sizeof(str), "PAIN DA PAIN: %u", (unsigned int) xbee.getResponse().getErrorCode());
    Serial.println(str);
  } else {
    Serial.println("???");
  }
  Serial.flush();
  
  delay(1000);
}
