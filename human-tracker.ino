
#include "utils.h"

#define LED_BUILTIN 13

void setup() {
  utils_setup();
  pinMode(LED_BUILTIN, OUTPUT);
}

void loop() {
  printf("Toggling pin %d HIGH\n", LED_BUILTIN);
  digitalWrite(LED_BUILTIN, HIGH);  // change state of the LED by setting the pin to the HIGH voltage level
  delay(1000);                      // wait for a second
  printf("Toggling pin %d LOW\n", LED_BUILTIN);
  digitalWrite(LED_BUILTIN, LOW);   // change state of the LED by setting the pin to the LOW voltage level
  delay(1000);                      // wait for a second
}
