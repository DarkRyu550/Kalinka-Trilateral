#include <Printers.h>
#include <XBee.h>
#include <stdio.h>

#define ADDRESS ("6969")

#define EWMA_F (0.5f)

#define BEACON0 (0x4200)
#define BEACON1 (0x4201)
#define BEACON2 (0x4202)

static float rssi_a[] = 
{
  -67.0f,
  -75.0f,
  -68.0f
};

static float rssi_n[] =
{
  1.0f,
  1.0f,
  1.0f
};

#define BEACON0_POS (Vec2f{.x = -1.0, .y = 1.0})
#define BEACON1_POS (Vec2f{.x = 1.0, .y = 1.0})
#define BEACON2_POS (Vec2f{.x = -1.0, .y = -1.0})

struct Vec2f
{
  float x;
  float y;
};

struct
{
  float beacon0;
  float beacon1;
  float beacon2;

  uint8_t init;
  
} rssi;

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
  
  rssi.beacon0 = 0;
  rssi.beacon1 = 0;
  rssi.beacon2 = 0;
  rssi.init = 0;
  
  xbee.setSerial(Serial1);
}

float d(float rssi, unsigned int index)
{
  float base = 1.0f;
  float exponent = (-(float)rssi - rssi_a[index]) / -(10.0f * rssi_n[index]);
  return pow(10.0f, exponent);
}

float a(const Vec2f& v0, const Vec2f& v1)
{
  return -(v1.x - v0.x) / (v1.y - v0.y);
}

float b(const Vec2f& v0, float r0, const Vec2f& v1, float r1)
{
  return (r0*r0 - r1*r1 - v0.x*v0.x + v1.x*v1.x - v0.y*v0.y + v1.y*v1.y)/(2*(v1.y-v0.y));
}

Vec2f position()
{
  auto beacon0 = d(rssi.beacon0, 0);
  auto beacon1 = d(rssi.beacon1, 1);
  auto beacon2 = d(rssi.beacon2, 2);

  /**
   * To make the trilateration, we intersect the three radii
   * (x-x0)² + (y-y0)² = r0² (I)
   * (x-x1)² + (y-y1)² = r1² (II)
   * (x-x2)² + (y-y2)² = r2² (III)
   * 
   * We can intersect the radii two by two, as long as their y isn't the same (division by 0)
   * So there are three possibilities:
   * 1) Intersect (I) with (II)  and (II) with (III)
   * 2) Intersect (I) with (II)  and (I)  with (III)
   * 3) Intersect (I) with (III) and (II) with (III)
   * Then the first intersection is either (I) with (II) or (I) with (III)
   * and the second one is either (II) with (III) or (I) with (III).
   * Also, if (I) with (II) is impossible, then (I) with (III) and (II) with (III)
   * is possible or all three centers are aligned and it's impossible to trilaterate
   * 
   * For (I) - (II) and isolating y:
   * y = -x(x1-x0)/(y1-y0) + (r0²-r1²-x0²+x1²-y0²+y1²)/(2*(y1-y0)) (IV)
   * 
   * Therefore, the points of intersection form a line y = a0*x + b0, 
   * a0 = -(x1-x0)/(y1-y0), b0 = (r0²-r1²-x0²+x1²-y0²+y1²)/2(y1-y0)
   * 
   * For (II) - (III) and isolating y:
   * y = -x(x2-x1)/(y2-y1) + (r1²-r2²-x1²+x2²-y1²+y2²)/(2(y2-y1)) (V)
   * 
   * Therefore, the points of intersection form a line y = a1*x + b1, 
   * a1 = -(x2-x1)/(y2-y1), b1 = (r1²-r2²-x1²+x2²-y1²+y2²)/2(y2-y1)
   */
  float a0, b0, a1, b1;
  if(BEACON0_POS.y != BEACON1_POS.y)
  {
    a0 = a(BEACON0_POS, BEACON1_POS);
    b0 = b(BEACON0_POS, beacon0, BEACON1_POS, beacon1);

    if(BEACON1_POS.y != BEACON2_POS.y)
    {
      // Case 1
      a1 = a(BEACON1_POS, BEACON2_POS);
      b1 = b(BEACON1_POS, beacon1, BEACON2_POS, beacon2);
    }
    else
    {
      // Case 2
      a1 = a(BEACON0_POS, BEACON2_POS);
      b1 = b(BEACON0_POS, beacon0, BEACON2_POS, beacon2);
    }
  }
  else if(BEACON0_POS.y != BEACON2_POS.y && BEACON1_POS.y != BEACON2_POS.y)
  {
    // Case 3
    a0 = a(BEACON0_POS, BEACON2_POS);
    b0 = b(BEACON0_POS, beacon0, BEACON2_POS, beacon2);
    a1 = a(BEACON1_POS, BEACON2_POS);
    b1 = b(BEACON1_POS, beacon1, BEACON2_POS, beacon2);
  }
  else
  {
    Serial.print("Error: the beacons are aligned");
    return Vec2f{.x = NAN, .y = NAN};
  }
  
  /**
   * To complete the trilateration, we intersect the two lines:
   * For (IV) - (V) and isolating x:
   * x = (b0 - b1) / (a1 - a0)
   * 
   * Then use x to solve (IV):
   * y = a0*x + b0
   */
  float x = (b0 - b1) / (a1 - a0);
  float y = a0*x + b0;

  return Vec2f{.x = x, .y = y};
}

void loop() {
  xbee_start(ADDRESS);
  if(xbee.readPacket(1000))
  {
    if(xbee.getResponse().getApiId() == Rx16Response::API_ID)
    {
      Rx16Response response;
      xbee.getResponse().getRx16Response(response);

      auto address = response.getRemoteAddress16();
      if(address == BEACON0)
      {
        Serial.print("[BEACON0] ");
        rssi.beacon0 = rssi.beacon0 * (1.0f - EWMA_F) + response.getRssi() * EWMA_F;
        rssi.init |= 1;
        Serial.print(rssi.beacon0);
        Serial.print(" / ");
        Serial.print(d(rssi.beacon0, 0));
        Serial.print(" ||| ");
      }
      else if(address == BEACON1)
      {
        Serial.print("[BEACON1] ");
        rssi.beacon1 = rssi.beacon1 * (1.0f - EWMA_F) + response.getRssi() * EWMA_F;
        rssi.init |= 2;
        Serial.print(rssi.beacon1);
        Serial.print(" / ");
        Serial.print(d(rssi.beacon1, 1));
        Serial.print(" ||| ");
      }
      else if(address == BEACON2)
      {
        Serial.print("[BEACON2] ");
        rssi.beacon2 = rssi.beacon2 * (1.0f - EWMA_F) + response.getRssi() * EWMA_F;
        rssi.init |= 4;
        Serial.print(rssi.beacon2);
        Serial.print(" / ");
        Serial.print(d(rssi.beacon2, 2));
        Serial.print(" ||| ");
      }
    }
  }

  if(rssi.init == 7)
  {
    Vec2f pos = position();

    Serial.print(pos.x);
    Serial.print(", ");
    Serial.println(pos.y);
  }
}
