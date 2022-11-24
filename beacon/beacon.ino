#include <Printers.h>
#include <XBee.h>
#include <stdio.h>

#define ADDRESS ("4202")
#define TARGET (0x6969)

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

void xbee_start(const char *addr_hex)
{
  static bool done = false;
  
  char target[64];
  snprintf(target, sizeof(target), "ATMY %s", addr_hex);
  
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
    xbee_at_cmd("ATAP 01");
    xbee_at_cmd("ATID E621");
    xbee_at_cmd(target);
    xbee_at_cmd("ATMM 00");
    xbee_at_cmd("ATRR 00");
    xbee_at_cmd("ATNT 19");
    xbee_at_cmd("ATCE 00");
    xbee_at_cmd("ATSC 1FFE");
    xbee_at_cmd("ATNT 19");
    xbee_at_cmd("ATSD 04");
    xbee_at_cmd("ATA1 00");
    xbee_at_cmd("ATA2 00");
    xbee_at_cmd("ATEE 00");
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
  xbee_start(ADDRESS);
  
  uint8_t data[] = { 0xe6, 0x21 };
  auto packet = Tx16Request(TARGET, data, sizeof(data));
  char str[64];
  
  xbee.send(packet);
  if(xbee.readPacket(5000))
  {
    Serial.println("OK!");
    delay(200);
  } else if(xbee.getResponse().isError()) {
    /* We'll just try again! */
  } else {
    Serial.println("???");
  }
}
