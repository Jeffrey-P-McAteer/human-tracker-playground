
#include "utils.h"

#include "RS02.h"

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

RD03D radar;

void setup() {
  // ms_since_led_toggled = 0;
  // ms_to_toggle_led_at = 1000;

  // button_state = NOT_PRESSED;
  // led_state = OFF;

  // utils_setup();

  // // pinMode(LED_PIN_NO, OUTPUT);
  // // pinMode(BUTTON_PIN_NO, INPUT);

  // radar.begin(256000);

  // printf("Bootup complete!\n");

  Serial.begin(115200);
  while (!Serial);  // Wait for USB Serial to be ready

  radar.begin();

  Serial.println("RD-03D ready.");
  Serial.println("x(mm)\ty(mm)\tspeed(mm/s)");
}

// ── Debug counters ────────────────────────────────────────────────────────────
static uint32_t dbg_bytes_seen   = 0;
static uint32_t dbg_frames_ok    = 0;
static uint32_t dbg_no_target    = 0;
static uint32_t dbg_last_summary = 0;

void loop() {
  // button_state = digitalRead(BUTTON_PIN_NO);

  // if (button_state == PRESSED) {
  //   ms_to_toggle_led_at = 100;
  // }
  // else {
  //   ms_to_toggle_led_at = 1000;
  // }

  // if (ms_since_led_toggled >= ms_to_toggle_led_at) {
  //   // Decide to toggle light
  //   if (led_state == ON) {
  //     //printf("Toggling pin %d LOW\n", LED_PIN_NO);
  //     digitalWrite(LED_PIN_NO, LOW);
  //     led_state = OFF;
  //   }
  //   else {
  //     //printf("Toggling pin %d HIGH\n", LED_PIN_NO);
  //     digitalWrite(LED_PIN_NO, HIGH);
  //     led_state = ON;
  //   }
  //   ms_since_led_toggled = 0;
  // }

  // delay(1);
  // ms_since_led_toggled += 1;

  // if (radar.update()) {
  //   RadarTarget tgt = radar.getTarget();
  //   Serial.print("X (mm): "); Serial.println(tgt.x);
  //   Serial.print("Y (mm): "); Serial.println(tgt.y);
  //   Serial.print("Distance (mm): "); Serial.println(tgt.distance);
  //   Serial.print("Angle (degrees): "); Serial.println(tgt.angle);
  //   Serial.print("Speed (cm/s): "); Serial.println(tgt.speed);
  //   Serial.println("-------------------------");
  // }

  //delay(1);

  RD03DTarget t;

    // ── Raw byte dump ─────────────────────────────────────────────────────────
    // Peek at bytes before the library's read() drains them.
    // We do this by calling available() on the shared _rd03d_serial instance.
    // Because _rd03d_serial is a file-scope static in the header, we access
    // it via the macro _RD03D_STREAM.
    while (_RD03D_STREAM.available()) {
        uint8_t b = (uint8_t)_RD03D_STREAM.read();
        dbg_bytes_seen++;

        // Print raw hex byte
        Serial.print(F("0x"));
        if (b < 0x10) Serial.print(F("0"));
        Serial.print(b, HEX);
        Serial.print(F(" "));

        // Newline after every 12 bytes (one frame width) for readability
        if (dbg_bytes_seen % 12 == 0) Serial.println();

        // Feed the byte into the library parser. If a complete frame is
        // returned, print the decoded result.
        if (radar.debug_feed(b, t, dbg_frames_ok, dbg_no_target)) {
            Serial.println();
            Serial.print(F(">> FRAME: "));
            if (t.present) {
                Serial.print(F("x=")); Serial.print(t.x_mm);
                Serial.print(F("mm y=")); Serial.print(t.y_mm);
                Serial.print(F("mm speed=")); Serial.print(t.speed_mms);
                Serial.println(F("mm/s"));
            } else {
                Serial.println(F("(no target)"));
            }
        }
    }

    // ── Periodic summary ──────────────────────────────────────────────────────
    uint32_t now = millis();
    if (now - dbg_last_summary >= 5000) {
        dbg_last_summary = now;
        Serial.println();
        Serial.println(F("--- 5s summary ---"));
        Serial.print(F("  Bytes received : ")); Serial.println(dbg_bytes_seen);
        Serial.print(F("  Valid frames   : ")); Serial.println(dbg_frames_ok);
        Serial.print(F("  No-target frames: ")); Serial.println(dbg_no_target);

        if (dbg_bytes_seen == 0) {
            Serial.println(F("  !! No bytes received at all."));
            Serial.println(F("     Check: radar TX wired to pin 4?"));
            Serial.println(F("     Check: radar powered (5V)?"));
            Serial.println(F("     Check: RD03D_BAUD matches radar output baud."));
        } else if (dbg_frames_ok == 0) {
            Serial.println(F("  !! Bytes arriving but no valid frames parsed."));
            Serial.println(F("     Check: header bytes above — expecting AA/AD FF 03 00"));
            Serial.println(F("     Check: footer bytes — expecting 55 CC at positions 10-11"));
            Serial.println(F("     Check: radar baud rate matches RD03D_BAUD (115200)."));
        }
        Serial.println(F("------------------"));
        Serial.println();
    }
}
