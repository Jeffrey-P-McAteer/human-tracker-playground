#pragma once

/**
 * rd03d.h — RD-03D mmWave Radar Single-Target Parser (Arduino Uno)
 *
 * TX-only (listen-only) driver. No radar RX pin required.
 * Uses SoftwareSerial to receive radar data on pin 4.
 *
 * WIRING:
 *   Radar TX  →  Arduino pin 4  (SoftwareSerial RX)
 *   Radar GND →  Arduino GND
 *   Radar VCC →  5V
 *   (Radar RX is left unconnected)
 *
 * ⚠️  BAUD RATE WARNING:
 *   The RD-03D runs at 256000 baud. SoftwareSerial on an Uno is
 *   unreliable above ~57600 baud. A logic-level voltage divider or
 *   level shifter is NOT the problem — the Uno's CPU simply cannot
 *   bit-bang 256000 baud reliably at 16MHz.
 *
 *   You have two options:
 *     A) Use the hardware Serial port (pins 0/1) at 256000 baud.
 *        Disconnect USB while the radar is wired to pins 0/1,
 *        or use a second Uno/Mega as a USB bridge.
 *     B) Lower the radar's baud rate to 115200 via a one-time config
 *        command (requires wiring the radar RX temporarily).
 *
 *   This file supports BOTH modes. See USE_HARDWARE_SERIAL below.
 *
 * Frame format (12 bytes total):
 *   [0]    0xAA (or 0xAD on some modules)  — Header
 *   [1]    0xFF
 *   [2]    0x03
 *   [3]    0x00
 *   [4..5] X coordinate  (uint16 LE, MSB = sign: 1=pos, 0=neg)
 *   [6..7] Y coordinate  (uint16 LE, MSB = sign: 1=pos, 0=neg)
 *   [8..9] Speed         (uint16 LE, MSB = sign: 1=pos, 0=neg)
 *   [10]   0x55          — Footer
 *   [11]   0xCC
 *
 * Usage (SoftwareSerial / lowered baud rate):
 *   #include "rd03d.h"
 *   RD03D radar;
 *
 *   void setup() {
 *       Serial.begin(115200);   // USB debug output
 *       radar.begin();
 *   }
 *
 *   void loop() {
 *       RD03DTarget t;
 *       if (radar.read(t)) {
 *           if (t.present) {
 *               Serial.print("x="); Serial.print(t.x_mm);
 *               Serial.print(" y="); Serial.print(t.y_mm);
 *               Serial.print(" speed="); Serial.println(t.speed_mms);
 *           } else {
 *               Serial.println("(no target)");
 *           }
 *       }
 *   }
 *
 * Usage (hardware Serial, no USB debug):
 *   #define RD03D_USE_HARDWARE_SERIAL
 *   #include "rd03d.h"
 *   ...same loop as above; Serial.print debug lines will be suppressed...
 */

#include <Arduino.h>

// ─── Config ──────────────────────────────────────────────────────────────────

// Uncomment to use hardware Serial (pins 0/1) instead of SoftwareSerial.
// You will lose USB Serial output. Useful if SoftwareSerial is too unreliable.
// #define RD03D_USE_HARDWARE_SERIAL

// Pin that the radar TX line is connected to (SoftwareSerial RX).
// Only used when RD03D_USE_HARDWARE_SERIAL is NOT defined.
#ifndef RD03D_RX_PIN
#define RD03D_RX_PIN 4
#endif

// Baud rate. If using SoftwareSerial, lower this to 115200 and reconfigure
// the radar module accordingly. If using hardware Serial, 256000 works.
#ifndef RD03D_BAUD
  #ifdef RD03D_USE_HARDWARE_SERIAL
    #define RD03D_BAUD 256000
  #else
    #define RD03D_BAUD 115200   // Safe maximum for SoftwareSerial on Uno
  #endif
#endif

// Accept 0xAD as a valid first header byte (common on some module revisions).
// Define RD03D_STRICT_HEADER to accept only 0xAA.
// #define RD03D_STRICT_HEADER

// ─── SoftwareSerial setup ────────────────────────────────────────────────────

#ifndef RD03D_USE_HARDWARE_SERIAL
  #include <SoftwareSerial.h>
  // TX pin (3) is unused — radar RX is unconnected — but SoftwareSerial
  // requires a TX argument. Pin 3 is used as a harmless dummy.
  static SoftwareSerial _rd03d_serial(RD03D_RX_PIN, 3 /* TX dummy, unused */);
  #define _RD03D_STREAM _rd03d_serial
#else
  #define _RD03D_STREAM Serial
#endif

// ─── Data type ───────────────────────────────────────────────────────────────

struct RD03DTarget {
    int16_t x_mm;       // X position in mm. Negative = left of sensor.
    int16_t y_mm;       // Y position in mm. Positive = forward.
    int16_t speed_mms;  // Speed in mm/s. Negative = moving away.
    bool    present;    // False if the radar reports no target.
};

// ─── Driver ──────────────────────────────────────────────────────────────────

class RD03D {
public:
    RD03D() : buf_len_(0) {
        memset(buf_, 0, sizeof(buf_));
    }

    /**
     * Call once in setup(). Starts the radar serial port.
     */
    void begin() {
        _RD03D_STREAM.begin(RD03D_BAUD);
    }

    /**
     * Call in loop(). Drains available bytes and parses frames.
     * Non-blocking — returns false immediately if no complete frame is ready.
     *
     * @param out  Populated on success.
     * @return true if a valid frame was parsed.
     */
    bool read(RD03DTarget& out) {
        while (_RD03D_STREAM.available()) {
            uint8_t byte = (uint8_t)_RD03D_STREAM.read();
            shift_in(byte);

            if (buf_len_ < FRAME_LEN) continue;
            if (!is_valid_header())   continue;
            if (buf_[10] != 0x55 || buf_[11] != 0xCC) continue;

            decode(out);
            buf_len_ = 0;
            memset(buf_, 0, sizeof(buf_));
            return true;
        }
        return false;
    }

    /**
     * Debug variant of read() — accepts a pre-read byte instead of pulling
     * from the stream. Call this from the sketch when you want to print raw
     * bytes yourself before feeding them to the parser.
     *
     * @param byte        The byte you already read from _RD03D_STREAM.
     * @param out         Populated if a valid frame is completed.
     * @param frames_ok   Counter incremented on each valid frame.
     * @param no_target   Counter incremented when frame is valid but empty.
     * @return true if a complete valid frame was just finished.
     */
    bool debug_feed(uint8_t byte, RD03DTarget& out,
                    uint32_t& frames_ok, uint32_t& no_target) {
        shift_in(byte);

        if (buf_len_ < FRAME_LEN)                        return false;
        if (!is_valid_header())                           return false;
        if (buf_[10] != 0x55 || buf_[11] != 0xCC)        return false;

        decode(out);
        frames_ok++;
        if (!out.present) no_target++;

        buf_len_ = 0;
        memset(buf_, 0, sizeof(buf_));

        // Print the decoded result directly — Serial is available in .ino scope
        // but not here, so we return true and let the sketch print if desired.
        return true;
    }

private:
    static constexpr uint8_t FRAME_LEN = 12;

    uint8_t buf_[FRAME_LEN];
    uint8_t buf_len_;

    void shift_in(uint8_t byte) {
        if (buf_len_ < FRAME_LEN) {
            buf_[buf_len_++] = byte;
        } else {
            memmove(buf_, buf_ + 1, FRAME_LEN - 1);
            buf_[FRAME_LEN - 1] = byte;
        }
    }

    bool is_valid_header() const {
#ifdef RD03D_STRICT_HEADER
        if (buf_[0] != 0xAA) return false;
#else
        if (buf_[0] != 0xAA && buf_[0] != 0xAD) return false;
#endif
        return buf_[1] == 0xFF && buf_[2] == 0x03 && buf_[3] == 0x00;
    }

    static int16_t decode_coord(uint8_t lo, uint8_t hi) {
        uint16_t raw = (uint16_t)lo | ((uint16_t)hi << 8);
        int16_t  val = (int16_t)(raw & 0x7FFF);
        return (raw & 0x8000) ? val : (int16_t)(-val);
    }

    void decode(RD03DTarget& out) const {
        out.x_mm      = decode_coord(buf_[4], buf_[5]);
        out.y_mm      = decode_coord(buf_[6], buf_[7]);
        out.speed_mms = decode_coord(buf_[8], buf_[9]);
        out.present   = (buf_[4] | buf_[5] | buf_[6] |
                         buf_[7] | buf_[8] | buf_[9]) != 0;
    }
};
