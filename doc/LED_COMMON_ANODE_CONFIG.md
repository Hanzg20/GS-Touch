# LED 共阳极配置指南

配置日期：2025-11-02
硬件要求：共阳极LED连接方式

---

## 🎯 什么是共阳极LED？

### 共阴极 vs 共阳极对比

```
共阴极 (Common Cathode)          共阳极 (Common Anode)
━━━━━━━━━━━━━━━━━━━━━━━━       ━━━━━━━━━━━━━━━━━━━━━━━━
     VCC (+3.3V)                       VCC (+3.3V)
        │                                    │
        ├───────┐                           LED
       LED      │                            │
        │       │                            ├───────┐
     GPIO ──────┤                         GPIO      │
        │       │                            │       │
       GND ─────┘                           GND ─────┘

GPIO=HIGH → LED亮               GPIO=HIGH → LED灭
GPIO=LOW  → LED灭               GPIO=LOW  → LED亮
```

---

## ⚙️ 软件配置

### config.h 配置（第41-44行）

```cpp
// LED极性配置
// true = 共阳极 (HIGH=熄灭, LOW=点亮)
// false = 共阴极 (HIGH=点亮, LOW=熄灭)
#define LED_COMMON_ANODE true    // ← 共阳极模式
```

**切换到共阴极**（如需要）：
```cpp
#define LED_COMMON_ANODE false   // 共阴极模式
```

---

## 🔧 实现原理

### 自动极性转换

#### GoldSky_Utils.ino 实现

```cpp
// LED输出辅助函数 - 根据极性配置自动反相
inline void ledWrite(int pin, bool state) {
  #if LED_COMMON_ANODE
    digitalWrite(pin, !state);  // 共阳极: 反相输出
  #else
    digitalWrite(pin, state);   // 共阴极: 直接输出
  #endif
}
```

#### 工作原理

| 代码调用 | 共阴极输出 | 共阳极输出 | LED状态 |
|---------|-----------|-----------|--------|
| `ledWrite(pin, LOW)` | LOW | **HIGH** | 熄灭 |
| `ledWrite(pin, HIGH)` | HIGH | **LOW** | 点亮 |

**关键**: 上层代码逻辑不变，`ledWrite()`函数自动处理极性转换！

---

## 📝 代码修改清单

### 修改的文件

#### 1. config.h
```cpp
// 新增配置项（第41-44行）
#define LED_COMMON_ANODE true
```

#### 2. GoldSky_Utils.ino

**新增函数**（第43-50行）：
```cpp
inline void ledWrite(int pin, bool state) {
  #if LED_COMMON_ANODE
    digitalWrite(pin, !state);
  #else
    digitalWrite(pin, state);
  #endif
}
```

**修改函数**：

**updateSingleLED()** - 全部digitalWrite()改为ledWrite()：
```cpp
void updateSingleLED(int pin, LEDStatus status) {
  switch (status) {
    case LED_OFF:
      ledWrite(pin, LOW);      // ← 修改
      break;
    case LED_ON:
      ledWrite(pin, HIGH);     // ← 修改
      break;
    case LED_BLINK_SLOW:
      // ...
      ledWrite(pin, ledIndicator.blinkState ? HIGH : LOW);  // ← 修改
      break;
    case LED_BLINK_FAST:
      // ...
      ledWrite(pin, ledIndicator.blinkState ? HIGH : LOW);  // ← 修改
      break;
  }
}
```

**updateLEDIndicators()** - LED_POWER控制改为ledWrite()：
```cpp
void updateLEDIndicators() {
  ledWrite(LED_POWER, ledIndicator.power ? HIGH : LOW);  // ← 修改
  updateSingleLED(LED_NETWORK, ledIndicator.network);
  updateSingleLED(LED_PROGRESS, ledIndicator.progress);
  updateSingleLED(LED_STATUS, ledIndicator.status);
}
```

---

## 🔌 硬件连接

### 共阳极LED接线

```
ESP32-S3                  LED                电阻
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
                           ┌─── VCC (+3.3V)
                           │
GPIO 4 (LED_POWER) ────────┤
                           │
                          LED (红色 - 电源指示)
                           │
                        [220Ω]
                           │
                          GND

GPIO 5 (LED_NETWORK) ──────┤
                           │
                          LED (绿色 - 网络状态)
                           │
                        [220Ω]
                           │
                          GND

GPIO 6 (LED_PROGRESS) ─────┤
                           │
                          LED (黄色 - 进度指示)
                           │
                        [220Ω]
                           │
                          GND

GPIO 7 (LED_STATUS) ───────┤
                           │
                          LED (蓝色 - 状态指示)
                           │
                        [220Ω]
                           │
                          GND
```

### 电阻选择

| LED颜色 | 工作电压 | 工作电流 | 推荐电阻 |
|--------|---------|---------|---------|
| 红色 | 1.8-2.2V | 20mA | 220Ω |
| 绿色 | 2.0-3.0V | 20mA | 220Ω |
| 黄色 | 2.0-2.4V | 20mA | 220Ω |
| 蓝色 | 3.0-3.4V | 20mA | 100Ω |
| 白色 | 3.0-3.6V | 20mA | 100Ω |

**计算公式**：
```
R = (Vcc - Vled) / I
  = (3.3V - 2.0V) / 0.02A
  = 65Ω（选择标准值220Ω更安全）
```

---

## 🧪 测试验证

### 上电测试

上传代码后，观察LED行为：

| LED | 状态 | 预期行为（共阳极） |
|-----|------|-----------------|
| **POWER** | 常亮 | GPIO 4 = LOW |
| **NETWORK** | WiFi连接成功时常亮 | GPIO 5 = LOW |
| **NETWORK** | WiFi断开时慢闪 | GPIO 5 = LOW/HIGH交替（500ms） |
| **PROGRESS** | 选择套餐后常亮 | GPIO 6 = LOW |
| **STATUS** | 刷卡时快闪 | GPIO 7 = LOW/HIGH交替（100ms） |

### 万用表测量

使用万用表测量GPIO引脚电压：

**共阳极模式（LED_COMMON_ANODE = true）**：
```
LED亮时: GPIO电压 = 0V (LOW)
LED灭时: GPIO电压 = 3.3V (HIGH)
```

**共阴极模式（LED_COMMON_ANODE = false）**：
```
LED亮时: GPIO电压 = 3.3V (HIGH)
LED灭时: GPIO电压 = 0V (LOW)
```

### 示波器验证（可选）

观察闪烁LED的波形：

**慢闪（500ms）**：
```
      500ms      500ms
     ┌─────┐            ┌─────┐
3.3V │     │            │     │  (共阳极: LED灭)
     │     │            │     │
 0V  ┘     └────────────┘     └  (共阳极: LED亮)
```

**快闪（100ms）**：
```
     100ms  100ms
     ┌──┐   ┌──┐   ┌──┐
3.3V │  │   │  │   │  │
     │  │   │  │   │  │
 0V  ┘  └───┘  └───┘  └
```

---

## ⚠️ 常见问题

### 1. LED全部常亮（应该灭的也亮）

**原因**: LED_COMMON_ANODE配置错误

**解决**:
```cpp
// 检查config.h第44行
#define LED_COMMON_ANODE true  // 确认是true
```

### 2. LED全部常灭（应该亮的也灭）

**原因**: LED_COMMON_ANODE配置反了

**解决**:
```cpp
// 如果硬件是共阴极，改为：
#define LED_COMMON_ANODE false
```

### 3. LED闪烁反相（应该亮时灭，应该灭时亮）

**原因**: 硬件接线与配置不匹配

**检查**:
1. 确认LED阳极是否真的接VCC
2. 确认LED阴极是否通过电阻接GPIO
3. 测量VCC电压是否正常3.3V

### 4. 部分LED不亮

**可能原因**:
- LED反接（阳极阴极接反）
- 电阻值过大
- GPIO引脚损坏
- LED烧坏

**排查步骤**:
```
1. 断电，检查LED极性
2. 测量电阻值（应该220Ω左右）
3. 更换LED测试
4. 检查GPIO引脚是否有输出（万用表测量）
```

---

## 🔄 切换极性配置

如果硬件从共阳极改为共阴极（或反过来），只需修改一行：

### 从共阳极切换到共阴极

```cpp
// config.h 第44行
#define LED_COMMON_ANODE false  // 改为false
```

### 从共阴极切换到共阳极

```cpp
// config.h 第44行
#define LED_COMMON_ANODE true   // 改为true
```

**重新编译上传即可，其他代码无需修改！**

---

## 📊 LED状态表（共阳极模式）

### 各状态下的LED行为

| 系统状态 | POWER | NETWORK | PROGRESS | STATUS |
|---------|-------|---------|----------|--------|
| **欢迎界面** | ● 常亮 | ● 常亮 / ◐ 慢闪 | ○ 熄灭 | ○ 熄灭 |
| **选择套餐** | ● 常亮 | ● 常亮 / ◐ 慢闪 | ● 常亮 | ○ 熄灭 |
| **刷卡等待** | ● 常亮 | ● 常亮 / ◐ 慢闪 | ● 常亮 | ◉ 快闪 |
| **VIP查询** | ● 常亮 | ● 常亮 / ◐ 慢闪 | ● 常亮 | ◉ 快闪 |
| **VIP显示** | ● 常亮 | ● 常亮 / ◐ 慢闪 | ◐ 慢闪 | ● 常亮 |
| **洗车中** | ● 常亮 | ● 常亮 | ● 常亮 | ● 常亮 |
| **完成** | ● 常亮 | ● 常亮 / ◐ 慢闪 | ◐ 慢闪 | ○ 熄灭 |
| **错误** | ● 常亮 | ● 常亮 / ◐ 慢闪 | ○ 熄灭 | ◉ 快闪 |

**图例**:
- ● 常亮（GPIO = LOW）
- ○ 熄灭（GPIO = HIGH）
- ◐ 慢闪（500ms，GPIO = LOW/HIGH交替）
- ◉ 快闪（100ms，GPIO = LOW/HIGH交替）

---

## 💡 优化建议

### 电流优化

共阳极模式下，多个LED同时亮起时，GPIO口会同时输出低电平，总电流消耗：

```
4个LED × 20mA = 80mA
```

ESP32-S3 GPIO总输出电流限制：**40mA per pin, 200mA total**

**建议**:
1. 使用更大的限流电阻（330Ω-470Ω）降低单LED电流到10-15mA
2. 或使用三极管驱动（推荐高电流应用）

### 三极管驱动电路（可选）

```
ESP32-S3          NPN三极管           LED
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
                                      VCC
                                       │
GPIO 4 ───[10kΩ]─── B                 LED
                    │                  │
                    C                  │
                    │                  │
                   GND ────────────────┘

(共阳极用NPN，GPIO=HIGH时LED亮)
```

---

## 📚 相关文档

- [config.h](../config.h) - 主配置文件
- [GoldSky_Utils.ino](../GoldSky_Utils.ino) - LED控制实现
- [引脚定义图](PIN_CONFIG_V6.md) - 完整引脚说明

---

## 🔧 故障排除流程

```
LED不工作？
  │
  ├─ 检查配置: LED_COMMON_ANODE = true? ────── 否 → 修改为true
  │                 │
  │                 是
  │                 ↓
  ├─ 测量GPIO电压: LED亮时=0V, 灭时=3.3V? ─── 否 → 检查硬件接线
  │                 │
  │                 是
  │                 ↓
  ├─ 检查电阻: 220Ω左右? ───────────────── 否 → 更换电阻
  │                 │
  │                 是
  │                 ↓
  ├─ 检查LED极性: 阳极接VCC, 阴极接GPIO? ─── 否 → 反接LED
  │                 │
  │                 是
  │                 ↓
  └─ 更换LED测试 ───────────────────────────────→ 解决
```

---

**文档版本**: v1.0
**最后更新**: 2025-11-02
**适用版本**: GoldSky_Lite v2.4+
