#ifdef TESTING
// Stub the GPIO functions
void pinMode(int pin, int mode) { }
void digitalWrite(int pin, int mode) { }
#else
#include "libBBB.h"
#endif

void pinReset() {
  pinMode(A0, INPUT);
  pinMode(A1, INPUT);
  pinMode(A2, INPUT);
  pinMode(A3, INPUT);
  pinMode(A4, INPUT);

  int ind;
  int setpin;
  /*  for (ind = 0; ind < relTotal; ind++) {
      pinMode(RELAYS[ind], OUTPUT);
    }
    for (setpin = 0; setpin < relTotal; setpin++) {
      digitalWrite(RELAYS[setpin], LOW); //Set all relays off
    }*/
  pinMode(2, OUTPUT);
  pinMode(3, OUTPUT);
  pinMode(4, OUTPUT);
  pinMode(5, OUTPUT);
  pinMode(6, OUTPUT);
  pinMode(7, OUTPUT);
  pinMode(8, OUTPUT);
  pinMode(9, OUTPUT);
  digitalWrite(2, HIGH);
  digitalWrite(3, HIGH);
  digitalWrite(4, HIGH);
  digitalWrite(5, HIGH);
  digitalWrite(6, HIGH);
  digitalWrite(7, HIGH);
  digitalWrite(8, HIGH);
  digitalWrite(9, HIGH);
}
