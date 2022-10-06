#include <Printers.h>
#include <XBee.h>
#include <stdio.h>

XBee xbee = XBee();

void setup() {
  Serial.begin(115200);
  Serial1.begin(9600);
  xbee.setSerial(Serial1);
}

void loop() {
  Serial.println("Vou me matar");
  Serial.flush();
  char buf[128];
  
  if(xbee.readPacket(5000))
  {
    snprintf(buf, sizeof(buf), "Got packet with renspose API ID 0x%x", xbee.getResponse().getApiId());
    Serial.println(buf);
    Serial.flush();
    
    if(xbee.getResponse().getApiId() == Rx64Response::API_ID)
    {
      Rx64Response response;
      xbee.getResponse().getRx64Response(response);

      auto address = response.getRemoteAddress64();

      snprintf(buf, sizeof(buf), "%x:%x", address.getMsb(), address.getLsb());
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
