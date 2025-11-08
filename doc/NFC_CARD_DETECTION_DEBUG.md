# NFC卡片检测调试指南

修复日期：2025-11-02
问题：系统流程正常，但NFC无法检测到卡片

---

## 🐛 问题描述

### 用户反馈

```
还是无法检测到卡，流程正常

22:38:45.239 -> 🚗 Eaglson Coin Wash Terminal v2.4
22:39:29.044 -> [44804][INFO] 按钮按下: GPIO2
22:39:30.760 -> [46518][INFO] 按钮按下: GPIO2
22:39:35.097 -> [50868][INFO] 按钮按下: GPIO1
22:39:35.221 -> [50969][INFO] 套餐选择: 8 Min Wash
22:39:35.267 -> [51019][INFO] 等待刷卡...
22:39:35.314 -> ..[53043][INFO] 等待刷卡...
22:39:37.343 -> ..[55067][INFO] 等待刷卡...
```

### 症状分析

✅ **正常的部分**：
- 系统启动成功
- 按钮响应正常
- 套餐选择正常
- 进入刷卡状态（STATE_CARD_SCAN）
- 显示"等待刷卡..."日志
- 心跳点（`.`）显示正常

❌ **异常的部分**：
- 刷卡时没有任何反应
- 没有"🔍 检测到卡片"日志
- `PICC_IsNewCardPresent()` 一直返回 false

---

## 🔍 可能的原因

### 1. 硬件问题

#### SPI接线检查

| 信号 | ESP32-S3 引脚 | RC522引脚 | 检查项 |
|------|--------------|-----------|--------|
| MOSI | GPIO 11 | MOSI | 是否接触良好？ |
| MISO | GPIO 13 | MISO | 是否接触良好？ |
| SCK  | GPIO 12 | SCK  | 是否接触良好？ |
| CS   | GPIO 10 | SDA  | 是否接触良好？ |
| RST  | GPIO 14 | RST  | **必须连接** |
| 3.3V | 3.3V | VCC | 电压是否稳定？ |
| GND  | GND | GND | 是否良好接地？ |

**检查方法**：
```cpp
// 在setup()中临时添加引脚检查
logInfo("SPI_MOSI = " + String(SPI_MOSI));
logInfo("SPI_MISO = " + String(SPI_MISO));
logInfo("SPI_SCK = " + String(SPI_SCK));
logInfo("RC522_CS = " + String(RC522_CS));
logInfo("RC522_RST = " + String(RC522_RST));
```

#### 电源检查

RC522模块对电源质量要求较高：
- 使用万用表测量RC522的VCC和GND之间电压
- 应该稳定在 **3.2V - 3.4V**
- 如果电压波动较大，需要：
  - 在RC522的VCC和GND之间加 **100μF电容** 和 **0.1μF陶瓷电容** 并联
  - 使用独立的3.3V电源供电（不要和其他模块共用）

#### RST引脚检查

**重要**：RST引脚必须正确连接，否则无法检测卡片

检查方法：
```cpp
// 在setup()中临时添加
pinMode(RC522_RST, OUTPUT);
digitalWrite(RC522_RST, HIGH);
delay(10);
digitalWrite(RC522_RST, LOW);
delay(10);
digitalWrite(RC522_RST, HIGH);
delay(10);
// 用万用表测量RST引脚电压，应该是3.3V
```

---

### 2. 软件问题

#### 问题1：天线未开启或增益太低

**症状**：模块初始化成功（版本0x92），但检测不到卡片

**检查**：
```cpp
// GoldSky_Lite.ino setup() 中应该有：
mfrc522.PCD_AntennaOn();        // 开启天线
delay(10);
mfrc522.PCD_SetAntennaGain(mfrc522.RxGain_max);  // 最大增益
```

**验证方法**：
```cpp
// 在setup()初始化NFC后临时添加
byte gain = mfrc522.PCD_GetAntennaGain();
logInfo("天线增益: 0x" + String(gain, HEX));
// 应该输出: 天线增益: 0x70 (最大增益)
```

#### 问题2：寄存器配置被破坏

频繁读取版本寄存器可能干扰RC522的内部状态

**当前优化**：
- 健康检查只在失败5次或60秒后触发
- 正常读卡时不读取寄存器

**验证**：查看是否有"⚠️ NFC模块异常"日志
- 如果有 → 模块状态异常，需要硬件检查
- 如果没有 → 模块状态正常，问题在卡片检测逻辑

#### 问题3：卡片类型不兼容

RC522支持的卡片类型：
- ✅ Mifare Classic 1K/4K
- ✅ Mifare Ultralight
- ✅ NTAG213/215/216
- ❌ Mifare DESFire（部分支持）
- ❌ 125kHz卡片（完全不支持）

**检查卡片**：
- 查看卡片背面是否标注 "Mifare" 或 "13.56MHz"
- 如果是125kHz卡片（如EM4100），RC522无法识别

---

### 3. 时序问题

#### 问题：I2C显示刷新干扰SPI通信

**症状**：
- OLED显示正常
- 但NFC无法读卡

**原因**：
- OLED使用I2C（GPIO4=SDA, GPIO5=SCL）
- NFC使用SPI（GPIO11,12,13）
- 虽然总线不同，但如果I2C刷新过于频繁，可能导致：
  - CPU时间片不足
  - 中断冲突
  - 电源噪声

**已优化**：
- 显示刷新率限制到2次/秒（500ms一次）
- 只在动画帧变化时刷新

---

## ✅ 调试改进

### 1. 增强的调试日志

**[GoldSky_Lite.ino:338-345](../GoldSky_Lite.ino#L338-L345)**

```cpp
void handleCardScanState() {
  setSystemLEDStatus();

  // 调试：每5秒显示NFC模块状态
  static unsigned long lastDebugPrint = 0;
  if (millis() - lastDebugPrint > 5000) {
    byte version = mfrc522.PCD_ReadRegister(mfrc522.VersionReg);
    logInfo("等待刷卡... [NFC版本: 0x" + String(version, HEX) + ", 状态: " +
            (sysStatus.nfcWorking ? "正常" : "异常") + "]");
    lastDebugPrint = millis();
  }

  displayCardScan();
  String uid = readCardUID();
  // ...
}
```

**预期日志**：
```
[INFO] 等待刷卡... [NFC版本: 0x92, 状态: 正常]
. . . . .
[INFO] 等待刷卡... [NFC版本: 0x92, 状态: 正常]
```

---

### 2. 卡片检测详细日志

**[GoldSky_Utils.ino:253-260](../GoldSky_Utils.ino#L253-L260)**

```cpp
String readCardUID() {
  // ... 健康检查

  if (!mfrc522.PICC_IsNewCardPresent()) {
    return "";  // 没有卡片，静默返回
  }

  // 调试：检测到卡片存在
  logInfo("🔍 检测到卡片，正在读取...");

  if (!mfrc522.PICC_ReadCardSerial()) {
    nfcReadFailCount++;
    logError("❌ 读取卡片序列号失败");
    return "";
  }

  // ... 成功读取
  logInfo("读取到卡片: HEX=" + hexUID + ", DEC=" + decimalUID);
  return decimalUID;
}
```

**预期日志（正常刷卡）**：
```
. . . .
[INFO] 🔍 检测到卡片，正在读取...
[INFO] 读取到卡片: HEX=8BCC1674, DEC=2345408116
[INFO] ✅ 在线验证成功
```

**预期日志（检测到但读取失败）**：
```
. . . .
[INFO] 🔍 检测到卡片，正在读取...
[ERROR] ❌ 读取卡片序列号失败
. . . .
```

---

### 3. 健康检查时间戳初始化

**[GoldSky_Lite.ino:749-752](../GoldSky_Lite.ino#L749-L752)**

```cpp
void setup() {
  // ... 初始化代码

  logInfo("系统已就绪！");
  lastHeartbeat = millis();
  lastSuccessfulOperation = millis();

  // 重要：初始化NFC健康检查时间戳，避免首次读卡时触发不必要的健康检查
  extern unsigned long lastNFCHealthCheck;
  lastNFCHealthCheck = millis();
}
```

**作用**：
- 避免系统启动后60秒时意外触发健康检查
- 保证首次读卡时不会读取版本寄存器干扰检测

---

## 🧪 测试步骤

### 步骤1：上传新代码

上传包含以上调试改进的代码

### 步骤2：查看启动日志

```
[INFO] ✅ NFC初始化成功: 0x92
[INFO] 系统已就绪！
```

**检查**：
- ✅ NFC版本应该是 `0x92` 或 `0x91`
- ❌ 如果是 `0x00` 或 `0xFF` → 硬件连接问题

---

### 步骤3：进入刷卡状态

按按钮选择套餐，进入刷卡状态

**预期日志**：
```
[INFO] 按钮按下: GPIO1
[INFO] 套餐选择: 8 Min Wash
[INFO] 等待刷卡... [NFC版本: 0x92, 状态: 正常]
. . . . .
[INFO] 等待刷卡... [NFC版本: 0x92, 状态: 正常]
```

**检查版本号**：
- ✅ 如果是 `0x92` → 模块正常，继续步骤4
- ❌ 如果变成 `0x00` 或 `0xFF` → 模块异常，检查硬件

---

### 步骤4：刷卡测试

**情况A - 完全没反应（当前问题）**：
```
[INFO] 等待刷卡... [NFC版本: 0x92, 状态: 正常]
. . . . .（用户刷卡，但没有任何反应）
. . . . .
```

**可能原因**：
1. 卡片类型不兼容（非13.56MHz Mifare卡）
2. 天线未开启或增益太低
3. 硬件问题（SPI接线、电源、天线）
4. 卡片与模块距离太远（应贴近，<3cm）

**排查方法**：
```cpp
// 临时测试代码（添加到handleCardScanState()）
static unsigned long lastAntennaCheck = 0;
if (millis() - lastAntennaCheck > 10000) {
  // 读取天线状态
  byte reg26 = mfrc522.PCD_ReadRegister(0x26);  // TxControlReg
  logInfo("天线寄存器: 0x" + String(reg26, HEX) +
          " (应该是0x83，bit1和bit0=1表示天线开启)");
  lastAntennaCheck = millis();
}
```

---

**情况B - 检测到但读取失败**：
```
[INFO] 等待刷卡... [NFC版本: 0x92, 状态: 正常]
. . . .
[INFO] 🔍 检测到卡片，正在读取...
[ERROR] ❌ 读取卡片序列号失败
. . . .
```

**可能原因**：
1. 卡片通信时序问题
2. 电源不稳定导致读取中断
3. SPI通信错误

**排查方法**：
- 检查电源电压
- 在VCC和GND间加电容
- 降低SPI速度：
  ```cpp
  // config.h
  #define SPI_SPEED 1000000  // 从4MHz降到1MHz
  ```

---

**情况C - 成功读取（期望结果）**：
```
[INFO] 等待刷卡... [NFC版本: 0x92, 状态: 正常]
. . . .
[INFO] 🔍 检测到卡片，正在读取...
[INFO] 读取到卡片: HEX=8BCC1674, DEC=2345408116
[INFO] ✅ 在线验证成功
```

**恭喜**：NFC工作正常！

---

## 🔧 高级诊断

### 测试1：手动触发卡片检测

在 `handleCardScanState()` 中临时添加：

```cpp
static unsigned long lastManualTest = 0;
if (millis() - lastManualTest > 3000) {
  // 强制读取FIFO和状态寄存器
  byte fifoLevel = mfrc522.PCD_ReadRegister(0x0A);  // FIFOLevelReg
  byte comIrq = mfrc522.PCD_ReadRegister(0x04);     // ComIrqReg
  byte errorReg = mfrc522.PCD_ReadRegister(0x06);   // ErrorReg

  Serial.printf("[DEBUG] FIFO=%02X ComIrq=%02X Error=%02X\n",
                fifoLevel, comIrq, errorReg);
  lastManualTest = millis();
}
```

**正常值**：
- FIFO: 接近0x00（空）
- ComIrq: 变化（表示有通信活动）
- Error: 0x00（无错误）

**异常值**：
- Error != 0x00 → 检查具体错误位
- FIFO一直是0x00且ComIrq不变 → 天线未工作

---

### 测试2：天线自检

```cpp
// 在setup()初始化NFC后添加
void testNFCAntenna() {
  logInfo("=== NFC天线自检 ===");

  // 1. 检查天线控制寄存器
  byte txControl = mfrc522.PCD_ReadRegister(0x14);  // TxControlReg
  logInfo("TxControl: 0x" + String(txControl, HEX) +
          " (bit1&bit0应该=1)");

  // 2. 开启自检
  mfrc522.PCD_WriteRegister(0x01, 0x09);  // CommandReg = SelfTest
  delay(100);

  // 3. 读取FIFO数据
  byte buffer[64];
  for (byte i = 0; i < 64; i++) {
    buffer[i] = mfrc522.PCD_ReadRegister(0x09);  // FIFODataReg
  }

  // 4. 检查是否全0（失败）或有数据（成功）
  bool allZero = true;
  for (byte i = 0; i < 64; i++) {
    if (buffer[i] != 0x00) {
      allZero = false;
      break;
    }
  }

  if (allZero) {
    logError("❌ 天线自检失败（无数据）");
  } else {
    logInfo("✅ 天线自检通过");
  }

  // 5. 退出自检，重新初始化
  mfrc522.PCD_Init(RC522_CS, RC522_RST);
  mfrc522.PCD_AntennaOn();
  mfrc522.PCD_SetAntennaGain(mfrc522.RxGain_max);
}
```

---

## 📊 常见问题对照表

| 症状 | 可能原因 | 解决方法 |
|------|---------|---------|
| 版本号0x00或0xFF | SPI连接问题 | 检查MOSI/MISO/SCK/CS接线 |
| 版本号0x92但不读卡 | 天线未开启 | 确认调用PCD_AntennaOn() |
| 检测到但读取失败 | 电源不稳定 | 加电容，检查3.3V电压 |
| 完全无反应 | 卡片类型不对 | 使用13.56MHz Mifare卡 |
| 间歇性失败 | I2C/SPI冲突 | 降低显示刷新率 |
| 版本号变成0x00 | 模块复位或死机 | 检查RST引脚，加电容 |

---

## 📚 相关文档

- [NFC_INIT_IMPROVEMENT.md](NFC_INIT_IMPROVEMENT.md) - NFC初始化改进
- [NFC_READ_FIX.md](../NFC_READ_FIX.md) - NFC读卡修复
- [DEBUG_IMPROVEMENTS.md](../DEBUG_IMPROVEMENTS.md) - 调试改进
- [config.h](../config.h) - NFC引脚配置

---

## 🎯 下一步

1. **上传新代码** - 包含增强的调试日志
2. **查看启动日志** - 确认NFC初始化成功（0x92）
3. **进入刷卡状态** - 查看NFC版本号是否保持0x92
4. **刷卡测试** - 根据日志判断问题类型
5. **提供反馈** - 复制完整的串口日志

**关键日志**：
- NFC初始化日志
- "等待刷卡..."日志（包含版本号）
- 是否出现"🔍 检测到卡片"日志
- 是否有错误日志

---

**文档版本**: v1.0
**最后更新**: 2025-11-02
**状态**: 待测试验证
