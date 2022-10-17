#include <Printers.h>
#include <XBee.h>
#include <stdio.h>

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
  Serial.println("Vou me matar");
  Serial.flush();
  char buf[128];
  
  if(xbee.readPacket(5000))
  {
    snprintf(buf, sizeof(buf), "Got packet with renspose API ID 0x%x", xbee.getResponse().getApiId());
    Serial.println(buf);
    Serial.flush();
    
    if(xbee.getResponse().getApiId() == RX_64_RESPONSE)
    {
      Rx64Response response;
      xbee.getResponse().getRx64Response(response);

      auto address = response.getRemoteAddress64();
      auto d0 = response.getData(0);
      auto d1 = response.getData(1);

      snprintf(buf, sizeof(buf), "{0x%02x, 0x%02x} => %x:%x", address.getMsb(), address.getLsb());
      Serial.println(buf);
      Serial.flush();
    } 
    else if(xbee.getResponse().getApiId() == RX_16_RESPONSE)
    {
      Rx16Response response;
      xbee.getResponse().getRx16Response(response);

      auto address = response.getRemoteAddress16();
      auto d0 = response.getData(0);
      auto d1 = response.getData(1);

      snprintf(buf, sizeof(buf), "{0x%02x, 0x%02x} => %x", address);
      Serial.println(buf);
      Serial.flush();
    }
  } else if(xbee.getResponse().isError()) {
    Serial.println("The plot thickens!");
    Serial.flush();
  } else {
    Serial.println("Timed out");
    Serial.flush();
  }
}
