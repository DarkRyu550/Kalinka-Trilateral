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
  auto beacon0 = d(rssi.beacon0);
  auto beacon1 = d(rssi.beacon1);
  auto beacon2 = d(rssi.beacon2);

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
    char message[20];
    Vec2f pos = position();

    snprintf(message, sizeof(message), "Coordinates: (%f, %f)", pos.x, pos.y);
    Serial.print(message);
  }
}
