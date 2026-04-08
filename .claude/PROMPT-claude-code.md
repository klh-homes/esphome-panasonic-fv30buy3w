# Panasonic FV-30BUY3W — ESP32 ESPHome Integration

## Context

This project reverse-engineers the Panasonic FV-30BUY3W bathroom ventilation fan's control protocol. The ESP32 replaces the original control panel, communicating directly with the host unit.

**Read these files first:**

- `panasonic-fv30buy3w-packets.json` — all packet waveform data
- `panasonic-fv30buy3w-protocol-analysis.md` — full protocol analysis

## Hardware

- **Host unit:** Panasonic FV-30BUY3W
- **Controller:** ESP32 (NodeMCU-32S or equivalent ESP32-WROOM-32)
- **Level shifter:** 2N7000 N-channel MOSFET, bidirectional
- **Connector:** CN201 JST PH 2.0mm 3-pin
  - White = GND → ESP32 GND
  - Red = VCC 5V → 10kΩ pull-up (also ESP32 VIN for standalone)
  - Black = Data → 2N7000 Drain
- **ESP32 GPIOs are NOT 5V tolerant** (abs max 3.6V). Level shifter is mandatory.

### Wiring

```
ESP32 GPIO23 ──┬── 2N7000 Source
               │
            [10kΩ]── ESP32 3V3

2N7000 Gate ──── ESP32 3V3

Host Data (black) ──┬── 2N7000 Drain
                    │
                 [10kΩ]── Host 5V (red)

ESP32 GND ───── Host GND (white)
```

## Protocol Summary

- Custom pulse-width encoding, not UART
- Single-wire half-duplex, panel = master, host = slave
- Base time unit: 1T = 3300 µs
- Idle state: data line HIGH
- No handshake; panel starts polling immediately after power-on

### Timing (measured)

| Parameter        | Value               |
| ---------------- | ------------------- |
| Panel packet     | ~590 ms             |
| Inter-packet gap | ~130 ms (HIGH idle) |
| Host response    | ~610 ms             |
| Full cycle       | ~1460 ms            |
| Command latency  | 1 cycle (~700 ms)   |
| Panel boot       | ~440 ms             |
| Host boot        | ~3180 ms            |

### Waveform Format

The `waveform` arrays in `panasonic-fv30buy3w-packets.json`:

- **Even index (0, 2, 4, ...)** = LOW duration in T units (active pull-down)
- **Odd index (1, 3, 5, ...)** = HIGH duration in T units (release, pull-up)
- 1T = 3300 µs
- Packets start with LOW (first action: pull data line low from idle HIGH)
- Packets end with LOW, then data line returns to idle HIGH

Example: `[2, 1, 6, 2, 5, 2, 3, 1, ...]` means:

```
LOW 6600µs → HIGH 3300µs → LOW 19800µs → HIGH 6600µs → LOW 16500µs → ...
```

### Suggested Constants

```c
#define DATA_PIN          23
#define T_US              3300
#define IDLE_THRESHOLD_US 30000   // >9T = idle (max HIGH in packet = 7T = 23.1ms)
#define RX_TIMEOUT_MS     200
#define INTER_PACKET_GAP  130     // ms between send and receive
#define CYCLE_MS          1460    // full communication cycle
```

## Architecture

### Send

- GPIO: `OUTPUT_OPEN_DRAIN`
- Even index → `digitalWrite(LOW)`, odd index → `digitalWrite(HIGH)`
- Packet starts by pulling LOW, ends by releasing to idle HIGH
- 2N7000 translates 3.3V signals to 5V for host

### Receive

- GPIO: `INPUT`
- Detect falling edge (idle HIGH → first LOW)
- Measure each LOW/HIGH duration, quantize to T units
- HIGH exceeding `IDLE_THRESHOLD_US` = packet end
- **Host response is used only for connection detection** (rx_len > 0 = connected)
- Timer countdown is managed internally by ESP32; auto-standby on expiry

### ESPHome Entities

- `select.fan_mode` — single select: standby + 21 mode/timer combinations
- `text_sensor.remaining_time` — internal countdown display
- `binary_sensor.host_connection` — host communication status (diagnostic)

## Development

- Framework: ESPHome with Arduino
- Build: `esphome run fan.yaml`
