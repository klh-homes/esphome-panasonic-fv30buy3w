#pragma once

#include "esphome/core/component.h"
#include "esphome/components/select/select.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/components/time/real_time_clock.h"
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

// --- Command: Standby ---
static const int WF_STANDBY[] = {
  2,1,6,2,1,1,3,2,2,2,1,1,3,2,2,2,5,2,3,1,1,1,3,2,2,2,5,2,3,1,2,1,2,2,
  2,2,5,2,3,1,1,2,2,2,3,1,5,2,3,1,3,1,1,2,2,2,5,2,3,1,1,1,1,1,1,2,3,1,
  5,2,3,1,2,2,1,2,3,1,1,2,7,1,1,2,1
};

// --- Commands: Ventilation ---
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

// --- Commands: Heating ---
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

// --- Commands: Hot Dry ---
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

// --- Commands: Cool Dry ---
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

// ============ Lookup Tables ============

struct CommandEntry {
  const char *display;   // "Standby", "Ventilation 15m", etc.
  const int *waveform;
  int len;
};

#define CMD_ENTRY(disp, arr) {disp, arr, sizeof(arr)/sizeof(int)}

static const CommandEntry COMMANDS[] = {
  CMD_ENTRY("Standby",          WF_STANDBY),
  CMD_ENTRY("Ventilation 15m",  WF_VENT_15M),
  CMD_ENTRY("Ventilation 30m",  WF_VENT_30M),
  CMD_ENTRY("Ventilation 1h",   WF_VENT_1H),
  CMD_ENTRY("Ventilation 3h",   WF_VENT_3H),
  CMD_ENTRY("Ventilation 6h",   WF_VENT_6H),
  CMD_ENTRY("Ventilation Cont.", WF_VENT_24H),
  CMD_ENTRY("Heating 15m",      WF_HEAT_15M),
  CMD_ENTRY("Heating 30m",      WF_HEAT_30M),
  CMD_ENTRY("Heating 1h",       WF_HEAT_1H),
  CMD_ENTRY("Heating 3h",       WF_HEAT_3H),
  CMD_ENTRY("Hot Dry 15m",      WF_DRYHOT_15M),
  CMD_ENTRY("Hot Dry 30m",      WF_DRYHOT_30M),
  CMD_ENTRY("Hot Dry 1h",       WF_DRYHOT_1H),
  CMD_ENTRY("Hot Dry 3h",       WF_DRYHOT_3H),
  CMD_ENTRY("Hot Dry 6h",       WF_DRYHOT_6H),
  CMD_ENTRY("Cool Dry 15m",     WF_DRYCOOL_15M),
  CMD_ENTRY("Cool Dry 30m",     WF_DRYCOOL_30M),
  CMD_ENTRY("Cool Dry 1h",      WF_DRYCOOL_1H),
  CMD_ENTRY("Cool Dry 3h",      WF_DRYCOOL_3H),
  CMD_ENTRY("Cool Dry 6h",      WF_DRYCOOL_6H),
  CMD_ENTRY("Cool Dry Cont.", WF_DRYCOOL_24H),
};
static const int NUM_COMMANDS = sizeof(COMMANDS) / sizeof(CommandEntry);

// ============ Forward Declaration ============
class PanasonicFV30BUY3W;

// ============ Select Entity ============

class FanModeSelect : public select::Select, public Component {
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
  void set_host_connection(binary_sensor::BinarySensor *s) { host_connection_ = s; }
  void set_timer_expires(text_sensor::TextSensor *s) { timer_expires_ = s; }
  void set_time(time::RealTimeClock *t) { time_ = t; }

  void setup() override;
  void loop() override;
  float get_setup_priority() const override { return setup_priority::DATA; }

  void on_mode_set(const std::string &value);

 protected:
  uint8_t pin_;
  FanModeSelect *mode_select_{nullptr};
  text_sensor::TextSensor *timer_expires_{nullptr};
  binary_sensor::BinarySensor *host_connection_{nullptr};
  time::RealTimeClock *time_{nullptr};

  // State
  std::string current_command_{"Standby"};
  bool command_pending_{false};
  uint32_t last_cycle_{0};
  int no_response_count_{0};
  bool last_connected_{false};
  // Countdown
  uint32_t countdown_start_ms_{0};
  uint32_t countdown_duration_s_{0};  // 0=standby, UINT32_MAX=continuous
  uint32_t last_countdown_update_{0};
  bool expiry_pending_publish_{false};
  std::string last_expiry_str_{};
  void check_timer_expiry();
  void publish_expiry();
  static uint32_t parse_duration(const std::string &command);

  // Protocol I/O
  void send_waveform(uint8_t pin, const int *waveform, int len);
  int receive_waveform(uint8_t pin, int *buffer, int max_len);
  const CommandEntry *find_command(const std::string &display);
};

}  // namespace panasonic_fv30buy3w
}  // namespace esphome
