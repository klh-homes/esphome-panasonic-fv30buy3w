# Panasonic FV-30BUY3W ESPHome Integration

ESP32 取代 Panasonic FV-30BUY3W 浴室換氣暖風機的原廠面板，透過 ESPHome / Home Assistant 控制。

![Home Assistant Screenshot](docs/ha-screenshot.png)

## 功能

- 透過 ESPHome / Home Assistant 控制換氣、取暖、乾燥熱、乾燥涼模式
- 支援 15分 / 30分 / 1小時 / 3小時 / 6小時 / 24小時 定時
- ESP32 內建倒數計時，到時自動切回待機
- 主機連線狀態即時偵測

## 硬體需求

| 零件                   | 數量 | 說明                                    |
| ---------------------- | ---- | --------------------------------------- |
| ESP32 開發板           | 1    | NodeMCU-32S 或同等 ESP32-WROOM-32       |
| 2N7000                 | 1    | N-channel MOSFET（TO-92，用於電位轉換） |
| 10kΩ 電阻              | 2    | 上拉電阻                                |
| JST PH 2.0mm 3pin 接頭 | 1    | 對接主機 CN201                          |

**⚠️ ESP32 GPIO 不是 5V tolerant（絕對最大值 3.6V），必須使用 level shifter。**

### 接線圖（2N7000 雙向 level shifter）

```
ESP32 GPIO23 ──┬── 2N7000 Source (S)
               │
            [10kΩ]── ESP32 3V3

2N7000 Gate (G) ──── ESP32 3V3

主機 Data(黑) ──┬── 2N7000 Drain (D)
                │
             [10kΩ]── 主機 5V(紅)

ESP32 GND ───── 主機 GND(白)
```

2N7000 腳位（TO-92 正面朝自己）：`[ S ] [ G ] [ D ]`

CN201 線色：白=GND、紅=5V、黑=Data

## 安裝步驟

### 1. 硬體組裝

1. 依上方接線圖接好 2N7000 level shifter
2. 拔掉原廠面板的 CN201 接頭，改接到 ESP32
3. ESP32 用 USB 供電（正式版可改接主機 5V 到 VIN）

### 2. 設定

1. Clone 此 repo
2. 安裝 ESPHome：`pipx install esphome`
3. 建立 `secrets.yaml`：
   ```yaml
   device_name: "your-device-name"
   friendly_name: "Your Device Name"
   wifi_ssid: "your-wifi-ssid"
   wifi_password: "your-wifi-password"
   ```
4. 如需修改 GPIO pin，編輯 `fan.yaml` 中的 `pin: 23`

### 3. 燒錄

首次燒錄（USB）：

```bash
esphome run fan.yaml --device /dev/ttyUSB0
```

後續更新（OTA）：

```bash
esphome run fan.yaml --device OTA
```

### 4. Home Assistant

1. HA 會自動發現 ESPHome 裝置
2. 加入裝置後即可使用 Fan Mode 選擇模式

## Home Assistant Entities

| Entity          | 類型                     | 說明                                                 |
| --------------- | ------------------------ | ---------------------------------------------------- |
| Fan Mode        | select                   | 待機 / 換氣 15分 / ... / 乾燥涼 24小時（共 23 選項） |
| Remaining Time  | text_sensor              | ESP32 內部倒數計時（24小時模式顯示「連續」）         |
| Host Connection | binary_sensor            | 主機回應狀態 ON/OFF                                  |
| IP Address      | text_sensor (diagnostic) | IP 位址                                              |
| Uptime          | sensor (diagnostic)      | 運行時間（小時）                                     |
| WiFi Signal     | sensor (diagnostic)      | WiFi 信號強度                                        |

### 可用模式

| 模式   | 可選定時                                     |
| ------ | -------------------------------------------- |
| 換氣   | 15分 / 30分 / 1小時 / 3小時 / 6小時 / 24小時 |
| 取暖   | 15分 / 30分 / 1小時 / 3小時                  |
| 乾燥熱 | 15分 / 30分 / 1小時 / 3小時 / 6小時          |
| 乾燥涼 | 15分 / 30分 / 1小時 / 3小時 / 6小時 / 24小時 |

## PoC 驗證（可選）

如果想先驗證硬體通訊是否正常，可以用 Arduino IDE 燒錄 PoC：

1. Arduino IDE 安裝 ESP32 board support
2. 開啟 `poc/poc.ino`，選擇 Board: NodeMCU-32S
3. 燒錄，開 Serial Monitor (115200)

預期輸出：

```
=== Panasonic FV-30BUY3W PoC ===
Version: 0.1.1
#1 | 無回應
#2 | 無回應
#3 | 主機狀態: 待機 (79 values)
#4 | 主機狀態: 待機 (79 values)
```

## 專案結構

```
├── poc/poc.ino                          # PoC Arduino sketch（Serial debug）
├── fan.yaml                             # ESPHome 設定檔
├── components/panasonic_fv30buy3w/      # ESPHome custom component
│   ├── __init__.py
│   ├── select.py
│   ├── binary_sensor.py
│   ├── text_sensor.py
│   ├── panasonic_fv30buy3w.h
│   └── panasonic_fv30buy3w.cpp
└── .claude/                             # 協議文件（Claude Code 參考用）
    ├── PROMPT-claude-code.md
    ├── panasonic-fv30buy3w-protocol-analysis.md
    ├── panasonic-fv30buy3w-packets.json
    └── capture/
```

## 協議摘要

- 自訂脈衝寬度編碼（非 UART / 非 1-Wire）
- 單線半雙工，面板=master，主機=slave
- 1T = 3300us，idle = HIGH
- 通訊週期 ~1460ms（面板 590ms + 130ms gap + 主機 610ms + 130ms gap）
- 封包格式: waveform 陣列，交替 [LOW_T, HIGH_T, LOW_T, HIGH_T, ...]（偶數 index=LOW，奇數 index=HIGH）
- 封包從 LOW 開始（將 idle HIGH 拉低），以 LOW 結束後回到 idle HIGH
- 22 個指令封包 + 1 polling，全部查表法
- 主機回應只用於偵測連線狀態（有回應 = ON），不解析內容
- 定時由 ESP32 內部倒數計時管理，倒數歸零自動送待機指令

## Claude Code 注意事項

- `.claude/` 下的檔案是協議的 source of truth，每次新對話請先讀取
- 封包資料使用 waveform 格式（交替 L/H），偶數 index (0,2,4,...)=LOW，奇數 index (1,3,5,...)=HIGH
- GPIO 使用 OUTPUT_OPEN_DRAIN 模式，搭配 2N7000 level shifter + pull-up
- ESP32 GPIO 不是 5V tolerant，絕對不能直接接 5V 信號
