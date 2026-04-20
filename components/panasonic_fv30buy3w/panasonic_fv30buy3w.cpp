#include "panasonic_fv30buy3w.h"
#include "esphome/core/log.h"

namespace esphome {
namespace panasonic_fv30buy3w {

static const char *TAG = "panasonic_fv30buy3w";

// ============ Select Control ============

void FanModeSelect::control(const std::string &value) {
  this->publish_state(value);
  if (parent_) parent_->on_mode_set(value);
}

// ============ Command Lookup ============

const CommandEntry *PanasonicFV30BUY3W::find_command(const std::string &display) {
  for (int i = 0; i < NUM_COMMANDS; i++) {
    if (display == COMMANDS[i].display) return &COMMANDS[i];
  }
  return nullptr;
}

// ============ Protocol I/O ============

void PanasonicFV30BUY3W::send_waveform(uint8_t pin, const int *waveform, int len) {
  pinMode(pin, OUTPUT_OPEN_DRAIN);

  for (int i = 0; i < len; i++) {
    digitalWrite(pin, (i % 2 == 0) ? LOW : HIGH);
    delayMicroseconds(waveform[i] * T_US);
  }

  digitalWrite(pin, HIGH);
}

int PanasonicFV30BUY3W::receive_waveform(uint8_t pin, int *buffer, int max_len) {
  pinMode(pin, INPUT);
  unsigned long start = millis();

  while (digitalRead(pin) == HIGH) {
    if (millis() - start > RX_TIMEOUT_MS) return 0;
  }

  int count = 0;
  bool expect_low = true;

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

// ============ Mode Control ============

void PanasonicFV30BUY3W::on_mode_set(const std::string &value) {
  ESP_LOGI(TAG, "Mode set: %s", value.c_str());
  current_command_ = value;
  command_pending_ = true;

  countdown_duration_s_ = parse_duration(value);
  countdown_start_ms_ = millis();
  publish_expiry();
}

// ============ Countdown ============

uint32_t PanasonicFV30BUY3W::parse_duration(const std::string &command) {
  if (command == "Standby") return 0;
  if (command.find("Cont.") != std::string::npos) return UINT32_MAX;
  if (command.find("15m") != std::string::npos) return 15 * 60;
  if (command.find("30m") != std::string::npos) return 30 * 60;
  if (command.find("1h") != std::string::npos) return 3600;
  if (command.find("3h") != std::string::npos) return 3 * 3600;
  if (command.find("6h") != std::string::npos) return 6 * 3600;
  return 0;
}

void PanasonicFV30BUY3W::check_timer_expiry() {
  if (countdown_duration_s_ == 0 || countdown_duration_s_ == UINT32_MAX) return;

  uint32_t elapsed = (millis() - countdown_start_ms_) / 1000;
  if (elapsed < countdown_duration_s_) return;

  ESP_LOGI(TAG, "Countdown expired, switching to standby");
  current_command_ = "Standby";
  command_pending_ = true;
  countdown_duration_s_ = 0;
  if (mode_select_) mode_select_->publish_state("Standby");
  // Intentionally leave timer_expires_ alone: HA keeps showing the past
  // expiry ("X ago"), accurate for natural expiry. Only user-chosen Standby clears it.
}

void PanasonicFV30BUY3W::publish_expiry() {
  if (!timer_expires_) {
    expiry_pending_publish_ = false;
    return;
  }

  // Standby or Continuous → no valid expiry timestamp
  if (countdown_duration_s_ == 0 || countdown_duration_s_ == UINT32_MAX) {
    if (last_expiry_str_ != "") {
      last_expiry_str_ = "";
      timer_expires_->publish_state("");
    }
    expiry_pending_publish_ = false;
    return;
  }

  // Defer until HA time is synchronised.
  if (time_ == nullptr || !time_->now().is_valid()) {
    expiry_pending_publish_ = true;
    return;
  }

  uint32_t elapsed = (millis() - countdown_start_ms_) / 1000;
  if (elapsed >= countdown_duration_s_) {
    expiry_pending_publish_ = false;
    return;
  }
  uint32_t remaining_s = countdown_duration_s_ - elapsed;

  time_t expiry_epoch = (time_t) (time_->now().timestamp + remaining_s);
  struct tm tm_utc;
  gmtime_r(&expiry_epoch, &tm_utc);
  char buf[32];
  strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%S+00:00", &tm_utc);
  std::string s(buf);

  if (s != last_expiry_str_) {
    last_expiry_str_ = s;
    timer_expires_->publish_state(s);
  }
  expiry_pending_publish_ = false;
}

// ============ Component Lifecycle ============

void PanasonicFV30BUY3W::setup() {
  ESP_LOGI(TAG, "Setting up on GPIO%d", pin_);
  pinMode(pin_, OUTPUT_OPEN_DRAIN);
  digitalWrite(pin_, HIGH);

  if (mode_select_) mode_select_->publish_state("Standby");
  if (host_connection_) host_connection_->publish_state(false);
  if (timer_expires_) timer_expires_->publish_state("");

  command_pending_ = true;
}

void PanasonicFV30BUY3W::loop() {
  uint32_t now = millis();

  // Once per second: check for timer expiry and retry deferred publish.
  if (now - last_countdown_update_ >= 1000) {
    last_countdown_update_ = now;
    check_timer_expiry();
    if (expiry_pending_publish_) publish_expiry();
  }

  if (now - last_cycle_ < 1460) return;
  last_cycle_ = now;

  // 1. Send command or polling
  // Only send commands after host is confirmed reachable (last_connected_)
  if (command_pending_ && last_connected_) {
    const CommandEntry *cmd = find_command(current_command_);
    if (cmd) {
      ESP_LOGI(TAG, "Sending command: %s", cmd->display);
      send_waveform(pin_, cmd->waveform, cmd->len);
      command_pending_ = false;
    } else {
      ESP_LOGW(TAG, "Unknown command: %s, sending polling", current_command_.c_str());
      send_waveform(pin_, WF_POLLING, sizeof(WF_POLLING) / sizeof(int));
      command_pending_ = false;
    }
  } else {
    send_waveform(pin_, WF_POLLING, sizeof(WF_POLLING) / sizeof(int));
  }

  // 2. Wait inter-packet gap
  delay(INTER_PACKET_GAP_MS);

  // 3. Receive host response — only used for connection status
  int rx_buf[MAX_WAVEFORM_LEN];
  int rx_len = receive_waveform(pin_, rx_buf, MAX_WAVEFORM_LEN);

  if (rx_len > 0) {
    no_response_count_ = 0;
    if (!last_connected_) {
      last_connected_ = true;
      if (host_connection_) host_connection_->publish_state(true);
    }
  } else {
    no_response_count_++;
    ESP_LOGD(TAG, "No response (count: %d)", no_response_count_);
    if (no_response_count_ >= 3 && last_connected_) {
      last_connected_ = false;
      if (host_connection_) host_connection_->publish_state(false);
    }
  }

  // 4. Return to idle
  pinMode(pin_, OUTPUT_OPEN_DRAIN);
  digitalWrite(pin_, HIGH);
}

}  // namespace panasonic_fv30buy3w
}  // namespace esphome
