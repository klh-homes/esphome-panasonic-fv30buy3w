// Panasonic FV-30BUY3W PoC — ESP32 replaces panel, sends polling + reads host response
//
// Wiring (2N7000 bidirectional level shifter):
//   ESP32 GPIO23 ──┬── 2N7000 Source
//                  [10kΩ]── ESP32 3V3
//   2N7000 Gate ──── ESP32 3V3
//   Host Data (black) ──┬── 2N7000 Drain
//                       [10kΩ]── Host 5V (red)
//   ESP32 GND ───── Host GND (white)
//   ESP32 powered via USB (do not connect Host 5V to VIN)
//
// WARNING: ESP32 GPIOs are NOT 5V tolerant (abs max 3.6V). Level shifter required.
//
// Waveform format: alternating [LOW_T, HIGH_T, LOW_T, HIGH_T, ..., LOW_T]
// 1T = 3300us. Packet starts by pulling LOW, returns to idle HIGH after.
//
// Usage: flash, open Serial Monitor at 115200

#include <Arduino.h>

// ============ Config ============
#define VERSION           "0.1.1"
#define DATA_PIN          23
#define T_US              3300
#define INTER_PACKET_GAP  130     // ms, gap between send and receive
#define RX_TIMEOUT_MS     200
#define IDLE_THRESHOLD_US 30000   // >9T = idle
#define CYCLE_MS          1460    // full comm cycle

// ============ Waveform Data ============
// Polling (panel -> host)
const int WF_POLLING[] = {
  2,1,6,2,5,2,3,1,1,1,3,2,2,2,5,2,3,1,5,2,3,1,5,2,3,1,5,2,3,1,5,2,3,1,
  5,2,3,1,5,2,3,1,5,2,3,1,5,2,3,1,5,2,3,1,5,2,3,1,5,2,3,1,1,2,7,1,1,1,1
};
const int WF_POLLING_LEN = sizeof(WF_POLLING) / sizeof(int);

// Known host responses
struct KnownResponse {
  const char* name;
  const int* waveform;
  int len;
};

const int WF_RESP_STANDBY[] = {
  2,1,6,2,5,2,3,1,2,1,2,2,2,2,5,2,3,1,3,1,1,2,2,2,5,2,3,1,1,1,1,1,1,2,
  3,1,5,2,3,1,2,2,1,2,3,1,5,2,3,1,5,2,3,1,5,2,3,1,5,2,3,1,5,2,3,1,5,2,
  3,1,1,2,7,1,1,1,2,5,1
};
const int WF_RESP_15M[] = {
  2,1,6,2,5,2,3,1,2,1,2,2,2,2,5,2,3,1,3,1,1,2,2,2,1,1,3,2,2,2,1,1,1,1,
  1,2,3,1,1,1,1,1,1,2,3,1,2,2,1,2,3,1,5,2,3,1,5,2,3,1,5,2,3,1,5,2,3,1,
  5,2,3,1,5,2,3,1,1,2,7,1,1,1,1
};
const int WF_RESP_30M[] = {
  2,1,6,2,5,2,3,1,2,1,2,2,2,2,5,2,3,1,3,1,1,2,2,2,1,2,2,2,3,1,1,1,1,1,
  1,2,3,1,5,2,3,1,2,2,1,2,3,1,5,2,3,1,5,2,3,1,5,2,3,1,5,2,3,1,5,2,3,1,
  5,2,3,1,1,2,7,1,2,1,1,5,1
};
const int WF_RESP_1H[] = {
  2,1,6,2,5,2,3,1,2,1,2,2,2,2,1,1,3,2,2,2,3,1,1,2,2,2,5,2,3,1,1,1,1,1,
  1,2,3,1,5,2,3,1,2,2,1,2,3,1,5,2,3,1,5,2,3,1,5,2,3,1,5,2,3,1,5,2,3,1,
  5,2,3,1,1,2,7,1,4
};
const int WF_RESP_3H[] = {
  2,1,6,2,5,2,3,1,2,1,2,2,2,2,1,2,2,2,3,1,3,1,1,2,2,2,5,2,3,1,1,1,1,1,
  1,2,3,1,5,2,3,1,2,2,1,2,3,1,5,2,3,1,5,2,3,1,5,2,3,1,5,2,3,1,5,2,3,1,
  5,2,3,1,1,2,7,1,2,1,1,5,1
};
const int WF_RESP_6H[] = {
  2,1,6,2,5,2,3,1,2,1,2,2,2,2,2,2,1,2,3,1,3,1,1,2,2,2,5,2,3,1,1,1,1,1,
  1,2,3,1,5,2,3,1,2,2,1,2,3,1,5,2,3,1,5,2,3,1,5,2,3,1,5,2,3,1,5,2,3,1,
  5,2,3,1,1,2,7,1,1,8,1
};
const int WF_RESP_24H[] = {
  2,1,6,2,5,2,3,1,2,1,2,2,2,2,2,2,3,1,1,2,3,1,1,2,2,2,2,2,3,1,1,2,1,1,
  1,1,1,2,3,1,2,2,3,1,1,2,2,2,1,2,3,1,5,2,3,1,5,2,3,1,5,2,3,1,5,2,3,1,
  5,2,3,1,5,2,3,1,1,2,7,1,1,4,3
};

const KnownResponse KNOWN_RESPONSES[] = {
  {"Standby", WF_RESP_STANDBY, sizeof(WF_RESP_STANDBY)/sizeof(int)},
  {"15m",     WF_RESP_15M,     sizeof(WF_RESP_15M)/sizeof(int)},
  {"30m",     WF_RESP_30M,     sizeof(WF_RESP_30M)/sizeof(int)},
  {"1h",      WF_RESP_1H,      sizeof(WF_RESP_1H)/sizeof(int)},
  {"3h",      WF_RESP_3H,      sizeof(WF_RESP_3H)/sizeof(int)},
  {"6h",      WF_RESP_6H,      sizeof(WF_RESP_6H)/sizeof(int)},
  {"24h",     WF_RESP_24H,     sizeof(WF_RESP_24H)/sizeof(int)},
};
const int NUM_KNOWN = sizeof(KNOWN_RESPONSES) / sizeof(KnownResponse);

// ============ Send Waveform ============
void send_waveform(const int* waveform, int len) {
  pinMode(DATA_PIN, OUTPUT_OPEN_DRAIN);
  for (int i = 0; i < len; i++) {
    digitalWrite(DATA_PIN, (i % 2 == 0) ? LOW : HIGH);
    delayMicroseconds(waveform[i] * T_US);
  }
  digitalWrite(DATA_PIN, HIGH);
}

// ============ Receive Waveform ============
int receive_waveform(int* buffer, int max_len) {
  pinMode(DATA_PIN, INPUT);
  unsigned long start = millis();

  while (digitalRead(DATA_PIN) == HIGH) {
    if (millis() - start > RX_TIMEOUT_MS) return 0;
  }

  int count = 0;
  bool expect_low = true;

  while (count < max_len) {
    unsigned long t0 = micros();
    int level = expect_low ? LOW : HIGH;

    while (digitalRead(DATA_PIN) == level) {
      if (micros() - t0 > IDLE_THRESHOLD_US) return count;
    }

    unsigned long dur = micros() - t0;
    int t_val = (dur + T_US / 2) / T_US;
    if (t_val < 1) t_val = 1;
    buffer[count++] = t_val;
    expect_low = !expect_low;
  }
  return count;
}

// ============ Match Response ============
// Prefix match: first 26 values, index 0 allows +/-1, rest exact.
#define MATCH_PREFIX 26
const char* match_response(const int* rx_data, int rx_len) {
  if (rx_len < MATCH_PREFIX) return NULL;
  for (int i = 0; i < NUM_KNOWN; i++) {
    const int* expected = KNOWN_RESPONSES[i].waveform;
    bool match = true;
    for (int j = 0; j < MATCH_PREFIX; j++) {
      if (j == 0) {
        int diff = rx_data[j] - expected[j];
        if (diff < -1 || diff > 1) { match = false; break; }
      } else {
        if (rx_data[j] != expected[j]) { match = false; break; }
      }
    }
    if (match) return KNOWN_RESPONSES[i].name;
  }
  return NULL;
}

// ============ Print Array ============
void print_array(const int* data, int len) {
  Serial.print("[");
  for (int i = 0; i < len; i++) {
    Serial.print(data[i]);
    if (i < len - 1) Serial.print(",");
  }
  Serial.println("]");
}

// ============ Main ============
int rx_buffer[128];
unsigned long last_cycle = 0;
int poll_count = 0;

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("=== Panasonic FV-30BUY3W PoC ===");
  Serial.printf("Version: %s\n", VERSION);
  Serial.printf("DATA_PIN: GPIO%d, 1T: %dus\n", DATA_PIN, T_US);
  Serial.println("Waveform format: alternating [L,H,L,H,...,L]");
  Serial.println("Polling started...\n");

  pinMode(DATA_PIN, OUTPUT_OPEN_DRAIN);
  digitalWrite(DATA_PIN, HIGH);
  delay(500);
}

void loop() {
  unsigned long now = millis();
  if (now - last_cycle < CYCLE_MS) return;
  last_cycle = now;
  poll_count++;

  // 1. Send polling
  send_waveform(WF_POLLING, WF_POLLING_LEN);

  // 2. Wait inter-packet gap
  delay(INTER_PACKET_GAP);

  // 3. Receive host response
  int rx_len = receive_waveform(rx_buffer, 128);

  // 4. Print result
  Serial.printf("#%d | ", poll_count);
  if (rx_len == 0) {
    Serial.println("No response");
  } else {
    const char* matched = match_response(rx_buffer, rx_len);
    if (matched) {
      Serial.printf("Host status: %s (%d values)\n", matched, rx_len);
    } else {
      Serial.printf("Unknown response (%d values): ", rx_len);
      print_array(rx_buffer, rx_len);
    }
  }

  // 5. Return to idle
  pinMode(DATA_PIN, OUTPUT_OPEN_DRAIN);
  digitalWrite(DATA_PIN, HIGH);
}
