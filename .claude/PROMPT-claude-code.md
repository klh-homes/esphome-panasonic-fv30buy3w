# Panasonic FV-30BUY3W 浴室換氣機 — ESP32 ESPHome 整合

## 背景

我已完成 Panasonic FV-30BUY3W 浴室換氣暖風機的通訊協議逆向工程。
所有封包資料在 `panasonic-fv30buy3w-packets.json`，完整分析報告在 `panasonic-fv30buy3w-protocol-analysis.md`。
請先閱讀這兩個檔案再開始。

## 硬體

- **主機**: Panasonic FV-30BUY3W 浴室換氣暖風機
- **控制器**: ESP32 (NodeMCU-32S 或同等 ESP32-WROOM-32 開發板)
- **電位轉換**: 2N7000 N-channel MOSFET 雙向 level shifter
- **接線**: CN201 JST PH 2.0mm 3pin
  - 白線 = GND → ESP32 GND
  - 紅線 = VCC 5V → 10kΩ pull-up（正式版也接 ESP32 VIN 供電）
  - 黑線 = Data → 2N7000 Drain
- **⚠️ ESP32 GPIO 不是 5V tolerant**（絕對最大值 3.6V），必須使用 level shifter

### 2N7000 Level Shifter 接線

```
ESP32 GPIO23 ──┬── 2N7000 Source
               │
            [10kΩ]── ESP32 3V3
               
2N7000 Gate ──── ESP32 3V3

主機 Data(黑) ──┬── 2N7000 Drain
                │
             [10kΩ]── 主機 5V(紅)

ESP32 GND ───── 主機 GND(白)
```

## 協議摘要

- 自訂脈衝寬度編碼，非 UART
- 單線半雙工，面板=master，主機=slave
- 基礎時間單位 1T = 3300μs (3.3ms)
- Idle 狀態 = Data line HIGH
- 無 handshake，上電後直接開始 polling

### 通訊時序（實測值）

```
面板發送封包:     ~590ms
封包間靜默:       ~130ms (HIGH idle)
主機回應封包:     ~610ms
封包間靜默:       ~130ms (HIGH idle)
完整通訊週期:     ~1460ms (面板→主機→面板)
指令生效延遲:     1 個週期 (~700ms)
面板開機時間:     ~440ms (復電後開始 polling)
主機開機時間:     ~3180ms (復電後首次回應)
```

### 封包波形格式

`panasonic-fv30buy3w-packets.json` 中的 `waveform` 陣列格式：

- **偶數 index (0, 2, 4, ...) = LOW 持續時間**（以 T 為單位，主動拉低）
- **奇數 index (1, 3, 5, ...) = HIGH 持續時間**（以 T 為單位，釋放由 pull-up 拉高）
- 1T = 3300μs
- 封包從 LOW 開始（第一個動作是從 idle HIGH 拉低 data line）
- 封包以 LOW 結束，之後 data line 回到 HIGH idle

範例：`[2, 1, 6, 2, 5, 2, 3, 1, ...]` 表示：

```
LOW 6600μs → HIGH 3300μs → LOW 19800μs → HIGH 6600μs → LOW 16500μs → HIGH 6600μs → LOW 9900μs → HIGH 3300μs → ...
```

### ESP32 建議常數

```c
#define DATA_PIN        23         // GPIO，依實際接線修改
#define T_US            3300       // 1T = 3300μs
#define IDLE_THRESHOLD_US 30000    // HIGH 超過 30ms 視為 idle（封包內最大 HIGH = 7T = 23.1ms）
#define RX_TIMEOUT_MS   200        // 等主機回應的 timeout
#define POLLING_INTERVAL_MS 130    // 面板封包和主機回應之間的間隔
#define CYCLE_DELAY_MS  130        // 主機回應結束後到下一次 polling 的間隔
```

## 實作方案（先做取代面板模式）

ESP32 取代面板，直接與主機通訊：

```
ESP32 GPIO23 ──[2N7000 level shifter]── 主機 Data
```

### ESP32 工作流程

```
loop:
  1. 發送 polling 封包（或指令封包）   ~590ms
  2. 等待 ~130ms
  3. 接收主機回應                      ~610ms
  4. 解析主機回應（判斷定時狀態）
  5. 等待 ~130ms
  6. 回到 1
```

### 發送封包

- GPIO 設為 OUTPUT_OPEN_DRAIN
- 依 waveform 陣列，偶數 index 拉 LOW，奇數 index 釋放 HIGH
- 封包從 LOW 開始（拉低 data line），結束後釋放回 idle HIGH
- 2N7000 level shifter 將 3.3V 信號轉換為 5V 信號送至主機

### 接收封包

- GPIO 設為 INPUT
- 偵測 HIGH→LOW 邊緣（封包開始）
- 記錄每個 HIGH/LOW 持續時間
- HIGH 超過 IDLE_THRESHOLD_US 視為封包結束
- 2N7000 level shifter 將主機 5V 信號轉換為 3.3V 信號
- 將收到的 waveform 序列與已知主機回應封包比對

## Home Assistant 整合目標

### Entities

- `select.bathroom_fan_mode` — 模式選擇: 待機 / 換氣 / 取暖 / 乾燥熱 / 乾燥涼
- `select.bathroom_fan_timer` — 定時選擇: 15分 / 30分 / 1小時 / 3小時 / 6小時 / 24小時
  - 根據模式動態調整可用選項（取暖無 6h/24h，乾燥熱無 24h）
- `sensor.bathroom_fan_host_timer` — 主機回報的定時狀態
- `sensor.bathroom_fan_host_status` — 主機回應是否正常（連線狀態）

### 模式 × 定時矩陣

|        | 15m | 30m | 1h  | 3h  | 6h  | 24h |
| ------ | --- | --- | --- | --- | --- | --- |
| 換氣   | ✅  | ✅  | ✅  | ✅  | ✅  | ✅  |
| 取暖   | ✅  | ✅  | ✅  | ✅  | ❌  | ❌  |
| 乾燥熱 | ✅  | ✅  | ✅  | ✅  | ✅  | ❌  |
| 乾燥涼 | ✅  | ✅  | ✅  | ✅  | ✅  | ✅  |

## 開發環境

- 使用 ESPHome
- Home Assistant 已有 ESPHome 整合
- HA SSH 連線: `ssh hassio@klh-ha`（使用 ProxyJump via klh-public-server）

## 第一步：最小可行驗證

先寫一個簡單的 ESPHome YAML + custom component：

1. ESP32 發 polling 封包
2. 讀取主機回應
3. 在 HA log 中顯示收到的回應是什麼
4. 確認通訊成功後再加模式/定時控制
