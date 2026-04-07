#pragma once

#include "esphome/core/component.h"
#include "esphome/components/select/select.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include <Arduino.h>

namespace esphome {
namespace panasonic_fv30buy3w {

// ============ Protocol Constants ============
static const uint32_t T_US = 3300;                // 1T = 3.3ms
static const uint32_t INTER_PACKET_GAP_MS = 130;  // gap between packets
static const uint32_t RX_TIMEOUT_MS = 200;         // host response timeout
static const uint32_t IDLE_THRESHOLD_US = 30000;   // >9T = idle
static const int MAX_WAVEFORM_LEN = 128;           // max waveform array size

// ============ Waveform Packet Data ============
// Format: alternating [LOW_T, HIGH_T, LOW_T, HIGH_T, ..., LOW_T]
// Odd number of elements, starts and ends with LOW duration in T units.
// Playback (open-drain): LOW=sink N*3300us -> HIGH=release(pull-up) N*3300us -> ...

// --- Polling (panel -> host) ---
static const int WF_POLLING[] = {
  2,1,6,2,5,2,3,1,1,1,3,2,2,2,5,2,3,1,5,2,3,1,5,2,3,1,5,2,3,1,5,2,3,1,
  5,2,3,1,5,2,3,1,5,2,3,1,5,2,3,1,5,2,3,1,5,2,3,1,5,2,3,1,1,2,7,1,1,1,1
};

// --- Command: 待機 ---
static const int WF_STANDBY[] = {
  2,1,6,2,1,1,3,2,2,2,1,1,3,2,2,2,5,2,3,1,1,1,3,2,2,2,5,2,3,1,2,1,2,2,
  2,2,5,2,3,1,1,2,2,2,3,1,5,2,3,1,3,1,1,2,2,2,5,2,3,1,1,1,1,1,1,2,3,1,
  5,2,3,1,2,2,1,2,3,1,1,2,7,1,1,2,1
};

// --- Commands: 換氣 ---
static const int WF_VENT_15M[] = {
  2,1,6,2,1,1,3,2,2,2,1,1,3,2,2,2,1,1,3,2,2,2,1,1,3,2,2,2,1,1,3,2,2,2,
  2,1,2,2,2,2,5,2,3,1,1,2,2,2,3,1,5,2,3,1,3,1,1,2,2,2,1,1,3,2,2,2,1,1,
  1,1,1,2,3,1,1,1,1,1,1,2,3,1,2,2,1,2,3,1,1,2,7,1,1,8,1
};
static const int WF_VENT_30M[] = {
  2,1,6,2,1,1,3,2,2,2,1,1,3,2,2,2,1,1,3,2,2,2,1,1,3,2,2,2,1,1,3,2,2,2,
  2,1,2,2,2,2,5,2,3,1,1,2,2,2,3,1,5,2,3,1,3,1,1,2,2,2,1,2,2,2,3,1,1,1,
  1,1,1,2,3,1,5,2,3,1,2,2,1,2,3,1,1,2,7,1,4
};
static const int WF_VENT_1H[] = {
  2,1,6,2,1,1,3,2,2,2,1,1,3,2,2,2,1,1,3,2,2,2,1,1,3,2,2,2,1,1,3,2,2,2,
  2,1,2,2,2,2,5,2,3,1,1,2,2,2,3,1,1,1,3,2,2,2,3,1,1,2,2,2,5,2,3,1,1,1,
  1,1,1,2,3,1,5,2,3,1,2,2,1,2,3,1,1,2,7,1,2,1,1,5,1
};
static const int WF_VENT_3H[] = {
  2,1,6,2,1,1,3,2,2,2,1,1,3,2,2,2,1,1,3,2,2,2,1,1,3,2,2,2,1,1,3,2,2,2,
  2,1,2,2,2,2,5,2,3,1,1,2,2,2,3,1,1,2,2,2,3,1,3,1,1,2,2,2,5,2,3,1,1,1,
  1,1,1,2,3,1,5,2,3,1,2,2,1,2,3,1,1,2,7,1,4
};
static const int WF_VENT_6H[] = {
  2,1,6,2,1,1,3,2,2,2,1,1,3,2,2,2,1,1,3,2,2,2,1,1,3,2,2,2,1,1,3,2,2,2,
  2,1,2,2,2,2,5,2,3,1,1,2,2,2,3,1,2,2,1,2,3,1,3,1,1,2,2,2,5,2,3,1,1,1,
  1,1,1,2,3,1,5,2,3,1,2,2,1,2,3,1,1,2,7,1,1,1,1
};
static const int WF_VENT_24H[] = {
  2,1,6,2,1,1,3,2,2,2,1,1,3,2,2,2,1,1,3,2,2,2,1,1,3,2,2,2,1,1,3,2,2,2,
  2,1,2,2,2,2,5,2,3,1,1,2,2,2,3,1,2,2,3,1,1,2,3,1,1,2,2,2,2,2,3,1,1,2,
  1,1,1,1,1,2,3,1,2,2,3,1,1,2,2,2,1,2,3,1,1,2,7,1,1,1,1,2,3,1,1
};

// --- Commands: 取暖 ---
static const int WF_HEAT_15M[] = {
  2,1,6,2,1,1,3,2,2,2,1,1,3,2,2,2,2,1,2,2,2,2,1,1,3,2,2,2,1,1,1,1,1,2,
  3,1,2,1,2,2,2,2,5,2,3,1,1,2,2,2,3,1,5,2,3,1,3,1,1,2,2,2,1,1,3,2,2,2,
  1,1,1,1,1,2,3,1,1,1,1,1,1,2,3,1,2,2,1,2,3,1,1,2,7,1,4
};
static const int WF_HEAT_30M[] = {
  2,1,6,2,1,1,3,2,2,2,1,1,3,2,2,2,2,1,2,2,2,2,1,1,3,2,2,2,1,1,1,1,1,2,
  3,1,2,1,2,2,2,2,5,2,3,1,1,2,2,2,3,1,5,2,3,1,3,1,1,2,2,2,1,2,2,2,3,1,
  1,1,1,1,1,2,3,1,5,2,3,1,2,2,1,2,3,1,1,2,7,1,1,8,1
};
static const int WF_HEAT_1H[] = {
  2,1,6,2,1,1,3,2,2,2,1,1,3,2,2,2,2,1,2,2,2,2,1,1,3,2,2,2,1,1,1,1,1,2,
  3,1,2,1,2,2,2,2,5,2,3,1,1,2,2,2,3,1,1,1,3,2,2,2,3,1,1,2,2,2,5,2,3,1,
  1,1,1,1,1,2,3,1,5,2,3,1,2,2,1,2,3,1,1,2,7,1,1,1,1
};
static const int WF_HEAT_3H[] = {
  2,1,6,2,1,1,3,2,2,2,1,1,3,2,2,2,2,1,2,2,2,2,1,1,3,2,2,2,1,1,1,1,1,2,
  3,1,2,1,2,2,2,2,5,2,3,1,1,2,2,2,3,1,1,2,2,2,3,1,3,1,1,2,2,2,5,2,3,1,
  1,1,1,1,1,2,3,1,5,2,3,1,2,2,1,2,3,1,1,2,7,1,1,8,1
};

// --- Commands: 乾燥熱 ---
static const int WF_DRYHOT_15M[] = {
  2,1,6,2,1,1,3,2,2,2,1,1,3,2,2,2,1,2,2,2,3,1,1,1,3,2,2,2,1,1,1,1,1,2,
  3,1,2,1,2,2,2,2,5,2,3,1,1,2,2,2,3,1,5,2,3,1,3,1,1,2,2,2,1,1,3,2,2,2,
  1,1,1,1,1,2,3,1,1,1,1,1,1,2,3,1,2,2,1,2,3,1,1,2,7,1,1,1,2,5,1
};
static const int WF_DRYHOT_30M[] = {
  2,1,6,2,1,1,3,2,2,2,1,1,3,2,2,2,1,2,2,2,3,1,1,1,3,2,2,2,1,1,1,1,1,2,
  3,1,2,1,2,2,2,2,5,2,3,1,1,2,2,2,3,1,5,2,3,1,3,1,1,2,2,2,1,2,2,2,3,1,
  1,1,1,1,1,2,3,1,5,2,3,1,2,2,1,2,3,1,1,2,7,1,2
};
static const int WF_DRYHOT_1H[] = {
  2,1,6,2,1,1,3,2,2,2,1,1,3,2,2,2,1,2,2,2,3,1,1,1,3,2,2,2,1,1,1,1,1,2,
  3,1,2,1,2,2,2,2,5,2,3,1,1,2,2,2,3,1,1,1,3,2,2,2,3,1,1,2,2,2,5,2,3,1,
  1,1,1,1,1,2,3,1,5,2,3,1,2,2,1,2,3,1,1,2,7,1,3,6,1
};
static const int WF_DRYHOT_3H[] = {
  2,1,6,2,1,1,3,2,2,2,1,1,3,2,2,2,1,2,2,2,3,1,1,1,3,2,2,2,1,1,1,1,1,2,
  3,1,2,1,2,2,2,2,5,2,3,1,1,2,2,2,3,1,1,2,2,2,3,1,3,1,1,2,2,2,5,2,3,1,
  1,1,1,1,1,2,3,1,5,2,3,1,2,2,1,2,3,1,1,2,7,1,2
};
static const int WF_DRYHOT_6H[] = {
  2,1,6,2,1,1,3,2,2,2,1,1,3,2,2,2,1,2,2,2,3,1,1,1,3,2,2,2,1,1,1,1,1,2,
  3,1,2,1,2,2,2,2,5,2,3,1,1,2,2,2,3,1,2,2,1,2,3,1,3,1,1,2,2,2,5,2,3,1,
  1,1,1,1,1,2,3,1,5,2,3,1,2,2,1,2,3,1,1,2,7,1,1,2,1
};

// --- Commands: 乾燥涼 ---
static const int WF_DRYCOOL_15M[] = {
  2,1,6,2,1,1,3,2,2,2,1,1,3,2,2,2,3,1,1,2,2,2,1,1,3,2,2,2,1,1,1,1,1,2,
  3,1,2,1,2,2,2,2,5,2,3,1,1,2,2,2,3,1,5,2,3,1,3,1,1,2,2,2,1,1,3,2,2,2,
  1,1,1,1,1,2,3,1,1,1,1,1,1,2,3,1,2,2,1,2,3,1,1,2,7,1,2
};
static const int WF_DRYCOOL_30M[] = {
  2,1,6,2,1,1,3,2,2,2,1,1,3,2,2,2,3,1,1,2,2,2,1,1,3,2,2,2,1,1,1,1,1,2,
  3,1,2,1,2,2,2,2,5,2,3,1,1,2,2,2,3,1,5,2,3,1,3,1,1,2,2,2,1,2,2,2,3,1,
  1,1,1,1,1,2,3,1,5,2,3,1,2,2,1,2,3,1,1,2,7,1,1,1,2,5,1
};
static const int WF_DRYCOOL_1H[] = {
  2,1,6,2,1,1,3,2,2,2,1,1,3,2,2,2,3,1,1,2,2,2,1,1,3,2,2,2,1,1,1,1,1,2,
  3,1,2,1,2,2,2,2,5,2,3,1,1,2,2,2,3,1,1,1,3,2,2,2,3,1,1,2,2,2,5,2,3,1,
  1,1,1,1,1,2,3,1,5,2,3,1,2,2,1,2,3,1,1,2,7,1,1,2,1
};
static const int WF_DRYCOOL_3H[] = {
  2,1,6,2,1,1,3,2,2,2,1,1,3,2,2,2,3,1,1,2,2,2,1,1,3,2,2,2,1,1,1,1,1,2,
  3,1,2,1,2,2,2,2,5,2,3,1,1,2,2,2,3,1,1,2,2,2,3,1,3,1,1,2,2,2,5,2,3,1,
  1,1,1,1,1,2,3,1,5,2,3,1,2,2,1,2,3,1,1,2,7,1,1,1,2,5,1
};
static const int WF_DRYCOOL_6H[] = {
  2,1,6,2,1,1,3,2,2,2,1,1,3,2,2,2,3,1,1,2,2,2,1,1,3,2,2,2,1,1,1,1,1,2,
  3,1,2,1,2,2,2,2,5,2,3,1,1,2,2,2,3,1,2,2,1,2,3,1,3,1,1,2,2,2,5,2,3,1,
  1,1,1,1,1,2,3,1,5,2,3,1,2,2,1,2,3,1,1,2,7,1,3,6,1
};
static const int WF_DRYCOOL_24H[] = {
  2,1,6,2,1,1,3,2,2,2,1,1,3,2,2,2,3,1,1,2,2,2,1,1,3,2,2,2,1,1,1,1,1,2,
  3,1,2,1,2,2,2,2,5,2,3,1,1,2,2,2,3,1,2,2,3,1,1,2,3,1,1,2,2,2,2,2,3,1,
  1,2,1,1,1,1,1,2,3,1,2,2,3,1,1,2,2,2,1,2,3,1,1,2,7,1,3,2,3
};

// --- Host Responses ---
static const int WF_RESP_STANDBY[] = {
  2,1,6,2,5,2,3,1,2,1,2,2,2,2,5,2,3,1,3,1,1,2,2,2,5,2,3,1,1,1,1,1,1,2,
  3,1,5,2,3,1,2,2,1,2,3,1,5,2,3,1,5,2,3,1,5,2,3,1,5,2,3,1,5,2,3,1,5,2,
  3,1,1,2,7,1,1,1,2,5,1
};
static const int WF_RESP_15M[] = {
  2,1,6,2,5,2,3,1,2,1,2,2,2,2,5,2,3,1,3,1,1,2,2,2,1,1,3,2,2,2,1,1,1,1,
  1,2,3,1,1,1,1,1,1,2,3,1,2,2,1,2,3,1,5,2,3,1,5,2,3,1,5,2,3,1,5,2,3,1,
  5,2,3,1,5,2,3,1,1,2,7,1,1,1,1
};
static const int WF_RESP_30M[] = {
  2,1,6,2,5,2,3,1,2,1,2,2,2,2,5,2,3,1,3,1,1,2,2,2,1,2,2,2,3,1,1,1,1,1,
  1,2,3,1,5,2,3,1,2,2,1,2,3,1,5,2,3,1,5,2,3,1,5,2,3,1,5,2,3,1,5,2,3,1,
  5,2,3,1,1,2,7,1,2,1,1,5,1
};
static const int WF_RESP_1H[] = {
  2,1,6,2,5,2,3,1,2,1,2,2,2,2,1,1,3,2,2,2,3,1,1,2,2,2,5,2,3,1,1,1,1,1,
  1,2,3,1,5,2,3,1,2,2,1,2,3,1,5,2,3,1,5,2,3,1,5,2,3,1,5,2,3,1,5,2,3,1,
  5,2,3,1,1,2,7,1,4
};
static const int WF_RESP_3H[] = {
  2,1,6,2,5,2,3,1,2,1,2,2,2,2,1,2,2,2,3,1,3,1,1,2,2,2,5,2,3,1,1,1,1,1,
  1,2,3,1,5,2,3,1,2,2,1,2,3,1,5,2,3,1,5,2,3,1,5,2,3,1,5,2,3,1,5,2,3,1,
  5,2,3,1,1,2,7,1,2,1,1,5,1
};
static const int WF_RESP_6H[] = {
  2,1,6,2,5,2,3,1,2,1,2,2,2,2,2,2,1,2,3,1,3,1,1,2,2,2,5,2,3,1,1,1,1,1,
  1,2,3,1,5,2,3,1,2,2,1,2,3,1,5,2,3,1,5,2,3,1,5,2,3,1,5,2,3,1,5,2,3,1,
  5,2,3,1,1,2,7,1,1,8,1
};
static const int WF_RESP_24H[] = {
  2,1,6,2,5,2,3,1,2,1,2,2,2,2,2,2,3,1,1,2,3,1,1,2,2,2,2,2,3,1,1,2,1,1,
  1,1,1,2,3,1,2,2,3,1,1,2,2,2,1,2,3,1,5,2,3,1,5,2,3,1,5,2,3,1,5,2,3,1,
  5,2,3,1,5,2,3,1,1,2,7,1,1,4,3
};

// ============ Lookup Tables ============

struct CommandEntry {
  const char *mode;
  const char *timer_key;
  const int *waveform;
  int len;
};

#define CMD_ENTRY(mode, timer, arr) {mode, timer, arr, sizeof(arr)/sizeof(int)}

static const CommandEntry COMMANDS[] = {
  CMD_ENTRY("待機",   "",    WF_STANDBY),
  CMD_ENTRY("換氣",   "15m", WF_VENT_15M),
  CMD_ENTRY("換氣",   "30m", WF_VENT_30M),
  CMD_ENTRY("換氣",   "1h",  WF_VENT_1H),
  CMD_ENTRY("換氣",   "3h",  WF_VENT_3H),
  CMD_ENTRY("換氣",   "6h",  WF_VENT_6H),
  CMD_ENTRY("換氣",   "24h", WF_VENT_24H),
  CMD_ENTRY("取暖",   "15m", WF_HEAT_15M),
  CMD_ENTRY("取暖",   "30m", WF_HEAT_30M),
  CMD_ENTRY("取暖",   "1h",  WF_HEAT_1H),
  CMD_ENTRY("取暖",   "3h",  WF_HEAT_3H),
  CMD_ENTRY("乾燥熱", "15m", WF_DRYHOT_15M),
  CMD_ENTRY("乾燥熱", "30m", WF_DRYHOT_30M),
  CMD_ENTRY("乾燥熱", "1h",  WF_DRYHOT_1H),
  CMD_ENTRY("乾燥熱", "3h",  WF_DRYHOT_3H),
  CMD_ENTRY("乾燥熱", "6h",  WF_DRYHOT_6H),
  CMD_ENTRY("乾燥涼", "15m", WF_DRYCOOL_15M),
  CMD_ENTRY("乾燥涼", "30m", WF_DRYCOOL_30M),
  CMD_ENTRY("乾燥涼", "1h",  WF_DRYCOOL_1H),
  CMD_ENTRY("乾燥涼", "3h",  WF_DRYCOOL_3H),
  CMD_ENTRY("乾燥涼", "6h",  WF_DRYCOOL_6H),
  CMD_ENTRY("乾燥涼", "24h", WF_DRYCOOL_24H),
};
static const int NUM_COMMANDS = sizeof(COMMANDS) / sizeof(CommandEntry);

struct ResponseEntry {
  const char *timer_key;
  const char *display;
  const int *waveform;
  int len;
};

#define RESP_ENTRY(key, disp, arr) {key, disp, arr, sizeof(arr)/sizeof(int)}

static const ResponseEntry RESPONSES[] = {
  RESP_ENTRY("standby", "待機",   WF_RESP_STANDBY),
  RESP_ENTRY("15m",     "15分",   WF_RESP_15M),
  RESP_ENTRY("30m",     "30分",   WF_RESP_30M),
  RESP_ENTRY("1h",      "1小時",  WF_RESP_1H),
  RESP_ENTRY("3h",      "3小時",  WF_RESP_3H),
  RESP_ENTRY("6h",      "6小時",  WF_RESP_6H),
  RESP_ENTRY("24h",     "24小時", WF_RESP_24H),
};
static const int NUM_RESPONSES = sizeof(RESPONSES) / sizeof(ResponseEntry);

// Timer display <-> key mapping
struct TimerMap {
  const char *display;
  const char *key;
};

static const TimerMap TIMER_NAMES[] = {
  {"15分",   "15m"},
  {"30分",   "30m"},
  {"1小時",  "1h"},
  {"3小時",  "3h"},
  {"6小時",  "6h"},
  {"24小時", "24h"},
};
static const int NUM_TIMERS = sizeof(TIMER_NAMES) / sizeof(TimerMap);

// Mode -> valid timer keys (NULL-terminated)
struct ModeTimerDef {
  const char *mode;
  const char *valid_timers[7];
};

static const ModeTimerDef MODE_TIMERS[] = {
  {"換氣",   {"15m", "30m", "1h", "3h", "6h", "24h", nullptr}},
  {"取暖",   {"15m", "30m", "1h", "3h", nullptr}},
  {"乾燥熱", {"15m", "30m", "1h", "3h", "6h", nullptr}},
  {"乾燥涼", {"15m", "30m", "1h", "3h", "6h", "24h", nullptr}},
};
static const int NUM_MODE_TIMERS = sizeof(MODE_TIMERS) / sizeof(ModeTimerDef);

// ============ Forward Declaration ============
class PanasonicFV30BUY3W;

// ============ Select Entities ============

class FanModeSelect : public select::Select, public Component {
 public:
  void set_parent(PanasonicFV30BUY3W *p) { parent_ = p; }
 protected:
  void control(const std::string &value) override;
  PanasonicFV30BUY3W *parent_{nullptr};
};

class FanTimerSelect : public select::Select, public Component {
 public:
  void set_parent(PanasonicFV30BUY3W *p) { parent_ = p; }
 protected:
  void control(const std::string &value) override;
  PanasonicFV30BUY3W *parent_{nullptr};
};

// ============ Main Component ============

class PanasonicFV30BUY3W : public Component {
 public:
  void set_pin(uint8_t pin) { pin_ = pin; }
  void set_mode_select(FanModeSelect *s) { mode_select_ = s; }
  void set_timer_select(FanTimerSelect *s) { timer_select_ = s; }
  void set_host_timer(text_sensor::TextSensor *s) { host_timer_ = s; }
  void set_host_connection(text_sensor::TextSensor *s) { host_connection_ = s; }

  void setup() override;
  void loop() override;
  float get_setup_priority() const override { return setup_priority::DATA; }

  void on_mode_set(const std::string &mode);
  void on_timer_set(const std::string &timer_display);

 protected:
  uint8_t pin_;
  FanModeSelect *mode_select_{nullptr};
  FanTimerSelect *timer_select_{nullptr};
  text_sensor::TextSensor *host_timer_{nullptr};
  text_sensor::TextSensor *host_connection_{nullptr};

  // State
  std::string current_mode_{"待機"};
  std::string current_timer_key_{};
  bool command_pending_{false};
  int command_retry_count_{0};
  int command_wait_cycles_{0};  // >0: wait N polling cycles before checking host response
  uint32_t last_cycle_{0};
  int no_response_count_{0};
  std::string last_host_timer_{};

  // Protocol I/O — pin parameter for future proxy mode
  void send_waveform(uint8_t pin, const int *waveform, int len);
  int receive_waveform(uint8_t pin, int *buffer, int max_len);

  // Match received full waveform against known response waveform
  std::string match_response(const int *rx_data, int rx_len);

  // Lookup
  const CommandEntry *find_command(const std::string &mode, const std::string &timer_key);

  // Timer helpers
  static const char *timer_display_to_key(const std::string &display);
  static const char *timer_key_to_display(const std::string &key);
  static bool is_valid_timer(const std::string &mode, const std::string &timer_key);
  static const char *default_timer_key(const std::string &mode);
  void update_timer_options(const std::string &mode);
};

}  // namespace panasonic_fv30buy3w
}  // namespace esphome
