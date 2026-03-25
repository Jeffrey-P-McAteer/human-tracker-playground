
#include "utils.h"

#include "RadarSensor.h"

#define LED_PIN_NO 13
#define BUTTON_PIN_NO 2
#define RADAR_TX_PIN_NO 4

#define ON 1
#define OFF 0

#define PRESSED HIGH
#define NOT_PRESSED LOW

int ms_since_led_toggled;
int ms_to_toggle_led_at;
int led_state;

int button_state;

RadarSensor radar(RADAR_TX_PIN_NO,3); // RX, TX pins to sensor TX, RX pins

void setup() {
  ms_since_led_toggled = 0;
  ms_to_toggle_led_at = 1000;

  button_state = NOT_PRESSED;
  led_state = OFF;

  utils_setup();

  pinMode(LED_PIN_NO, OUTPUT);
  pinMode(BUTTON_PIN_NO, INPUT);

  radar.begin(256000);

  printf("Bootup complete!\n");
}

void loop() {
  button_state = digitalRead(BUTTON_PIN_NO);

  if (button_state == PRESSED) {
    ms_to_toggle_led_at = 100;
  }
  else {
    ms_to_toggle_led_at = 1000;
  }

  if (ms_since_led_toggled >= ms_to_toggle_led_at) {
    // Decide to toggle light
    if (led_state == ON) {
      //printf("Toggling pin %d LOW\n", LED_PIN_NO);
      digitalWrite(LED_PIN_NO, LOW);
      led_state = OFF;
    }
    else {
      //printf("Toggling pin %d HIGH\n", LED_PIN_NO);
      digitalWrite(LED_PIN_NO, HIGH);
      led_state = ON;
    }
    ms_since_led_toggled = 0;
  }

  delay(1);
  ms_since_led_toggled += 1;

  if (radar.update()) {
    RadarTarget tgt = radar.getTarget();
    Serial.print("X (mm): "); Serial.println(tgt.x);
    Serial.print("Y (mm): "); Serial.println(tgt.y);
    Serial.print("Distance (mm): "); Serial.println(tgt.distance);
    Serial.print("Angle (degrees): "); Serial.println(tgt.angle);
    Serial.print("Speed (cm/s): "); Serial.println(tgt.speed);
    Serial.println("-------------------------");
  }
}
