/*  _
** |_|___ ___
** | |_ -|_ -|
** |_|___|___|
**  iss(c)2020
**
**  public site: https://forum.defence-force.org/viewtopic.php?f=9&t=1699
**
**  Updated: 2020.10.05 - fixed bit operation - @john
**           2020.10.29 - fixed more bit operation - @thekorex
*/

/* ================================================================== */
#include <SoftwareSerial.h>

#define DI          15  // PC1
#define DO           8  // PB0
#define CAS          9  // PB1
#define RAS         17  // PC3
#define WE          16  // PC2

#define XA0         18  // PC4
#define XA1          2  // PD2
#define XA2         19  // PC5
#define XA3          6  // PD6
#define XA4          5  // PD5
#define XA5          4  // PD4
#define XA6          7  // PD7
#define XA7          3  // PD3
#define XA8         14  // PC0

#define M_TYPE      10  // PB2
#define R_LED       11  // PB3
#define G_LED       12  // PB4

#define RXD          0  // PD0
#define TXD          1  // PD1

#define BUS_SIZE     9

/* ================================================================== */
volatile int bus_size;

//SoftwareSerial USB(RXD, TXD);

const unsigned int a_bus[BUS_SIZE] = {
  XA0, XA1, XA2, XA3, XA4, XA5, XA6, XA7, XA8
};

void setBus(unsigned int a) {
  int i;
  for (i = 0; i < BUS_SIZE; i++) {
    digitalWrite(a_bus[i], a & 1);
    a /= 2;
  }
}

void writeAddress(unsigned int r, unsigned int c, int v) {
  /* row */
  setBus(r);
  digitalWrite(RAS, LOW);

  /* rw */
  digitalWrite(WE, LOW);

  /* val */
  digitalWrite(DI, (v & 1)? HIGH : LOW);

  /* col */
  setBus(c);
  digitalWrite(CAS, LOW);

  digitalWrite(WE, HIGH);
  digitalWrite(CAS, HIGH);
  digitalWrite(RAS, HIGH);
}

int readAddress(unsigned int r, unsigned int c) {
  int ret = 0;

  /* row */
  setBus(r);
  digitalWrite(RAS, LOW);

  /* col */
  setBus(c);
  digitalWrite(CAS, LOW);

  /* get current value */
  ret = digitalRead(DO);

  digitalWrite(CAS, HIGH);
  digitalWrite(RAS, HIGH);

  return ret;
}

void error(int r, int c)
{
  unsigned long a = ((unsigned long)c << bus_size) + r;
  digitalWrite(R_LED, LOW);
  digitalWrite(G_LED, HIGH);
  interrupts();
  Serial.print(" FAILED $");
  Serial.println(a, HEX);
  for (int i=0; i<40; i++) {
    Serial.print("\a\a\n");
    delay(100);
    Serial.flush();
  }
  Serial.flush();
  while (1)
    ;
}

void ok(void)
{
  digitalWrite(R_LED, HIGH);
  digitalWrite(G_LED, LOW);
  interrupts();
  Serial.println("\n\r OK!");
  for (int i=0; i<10; i++) {
    Serial.print("\a\a\n");
    delay(100);
    Serial.flush();
  }
  Serial.flush();
  while (1)
    ;
}

void blink(void)
{
  digitalWrite(G_LED, LOW);
  digitalWrite(R_LED, LOW);
  delay(1000);
  digitalWrite(R_LED, HIGH);
  digitalWrite(G_LED, HIGH);
}

void green(int v) {
  digitalWrite(G_LED, v);
}

void print_perc(int r, int c) {
  unsigned long perc=100*((unsigned long) c)/(1<<bus_size);
  Serial.print("\r");
  Serial.print(perc, DEC);
  Serial.print(" %   ");
  Serial.print("Addr: 0x");
  Serial.print(((unsigned long) c)*(1<<((unsigned long) bus_size)), HEX);
  Serial.flush();
}

#define STEP 128
void print_c(int r, int c) {
  unsigned long a = ((unsigned long)c << bus_size) + r;

  if ((c % STEP) == 0) {
      Serial.println();
      Serial.print("Dir: r:");
      Serial.print(r, HEX);
      Serial.print(", c:");
      Serial.print(c, HEX);
      Serial.print(", a:");
      Serial.print(a, HEX);
      Serial.print(", Size:");
      Serial.print((1<<bus_size), HEX);
      Serial.print(" ");
      Serial.print((1<<bus_size), DEC);
      
            Serial.println();
      Serial.flush();
  }
}
void print_r(int r, int c) {
  if ((r % STEP) == 0) {
      Serial.print(r, HEX);
      Serial.print(". ");
      Serial.flush();
  }
}
void fill(int v) {
  int r, c, g = 0;
  v &= 1;
  for (c = 0; c < (1<<bus_size); c++) {
    green(g? HIGH : LOW);
    for (r = 0; r < (1<<bus_size); r++) {
      writeAddress(r, c, v);
      
      if (v != readAddress(r, c))
        error(r, c);
    }
    print_c(r, c);
    print_perc(r, c);
    g ^= 1;
  }
  blink();
}

void fillx(int v) {
  int r, c, g = 0;
  v &= 1;
  for (c = 0; c < (1<<bus_size); c++) {
    green(g? HIGH : LOW);
    for (r = 0; r < (1<<bus_size); r++) {
      writeAddress(r, c, v);
      
      if (v != readAddress(r, c))
        error(r, c);
      v ^= 1;
    }
    print_c(r, c);
    print_perc(r, c);
    g ^= 1;
  }
  blink();
}

void setup() {
  int i;

  Serial.begin(115200);
  while (!Serial)
    ; /* wait */

  Serial.println();
  Serial.print("\x1b[2J");
  Serial.print("\a");
  Serial.print("DRAM TESTER \n\r");

  for (i = 0; i < BUS_SIZE; i++)
    pinMode(a_bus[i], OUTPUT);

  pinMode(CAS, OUTPUT);
  pinMode(RAS, OUTPUT);
  pinMode(WE, OUTPUT);
  pinMode(DI, OUTPUT);

  pinMode(R_LED, OUTPUT);
  pinMode(G_LED, OUTPUT);

  pinMode(M_TYPE, INPUT);
  pinMode(DO, INPUT);

  digitalWrite(WE, HIGH);
  digitalWrite(RAS, HIGH);
  digitalWrite(CAS, HIGH);

  digitalWrite(R_LED, HIGH);
  digitalWrite(G_LED, HIGH);

  if (digitalRead(M_TYPE)) {
    /* jumper not set - 41256 */
    bus_size = BUS_SIZE;
    Serial.print("256Kx1 ");
  } else {
    /* jumper set - 4164 */
    bus_size = BUS_SIZE - 1;
    Serial.print("64Kx1 ");
  }
  Serial.println();
  Serial.flush();

  digitalWrite(R_LED, LOW);
  digitalWrite(G_LED, LOW);

  noInterrupts();
  for (i = 0; i < (1 << BUS_SIZE); i++) {
    digitalWrite(RAS, LOW);
    digitalWrite(RAS, HIGH);
  }
  digitalWrite(R_LED, HIGH);
  digitalWrite(G_LED, HIGH);
}

void loop() {
  interrupts(); 
  Serial.print("Write 0s\n"); Serial.flush(); noInterrupts(); fillx(0);
  Serial.print("\n\rDone\n"); Serial.flush();
  interrupts(); Serial.print("\r\nWrite 1s\n"); Serial.flush(); noInterrupts(); fillx(1);
    Serial.print("\n\rDone\n"); Serial.flush();
  interrupts(); Serial.print("\r\nWrite 0s\n"); Serial.flush(); noInterrupts(); fill(0);
    Serial.print("\n\rDone\n"); Serial.flush();
  interrupts(); Serial.print("\r\nWrite 1s\n\r"); Serial.flush(); noInterrupts(); fill(1);
  ok();
}
