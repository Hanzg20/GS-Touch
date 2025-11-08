# 📌 GoldSky_Lite v6.0 引脚配置速查表

**版本：** v6.0（用户优化版）
**日期：** 2025-10-30
**状态：** ✅ 已优化，无冲突

---

## 🎯 快速引脚配置（复制粘贴到代码）

```cpp
// =================== GPIO 引脚定义 (v6.0 用户优化版) ===================

// I2C 总线（OLED显示屏）
#define OLED_SDA 8
#define OLED_SCL 9

// SPI 总线（RC522 NFC模块）
#define RC522_CS   10
#define RC522_MOSI 11
#define RC522_SCK  12
#define RC522_MISO 13
#define RC522_RST  14

// 用户输入
#define BTN_OK     1      // 确认按钮
#define BTN_SELECT 2      // 选择按钮

// LED 指示灯系统（✅ 用户优化配置）
#define LED_POWER    3    // L1: 电源指示灯
#define LED_NETWORK  4    // L2: 网络指示灯
#define LED_PROGRESS 5    // L3: 进度指示灯
#define LED_STATUS   6    // L4: 状态指示灯

// 输出设备
#define PULSE_OUTPUT_PIN 7    // 脉冲输出（控制外部计时器）
#define BUZZER_PIN       16   // 蜂鸣器
```

---

## 📊 完整引脚分配表

| GPIO | 功能 | 方向 | 说明 | 电平 |
|------|------|------|------|------|
| **GPIO 1** | BTN_OK | 输入 | 确认按钮（内部上拉） | 按下=LOW |
| **GPIO 2** | BTN_SELECT | 输入 | 选择按钮（内部上拉） | 按下=LOW |
| **GPIO 3** | LED_POWER | 输出 | L1 电源指示灯 | HIGH=亮 |
| **GPIO 4** | LED_NETWORK | 输出 | L2 网络指示灯 | HIGH=亮 |
| **GPIO 5** | LED_PROGRESS | 输出 | L3 进度指示灯 | HIGH=亮 |
| **GPIO 6** | LED_STATUS | 输出 | L4 状态指示灯 | HIGH=亮 |
| **GPIO 7** | PULSE_OUT | 输出 | 脉冲输出（控制外部设备） | HIGH=触发 |
| **GPIO 8** | OLED_SDA | I2C | OLED数据线 | - |
| **GPIO 9** | OLED_SCL | I2C | OLED时钟线 | - |
| **GPIO 10** | RC522_CS | SPI | NFC片选信号 | LOW=选中 |
| **GPIO 11** | RC522_MOSI | SPI | NFC主机输出 | - |
| **GPIO 12** | RC522_SCK | SPI | NFC时钟 | - |
| **GPIO 13** | RC522_MISO | SPI | NFC从机输出 | - |
| **GPIO 14** | RC522_RST | 输出 | NFC复位信号 | HIGH=正常 |
| **GPIO 16** | BUZZER | 输出 | 蜂鸣器 | HIGH=响 |

---

## 🔄 版本对比：v5.2 vs v6.0

### LED系统变化

| LED功能 | v5.2引脚 | v6.0引脚 | 变化 |
|---------|---------|---------|------|
| L1 电源灯 | 无 | GPIO 3 | ✅ 新增 |
| L2 网络灯 | GPIO 6（蓝） | GPIO 4 | ✅ 调整 |
| L3 进度灯 | GPIO 5（绿） | GPIO 5 | ✅ 保持 |
| L4 状态灯 | GPIO 7（红） | GPIO 6 | ✅ 调整 |

### 脉冲输出变化

| 功能 | v5.2引脚 | v6.0引脚 | 说明 |
|------|---------|---------|------|
| PULSE_OUT | GPIO 4 | GPIO 7 | ✅ 调整到GPIO 7，驱动能力更强 |

### 优化效果
- ✅ **LED连续布局**：GPIO 3-6，便于PCB布线
- ✅ **无引脚冲突**：Phase1文档中的GPIO 7冲突已解决
- ✅ **硬件友好**：脉冲输出独立，无干扰

---

## 🔌 硬件接线图

### LED 指示灯（4个独立LED）

```
ESP32-S3          LED（共阴接法，推荐）
---------         ----------------------
GPIO 3  --------> LED1 正极（L1 电源灯）
GPIO 4  --------> LED2 正极（L2 网络灯）
GPIO 5  --------> LED3 正极（L3 进度灯）
GPIO 6  --------> LED4 正极（L4 状态灯）

所有LED负极 ----> GND
建议：每个LED串联220Ω限流电阻
```

**LED推荐颜色：**
- L1 电源灯：白色或绿色（常亮）
- L2 网络灯：蓝色（WiFi状态）
- L3 进度灯：黄色或橙色（处理中闪烁）
- L4 状态灯：绿色（正常常亮/异常闪烁）

### 脉冲输出

```
ESP32-S3          外部设备
---------         ---------------------
GPIO 7  --------> 继电器/光耦输入
                  （或直接驱动3.3V逻辑设备）
GND     --------> GND
```

**脉冲参数：**
- 脉冲宽度：100ms
- 脉冲间隔：1000ms（1秒）
- 电平：HIGH触发

**注意事项：**
- 如果外部设备需要5V/12V驱动，需要增加电平转换电路
- 建议使用光耦隔离，避免干扰ESP32

### OLED 显示屏（I2C）

```
SSD1306 OLED      ESP32-S3
-------------     ---------
VCC     --------> 3.3V
GND     --------> GND
SDA     --------> GPIO 8
SCL     --------> GPIO 9
```

**I2C地址：** 0x3C（默认）

### RC522 NFC模块（SPI）

```
RC522 模块        ESP32-S3
-----------       ---------
SDA (CS)  ------> GPIO 10
SCK       ------> GPIO 12
MOSI      ------> GPIO 11
MISO      ------> GPIO 13
IRQ       ------> 不接（未使用）
GND       ------> GND
RST       ------> GPIO 14
3.3V      ------> 3.3V
```

**注意事项：**
- RC522 必须使用 3.3V 供电（不能使用5V）
- 如果供电不稳定，增加 0.1uF 滤波电容

### 按钮（内部上拉）

```
ESP32-S3          按钮
---------         --------
GPIO 1    <------ OK按钮一端
GPIO 2    <------ SELECT按钮一端
GND       <------ 两个按钮另一端（共地）
```

**按钮逻辑：**
- 平时：HIGH（内部上拉）
- 按下：LOW（接地）

### 蜂鸣器

```
ESP32-S3          蜂鸣器（有源）
---------         ---------------
GPIO 16   ------> 正极（+）
GND       ------> 负极（-）
```

**推荐：** 5V有源蜂鸣器 + 三极管驱动电路

---

## 🧪 引脚验证测试

### 测试1：LED独立控制

```cpp
void testLEDs() {
  pinMode(LED_POWER, OUTPUT);
  pinMode(LED_NETWORK, OUTPUT);
  pinMode(LED_PROGRESS, OUTPUT);
  pinMode(LED_STATUS, OUTPUT);

  // 依次点亮测试
  digitalWrite(LED_POWER, HIGH);    delay(500);
  digitalWrite(LED_NETWORK, HIGH);  delay(500);
  digitalWrite(LED_PROGRESS, HIGH); delay(500);
  digitalWrite(LED_STATUS, HIGH);   delay(500);

  // 全部熄灭
  digitalWrite(LED_POWER, LOW);
  digitalWrite(LED_NETWORK, LOW);
  digitalWrite(LED_PROGRESS, LOW);
  digitalWrite(LED_STATUS, LOW);
}
```

**预期结果：** 4个LED依次点亮，间隔500ms

### 测试2：脉冲输出

```cpp
void testPulse() {
  pinMode(PULSE_OUTPUT_PIN, OUTPUT);

  for (int i = 0; i < 5; i++) {
    digitalWrite(PULSE_OUTPUT_PIN, HIGH);  // 脉冲开始
    delay(100);                             // 持续100ms
    digitalWrite(PULSE_OUTPUT_PIN, LOW);   // 脉冲结束
    delay(1000);                            // 间隔1000ms
  }
}
```

**预期结果：** GPIO 7 输出5个脉冲，宽度100ms，间隔1000ms

### 测试3：按钮读取

```cpp
void testButtons() {
  pinMode(BTN_OK, INPUT_PULLUP);
  pinMode(BTN_SELECT, INPUT_PULLUP);

  while (1) {
    if (digitalRead(BTN_OK) == LOW) {
      Serial.println("OK按钮被按下");
    }
    if (digitalRead(BTN_SELECT) == LOW) {
      Serial.println("SELECT按钮被按下");
    }
    delay(100);
  }
}
```

**预期结果：** 按下按钮时串口输出提示

---

## ⚠️ 注意事项

### 1. 电源要求

- **ESP32-S3**：5V/2A（推荐）或3.3V/1A
- **RC522模块**：3.3V，电流约80mA
- **OLED显示**：3.3V，电流约20mA
- **LED×4**：每个约20mA，共80mA
- **总计**：建议使用5V/2A电源适配器

### 2. GPIO使用限制

- **GPIO 0**：保留给Boot模式，不使用 ✅
- **GPIO 43/44**：UART0（调试串口），不使用 ✅
- **GPIO 19/20**：USB-JTAG，不使用 ✅
- **GPIO 3-7**：标准GPIO，可安全使用 ✅

### 3. 引脚冲突检查

| 冲突项 | v5.2状态 | v6.0状态 |
|--------|---------|---------|
| GPIO 7（LED vs 脉冲） | ❌ 部分冲突 | ✅ 已解决 |
| I2C地址冲突 | ✅ 无冲突 | ✅ 无冲突 |
| SPI总线冲突 | ✅ 无冲突 | ✅ 无冲突 |

---

## 📝 修改清单（从v5.2升级到v6.0）

### setup() 函数修改

```cpp
void setup() {
  // =================== 新增/修改的pinMode ===================

  // LED系统（v6.0新增L1，调整L2/L4）
  pinMode(LED_POWER, OUTPUT);      // 新增：GPIO 3
  pinMode(LED_NETWORK, OUTPUT);    // 调整：GPIO 6 → GPIO 4
  pinMode(LED_PROGRESS, OUTPUT);   // 保持：GPIO 5
  pinMode(LED_STATUS, OUTPUT);     // 调整：GPIO 7 → GPIO 6

  // 脉冲输出（调整引脚）
  pinMode(PULSE_OUTPUT_PIN, OUTPUT);  // 调整：GPIO 4 → GPIO 7

  // 其他引脚保持不变...
}
```

---

## 🚀 下一步

1. ✅ 引脚配置已优化（本文档）
2. ⏳ 修改 GoldSky_Lite.ino 引脚定义
3. ⏳ 实现LED控制函数（参考WORKFLOW_OPTIMIZATION_PLAN.md）
4. ⏳ 状态机重构（6状态）
5. ⏳ 完整测试

---

## 📚 相关文档

- [WORKFLOW_OPTIMIZATION_PLAN.md](WORKFLOW_OPTIMIZATION_PLAN.md) - 完整优化实施方案
- [OPTIMIZATION_GUIDE.md](OPTIMIZATION_GUIDE.md) - 优化指南
- [QUICK_START.md](QUICK_START.md) - 快速部署指南
- [README.md](README.md) - 项目主文档

---

**文档版本：** v1.0
**创建日期：** 2025-10-30
**维护者：** Eaglson Development Team
**适用版本：** GoldSky_Lite v6.0+

---

**准备实施？** 按照本文档配置硬件，然后参考 WORKFLOW_OPTIMIZATION_PLAN.md 修改代码！ 🎉
