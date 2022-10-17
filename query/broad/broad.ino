#include <Printers.h>
#include <XBee.h>
#include <stdio.h>

#define TARGET (XBeeAddress64(0x0, 0x0))
XBee xbee = XBee();

void xbee_at_cmd(const String& line)
{
  Serial1.print(line);
  Serial1.write('\r');
  Serial1.flush();
    
  while(!Serial1.available());
  auto resp = Serial1.readStringUntil('\r');

  Serial.print(line);
  Serial.print(" => ");
  Serial.println(resp);
}

void xbee_start()
{
  static bool done = false;
  if(!done)
  {
    delay(2000);
    Serial.println("-- DISCARD");
    Serial.println("-- DISCARD");
    Serial.println("-- DISCARD");
    Serial.println("Running XBee initialization procedure.");
    
    delay(1200);
    Serial1.write("+++");
    Serial1.flush();
    delay(1200);
    while(!Serial1.available());
    auto resp = Serial1.readStringUntil('\r');
    
    Serial.print("+++ => ");
    Serial.println(resp);

    xbee_at_cmd("ATCH 0B");
    xbee_at_cmd("ATAP 02");
    xbee_at_cmd("ATCN");

    done = true;
  }
}

void setup() {
  Serial.begin(115200);
  
  Serial1.begin(9600);
  xbee.setSerial(Serial1);
}

void loop() {
  xbee_start();
  uint8_t data[] = { 0xe6, 0x21 };
  auto address = TARGET;
  auto packet = Tx16Request(BROADCAST_ADDRESS, data, sizeof(data));

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
  
  delay(100);
}
