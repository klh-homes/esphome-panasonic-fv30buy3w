// Panasonic FV-30BUY3W PoC — ESP32 取代面板，發 polling + 讀主機回應
//
// 接線（2N7000 雙向 level shifter）：
//   ESP32 GPIO23 ──┬── 2N7000 Source
//                  [10kΩ]── ESP32 3V3
//   2N7000 Gate ──── ESP32 3V3
//   主機 Data(黑) ──┬── 2N7000 Drain
//                   [10kΩ]── 主機 5V(紅)
//   ESP32 GND ───── 主機 GND(白)
//   ESP32 用 USB 供電（不接主機 5V 到 VIN）
//
// ⚠️ ESP32 GPIO 不是 5V tolerant（最大 3.6V），必須使用 level shifter
//
// Waveform 格式：交替 [LOW_T, HIGH_T, LOW_T, HIGH_T, ..., LOW_T]
// 1T = 3300μs，封包從 LOW 開始（拉低 data line），封包結束後回到 idle HIGH
//
// 用法：燒錄後開 Serial Monitor (115200)，觀察主機回應

#include <Arduino.h>

// ============ 設定 ============
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
  {"待機", WF_RESP_STANDBY, sizeof(WF_RESP_STANDBY)/sizeof(int)},
  {"15m",  WF_RESP_15M,     sizeof(WF_RESP_15M)/sizeof(int)},
  {"30m",  WF_RESP_30M,     sizeof(WF_RESP_30M)/sizeof(int)},
  {"1h",   WF_RESP_1H,      sizeof(WF_RESP_1H)/sizeof(int)},
  {"3h",   WF_RESP_3H,      sizeof(WF_RESP_3H)/sizeof(int)},
  {"6h",   WF_RESP_6H,      sizeof(WF_RESP_6H)/sizeof(int)},
  {"24h",  WF_RESP_24H,     sizeof(WF_RESP_24H)/sizeof(int)},
};
const int NUM_KNOWN = sizeof(KNOWN_RESPONSES) / sizeof(KnownResponse);

// ============ Send Waveform ============
// Plays alternating LOW/HIGH durations (packet starts by pulling LOW)
void send_waveform(const int* waveform, int len) {
  pinMode(DATA_PIN, OUTPUT_OPEN_DRAIN);
  for (int i = 0; i < len; i++) {
    digitalWrite(DATA_PIN, (i % 2 == 0) ? LOW : HIGH);
    delayMicroseconds(waveform[i] * T_US);
  }
  digitalWrite(DATA_PIN, HIGH);  // release to idle (high-Z, pull-up maintains HIGH)
}

// ============ Receive Waveform ============
// Captures waveform[1:] (from first falling edge)
// Returns number of values captured, 0 = timeout
int receive_waveform(int* buffer, int max_len) {
  pinMode(DATA_PIN, INPUT);
  unsigned long start = millis();

  // Wait for falling edge (idle HIGH -> first LOW)
  while (digitalRead(DATA_PIN) == HIGH) {
    if (millis() - start > RX_TIMEOUT_MS) return 0;
  }

  int count = 0;
  bool expect_low = true;  // first value is LOW duration

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
// Waveform format: [LOW_T, HIGH_T, LOW_T, HIGH_T, ..., LOW_T]
// receive_waveform detects first falling edge (start of waveform[0] LOW),
// captures full waveform, and returns when trailing idle HIGH exceeds threshold.
// So rx_len = waveform_len.
const char* match_response(const int* rx_data, int rx_len) {
  for (int i = 0; i < NUM_KNOWN; i++) {
    int expected_len = KNOWN_RESPONSES[i].len;
    if (rx_len != expected_len) continue;
    const int* expected = KNOWN_RESPONSES[i].waveform;
    bool match = true;
    for (int j = 0; j < expected_len; j++) {
      if (rx_data[j] != expected[j]) { match = false; break; }
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
  Serial.printf("DATA_PIN: GPIO%d, 1T: %dμs\n", DATA_PIN, T_US);
  Serial.println("Waveform format: alternating [L,H,L,H,...,L]");
  Serial.println("開始 polling...\n");

  pinMode(DATA_PIN, OUTPUT_OPEN_DRAIN);
  digitalWrite(DATA_PIN, HIGH);  // high-Z, pull-up maintains idle HIGH
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
    Serial.println("無回應");
  } else {
    const char* matched = match_response(rx_buffer, rx_len);
    if (matched) {
      Serial.printf("主機狀態: %s (%d values)\n", matched, rx_len);
    } else {
      Serial.printf("未知回應 (%d values): ", rx_len);
      print_array(rx_buffer, rx_len);
    }
  }

  // 5. Return to idle
  pinMode(DATA_PIN, OUTPUT_OPEN_DRAIN);
  digitalWrite(DATA_PIN, HIGH);  // high-Z idle
}
