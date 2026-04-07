#include "panasonic_fv30buy3w.h"
#include "esphome/core/log.h"

namespace esphome {
namespace panasonic_fv30buy3w {

static const char *TAG = "panasonic_fv30buy3w";

// ============ Select Controls ============

void FanModeSelect::control(const std::string &value) {
  this->publish_state(value);
  if (parent_) parent_->on_mode_set(value);
}

void FanTimerSelect::control(const std::string &value) {
  this->publish_state(value);
  if (parent_) parent_->on_timer_set(value);
}

// ============ Timer Helpers ============

const char *PanasonicFV30BUY3W::timer_display_to_key(const std::string &display) {
  for (int i = 0; i < NUM_TIMERS; i++) {
    if (display == TIMER_NAMES[i].display) return TIMER_NAMES[i].key;
  }
  return nullptr;
}

const char *PanasonicFV30BUY3W::timer_key_to_display(const std::string &key) {
  for (int i = 0; i < NUM_TIMERS; i++) {
    if (key == TIMER_NAMES[i].key) return TIMER_NAMES[i].display;
  }
  return nullptr;
}

bool PanasonicFV30BUY3W::is_valid_timer(const std::string &mode, const std::string &timer_key) {
  for (int i = 0; i < NUM_MODE_TIMERS; i++) {
    if (mode == MODE_TIMERS[i].mode) {
      for (int j = 0; MODE_TIMERS[i].valid_timers[j] != nullptr; j++) {
        if (timer_key == MODE_TIMERS[i].valid_timers[j]) return true;
      }
      return false;
    }
  }
  return false;
}

const char *PanasonicFV30BUY3W::default_timer_key(const std::string &mode) {
  for (int i = 0; i < NUM_MODE_TIMERS; i++) {
    if (mode == MODE_TIMERS[i].mode) {
      return MODE_TIMERS[i].valid_timers[0];
    }
  }
  return nullptr;
}

void PanasonicFV30BUY3W::update_timer_options(const std::string &mode) {
  if (!timer_select_) return;
  if (mode == "換氣" || mode == "乾燥涼") {
    timer_select_->traits.set_options({"15分", "30分", "1小時", "3小時", "6小時", "24小時"});
  } else if (mode == "乾燥熱") {
    timer_select_->traits.set_options({"15分", "30分", "1小時", "3小時", "6小時"});
  } else if (mode == "取暖") {
    timer_select_->traits.set_options({"15分", "30分", "1小時", "3小時"});
  }
}

// ============ Packet Lookup ============

const CommandEntry *PanasonicFV30BUY3W::find_command(const std::string &mode, const std::string &timer_key) {
  for (int i = 0; i < NUM_COMMANDS; i++) {
    if (mode == COMMANDS[i].mode && timer_key == COMMANDS[i].timer_key) {
      return &COMMANDS[i];
    }
  }
  return nullptr;
}

std::string PanasonicFV30BUY3W::match_response(const int *rx_data, int rx_len) {
  // Host responses: first 26 values uniquely identify the timer setting.
  // Values after index 26 change during countdown — ignore them.
  // Only index 0 gets ±1 tolerance (delay() jitter). Rest must be exact
  // to avoid confusing adjacent timer states (e.g., 15m vs 30m differ by 1).
  static const int MATCH_PREFIX = 26;
  if (rx_len < MATCH_PREFIX) return "";

  for (int i = 0; i < NUM_RESPONSES; i++) {
    const int *expected = RESPONSES[i].waveform;
    bool match = true;
    for (int j = 0; j < MATCH_PREFIX; j++) {
      if (j == 0) {
        int diff = rx_data[j] - expected[j];
        if (diff < -1 || diff > 1) { match = false; break; }
      } else {
        if (rx_data[j] != expected[j]) { match = false; break; }
      }
    }
    if (match) return RESPONSES[i].display;
  }
  return "";
}

// ============ Protocol I/O ============

void PanasonicFV30BUY3W::send_waveform(uint8_t pin, const int *waveform, int len) {
  pinMode(pin, OUTPUT_OPEN_DRAIN);

  for (int i = 0; i < len; i++) {
    digitalWrite(pin, (i % 2 == 0) ? LOW : HIGH);
    delayMicroseconds(waveform[i] * T_US);
  }

  // Release to idle (high-Z, pull-up maintains HIGH)
  digitalWrite(pin, HIGH);
}

int PanasonicFV30BUY3W::receive_waveform(uint8_t pin, int *buffer, int max_len) {
  pinMode(pin, INPUT);
  unsigned long start = millis();

  // Wait for falling edge (idle HIGH -> first LOW = waveform[0])
  // Keep interrupts enabled during wait (can take up to 200ms)
  while (digitalRead(pin) == HIGH) {
    if (millis() - start > RX_TIMEOUT_MS) return 0;
  }

  // Capture alternating LOW/HIGH durations starting from waveform[0]
  int count = 0;
  bool expect_low = true;  // first captured value is LOW duration

  while (count < max_len) {
    unsigned long t0 = micros();
    int current_level = expect_low ? LOW : HIGH;

    while (digitalRead(pin) == current_level) {
      if (micros() - t0 > IDLE_THRESHOLD_US) {
        return count;
      }
    }

    unsigned long dur = micros() - t0;

    int t_val = (dur + T_US / 2) / T_US;
    if (t_val < 1) t_val = 1;
    buffer[count++] = t_val;
    expect_low = !expect_low;
  }
  return count;
}

// ============ Mode / Timer Control ============

void PanasonicFV30BUY3W::on_mode_set(const std::string &mode) {
  ESP_LOGI(TAG, "Mode set: %s", mode.c_str());
  current_mode_ = mode;

  if (mode == "待機") {
    current_timer_key_ = "";
    command_pending_ = true;
    command_retry_count_ = 0;
    return;
  }

  update_timer_options(mode);

  if (current_timer_key_.empty() || !is_valid_timer(mode, current_timer_key_)) {
    const char *def = default_timer_key(mode);
    if (def) {
      current_timer_key_ = def;
      const char *disp = timer_key_to_display(current_timer_key_);
      if (disp && timer_select_) timer_select_->publish_state(disp);
    }
  }

  command_pending_ = true;
  command_retry_count_ = 0;
}

void PanasonicFV30BUY3W::on_timer_set(const std::string &timer_display) {
  const char *key = timer_display_to_key(timer_display);
  if (!key) {
    ESP_LOGW(TAG, "Unknown timer: %s", timer_display.c_str());
    return;
  }
  if (current_mode_ == "待機") {
    ESP_LOGW(TAG, "Cannot set timer in standby mode");
    return;
  }
  if (!is_valid_timer(current_mode_, key)) {
    ESP_LOGW(TAG, "Timer %s not valid for mode %s", timer_display.c_str(), current_mode_.c_str());
    return;
  }

  ESP_LOGI(TAG, "Timer set: %s", timer_display.c_str());
  current_timer_key_ = key;
  command_pending_ = true;
  command_retry_count_ = 0;
}

// ============ Component Lifecycle ============

void PanasonicFV30BUY3W::setup() {
  ESP_LOGI(TAG, "Setting up on GPIO%d", pin_);
  pinMode(pin_, OUTPUT_OPEN_DRAIN);
  digitalWrite(pin_, HIGH);  // high-Z idle

  if (mode_select_) mode_select_->publish_state("待機");
  if (host_timer_) host_timer_->publish_state("待機");
  if (host_connection_) host_connection_->publish_state("啟動中");

  // Send standby command on startup to sync host state
  command_pending_ = true;
}

void PanasonicFV30BUY3W::loop() {
  uint32_t now = millis();
  // Full cycle: send ~590ms + gap 130ms + receive ~610ms + gap 130ms = ~1460ms
  // Use last_cycle_ to avoid overlapping cycles
  if (now - last_cycle_ < 1460) return;
  last_cycle_ = now;

  // 1. Send polling or command (single send, no retry)
  if (command_pending_) {
    const CommandEntry *cmd = find_command(current_mode_, current_timer_key_);
    if (cmd) {
      ESP_LOGI(TAG, "Sending command: %s %s", cmd->mode, cmd->timer_key);
      send_waveform(pin_, cmd->waveform, cmd->len);
      command_pending_ = false;
    } else {
      ESP_LOGW(TAG, "No command for %s/%s, sending polling",
               current_mode_.c_str(), current_timer_key_.c_str());
      send_waveform(pin_, WF_POLLING, sizeof(WF_POLLING) / sizeof(int));
      command_pending_ = false;
    }
  } else {
    send_waveform(pin_, WF_POLLING, sizeof(WF_POLLING) / sizeof(int));
  }

  // 2. Wait inter-packet gap
  delay(INTER_PACKET_GAP_MS);

  // 3. Receive host response
  int rx_buf[MAX_WAVEFORM_LEN];
  int rx_len = receive_waveform(pin_, rx_buf, MAX_WAVEFORM_LEN);

  // 4. Process response
  if (rx_len > 0) {
    no_response_count_ = 0;
    std::string status = match_response(rx_buf, rx_len);

    if (!status.empty()) {
      if (host_connection_) host_connection_->publish_state("正常");

      // Standby sync — host timer went to 待機 from active
      if (status == "待機" && !last_host_timer_.empty() &&
          last_host_timer_ != "待機") {
        ESP_LOGI(TAG, "Host returned to standby (was %s), syncing mode",
                 last_host_timer_.c_str());
        current_mode_ = "待機";
        current_timer_key_ = "";
        if (mode_select_) mode_select_->publish_state("待機");
      }

      last_host_timer_ = status;
      if (host_timer_) host_timer_->publish_state(status);

    } else {
      // Unmatched response — keep last known state, log full waveform
      if (host_connection_) host_connection_->publish_state("正常");
      std::string raw = "[";
      for (int i = 0; i < rx_len; i++) {
        if (i > 0) raw += ",";
        raw += to_string(rx_buf[i]);
      }
      raw += "]";
      ESP_LOGD(TAG, "Unmatched response (%d values): %s", rx_len, raw.c_str());
    }
  } else {
    no_response_count_++;
    if (no_response_count_ >= 3) {
      if (host_connection_) host_connection_->publish_state("無回應");
    }
    ESP_LOGD(TAG, "No response (count: %d)", no_response_count_);
  }

  // 5. Return to idle
  pinMode(pin_, OUTPUT_OPEN_DRAIN);
  digitalWrite(pin_, HIGH);
}

}  // namespace panasonic_fv30buy3w
}  // namespace esphome
