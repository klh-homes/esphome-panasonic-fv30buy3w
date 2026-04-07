# Panasonic FV-30BUY3W ESPHome Integration

ESP32 取代 Panasonic FV-30BUY3W 浴室換氣暖風機的原廠面板，透過 Home Assistant 控制。

## 原理

原廠面板與主機之間使用自訂脈衝寬度編碼協議，單線半雙工通訊。ESP32 取代面板角色（master），發送 polling/指令封包，接收主機回應，透過 ESPHome 整合到 Home Assistant。

協議細節見 `.claude/panasonic-fv30buy3w-protocol-analysis.md`，封包資料見 `.claude/panasonic-fv30buy3w-packets.json`。

## 硬體

- **主機**: Panasonic FV-30BUY3W
- **控制器**: ESP32 (NodeMCU-32S 或同等 ESP32-WROOM-32 開發板)
- **電位轉換**: 2N7000 N-channel MOSFET + 10kΩ pull-up ×2
- **連接器**: CN201 JST PH 2.0mm 3pin（白=GND, 紅=5V, 黑=Data）

**⚠️ ESP32 GPIO 不是 5V tolerant（絕對最大值 3.6V），必須使用 level shifter。**

### 接線（2N7000 雙向 level shifter）

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

PoC 階段 ESP32 用 USB 供電，主機 5V 紅線只提供 Drain 側 pull-up。
正式版可將主機 5V 紅線接 ESP32 VIN 供電（拔 USB）。

2N7000 腳位（TO-92 正面朝你）：`[ S ] [ G ] [ D ]`

## 專案結構

```
├── poc/poc.ino                          # PoC Arduino sketch（Serial debug）
├── bathroom-fan.yaml                    # ESPHome 設定檔
├── components/panasonic_fv30buy3w/      # ESPHome custom component
│   ├── __init__.py
│   ├── select.py
│   ├── text_sensor.py
│   ├── panasonic_fv30buy3w.h
│   └── panasonic_fv30buy3w.cpp
└── .claude/                             # 協議文件（Claude Code 參考用）
    ├── PROMPT-claude-code.md            # 專案指引與 ESP32 建議常數
    ├── panasonic-fv30buy3w-protocol-analysis.md
    ├── panasonic-fv30buy3w-packets.json # 所有封包 waveform 資料
    └── capture/                         # PulseView 原始錄製檔 (.sr)
```

## Home Assistant Entities

| Entity       | 類型        | 說明                                                           |
| ------------ | ----------- | -------------------------------------------------------------- |
| 風機模式     | select      | 待機 / 換氣 / 取暖 / 乾燥熱 / 乾燥涼                           |
| 風機定時     | select      | 15分 / 30分 / 1小時 / 3小時 / 6小時 / 24小時（依模式動態調整） |
| 風機定時狀態 | text_sensor | 主機回報的定時狀態                                             |
| 風機連線狀態 | text_sensor | 啟動中 / 連線中 / 無回應 / 未知回應                            |

### 模式 x 定時可用矩陣

|        | 15m | 30m | 1h  | 3h  | 6h  | 24h |
| ------ | --- | --- | --- | --- | --- | --- |
| 換氣   | o   | o   | o   | o   | o   | o   |
| 取暖   | o   | o   | o   | o   |     |     |
| 乾燥熱 | o   | o   | o   | o   | o   |     |
| 乾燥涼 | o   | o   | o   | o   | o   | o   |

## 使用方式

### Step 1: PoC 驗證通訊

1. Arduino IDE 安裝 ESP32 board support
2. 開啟 `poc/poc.ino`，選擇 Board: NodeMCU-32S
3. 燒錄，開 Serial Monitor (115200)
4. 依上方接線圖接好 2N7000 level shifter，連接主機 Data/GND/5V

預期輸出：

```
=== Panasonic FV-30BUY3W PoC ===
Version: 0.1.1
#1 | 無回應
#2 | 無回應
#3 | 主機狀態: 待機 (79 values)
#4 | 主機狀態: 待機 (79 values)
```

### Step 2: ESPHome 正式版

1. 修改 `secrets.yaml` 中的 WiFi 設定
2. 用 ESPHome 編譯並燒錄
3. Home Assistant 中加入裝置

## 協議摘要

- 自訂脈衝寬度編碼（非 UART / 非 1-Wire）
- 單線半雙工，面板=master，主機=slave
- 1T = 3300us，idle = HIGH
- 通訊週期 ~1460ms（面板 590ms + 130ms gap + 主機 610ms + 130ms gap）
- 封包格式: waveform 陣列，交替 [LOW_T, HIGH_T, LOW_T, HIGH_T, ...]（偶數 index=LOW，奇數 index=HIGH）
- 封包從 LOW 開始（將 idle HIGH 拉低），以 LOW 結束後回到 idle HIGH
- 22 個指令封包 + 1 polling + 8 主機回應，全部查表法
- 主機回應只含定時資訊，不區分模式
- 主機回應比對方式：前 26 個值 prefix matching（index 0 允許 +/-1 容差），忽略 index 26 以後的 countdown 資料

## Claude Code 注意事項

- `.claude/` 下的檔案是協議的 source of truth，每次新對話請先讀取
- 封包資料使用 waveform 格式（交替 L/H），偶數 index (0,2,4,...)=LOW，奇數 index (1,3,5,...)=HIGH
- GPIO 使用 OUTPUT_OPEN_DRAIN 模式，搭配 2N7000 level shifter + pull-up
- ⚠️ ESP32 GPIO 不是 5V tolerant，絕對不能直接接 5V 信號
- 未來可能新增 proxy mode（ESP32 插在面板與主機之間），send/receive 函式已預留 pin 參數
