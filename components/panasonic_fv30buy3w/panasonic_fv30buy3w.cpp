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
  update_countdown();
}

// ============ Countdown ============

uint32_t PanasonicFV30BUY3W::parse_duration(const std::string &command) {
  if (command == "待機") return 0;
  if (command.find("24") != std::string::npos) return UINT32_MAX;
  if (command.find("15分") != std::string::npos) return 15 * 60;
  if (command.find("30分") != std::string::npos) return 30 * 60;
  if (command.find("1小時") != std::string::npos) return 3600;
  if (command.find("3小時") != std::string::npos) return 3 * 3600;
  if (command.find("6小時") != std::string::npos) return 6 * 3600;
  return 0;
}

void PanasonicFV30BUY3W::update_countdown() {
  std::string text;
  if (countdown_duration_s_ == 0) {
    text = "0s";
  } else if (countdown_duration_s_ == UINT32_MAX) {
    text = "連續";
  } else {
    uint32_t elapsed = (millis() - countdown_start_ms_) / 1000;
    int32_t remaining = (int32_t)countdown_duration_s_ - (int32_t)elapsed;
    if (remaining <= 0) {
      // Timer expired — send standby and reset
      ESP_LOGI(TAG, "Countdown expired, switching to standby");
      current_command_ = "待機";
      command_pending_ = true;
      countdown_duration_s_ = 0;
      if (mode_select_) mode_select_->publish_state("待機");
      text = "0s";
    } else if (remaining < 60) {
      // Last minute — show seconds
      text = to_string(remaining) + "s";
    } else if (remaining < 3600) {
      int m = remaining / 60;
      text = to_string(m) + "m";
    } else {
      int h = remaining / 3600;
      int m = (remaining % 3600) / 60;
      text = to_string(h) + "h " + to_string(m) + "m";
    }
  }

  if (remaining_time_ && text != last_remaining_str_) {
    last_remaining_str_ = text;
    remaining_time_->publish_state(text);
  }
}

// ============ Component Lifecycle ============

void PanasonicFV30BUY3W::setup() {
  ESP_LOGI(TAG, "Setting up on GPIO%d", pin_);
  pinMode(pin_, OUTPUT_OPEN_DRAIN);
  digitalWrite(pin_, HIGH);

  if (mode_select_) mode_select_->publish_state("待機");
  if (host_connection_) host_connection_->publish_state(false);
  if (remaining_time_) remaining_time_->publish_state("0s");

  command_pending_ = true;
}

void PanasonicFV30BUY3W::loop() {
  uint32_t now = millis();

  // Update countdown every second
  if (now - last_countdown_update_ >= 1000) {
    last_countdown_update_ = now;
    update_countdown();
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
