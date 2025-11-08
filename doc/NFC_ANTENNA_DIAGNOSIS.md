# NFC天线诊断指南

创建日期：2025-11-02
用途：诊断RC522天线问题导致无法检测卡片

---

## 📋 问题描述

**症状**：
- NFC模块初始化成功（版本0x92）
- 进入刷卡状态正常
- 但无法检测到任何卡片
- 没有"🔍 检测到卡片"日志

**最可能原因**：天线未正确开启或增益太低

---

## 🔍 新增诊断功能

### 1. 启动时天线状态检查

**[GoldSky_Lite.ino:702-717](../GoldSky_Lite.ino#L702-L717)**

```cpp
// 验证天线状态
byte txControl = mfrc522.PCD_ReadRegister(0x14);  // TxControlReg
byte gain = mfrc522.PCD_ReadRegister(0x26);       // RFCfgReg (增益)

logInfo("✅ NFC初始化成功: 0x" + String(version, HEX));
logInfo("   天线控制: 0x" + String(txControl, HEX) + " (应为0x83，表示Tx1和Tx2开启)");
logInfo("   天线增益: 0x" + String(gain, HEX) + " (应为0x70，表示最大增益)");

// 检查天线是否真正开启
if ((txControl & 0x03) != 0x03) {
  logWarn("⚠️ 天线可能未开启！尝试强制开启...");
  mfrc522.PCD_WriteRegister(0x14, 0x83);  // 强制开启Tx1和Tx2
  delay(10);
  txControl = mfrc522.PCD_ReadRegister(0x14);
  logInfo("   重新检查: 0x" + String(txControl, HEX));
}
```

---

### 2. 刷卡时天线状态检查

**[GoldSky_Lite.ino:341-353](../GoldSky_Lite.ino#L341-L353)**

```cpp
static unsigned long lastDebugPrint = 0;
static bool firstCheck = true;
if (millis() - lastDebugPrint > 5000 || firstCheck) {
  byte version = mfrc522.PCD_ReadRegister(mfrc522.VersionReg);
  byte txControl = mfrc522.PCD_ReadRegister(0x14);  // TxControlReg
  byte gain = mfrc522.PCD_ReadRegister(0x26);       // RFCfgReg

  logInfo("等待刷卡... [NFC版本: 0x" + String(version, HEX) + ", 状态: " +
          (sysStatus.nfcWorking ? "正常" : "异常") + "]");

  if (firstCheck) {
    logInfo("   天线: 0x" + String(txControl, HEX) + " (bit0&1应=1), 增益: 0x" + String(gain, HEX));
    firstCheck = false;
  }

  lastDebugPrint = millis();
}
```

---

## 🧪 预期的串口输出

### 正常情况（天线工作）

```
[INFO] 🔌 初始化SPI和NFC...
[INFO] ✅ NFC初始化成功: 0x92
[INFO]    天线控制: 0x83 (应为0x83，表示Tx1和Tx2开启)
[INFO]    天线增益: 0x70 (应为0x70，表示最大增益)

... 用户选择套餐 ...

[INFO] 等待刷卡... [NFC版本: 0x92, 状态: 正常]
[INFO]    天线: 0x83 (bit0&1应=1), 增益: 0x70
. . . .（用户刷卡）
[INFO] 🔍 检测到卡片，正在读取...
[INFO] 读取到卡片: HEX=8BCC1674, DEC=2345408116
```

**解读**：
- ✅ `TxControl = 0x83` → Tx1和Tx2都开启（bit0和bit1都是1）
- ✅ `增益 = 0x70` → 最大增益（48dB）
- ✅ 可以正常检测卡片

---

### 异常情况1：天线未开启

```
[INFO] 🔌 初始化SPI和NFC...
[INFO] ✅ NFC初始化成功: 0x92
[INFO]    天线控制: 0x80 (应为0x83，表示Tx1和Tx2开启)
[INFO]    天线增益: 0x70 (应为0x70，表示最大增益)
[WARN] ⚠️ 天线可能未开启！尝试强制开启...
[INFO]    重新检查: 0x83

... 用户选择套餐 ...

[INFO] 等待刷卡... [NFC版本: 0x92, 状态: 正常]
[INFO]    天线: 0x83 (bit0&1应=1), 增益: 0x70
. . . .（用户刷卡）
[INFO] 🔍 检测到卡片，正在读取...
```

**解读**：
- ❌ 初始 `TxControl = 0x80` → Tx1和Tx2未开启（bit0和bit1是0）
- ✅ 强制开启后变成 `0x83` → 修复成功
- ✅ 现在可以检测卡片

**原因**：`PCD_AntennaOn()` 函数未正确执行或被覆盖

---

### 异常情况2：天线始终无法开启

```
[INFO] 🔌 初始化SPI和NFC...
[INFO] ✅ NFC初始化成功: 0x92
[INFO]    天线控制: 0x80 (应为0x83，表示Tx1和Tx2开启)
[INFO]    天线增益: 0x70 (应为0x70，表示最大增益)
[WARN] ⚠️ 天线可能未开启！尝试强制开启...
[INFO]    重新检查: 0x80  ← 仍然是0x80，未成功

... 用户选择套餐 ...

[INFO] 等待刷卡... [NFC版本: 0x92, 状态: 正常]
[INFO]    天线: 0x80 (bit0&1应=1), 增益: 0x70
. . . .（用户刷卡，但没有任何反应）
. . . .
```

**解读**：
- ❌ `TxControl` 始终是 `0x80`，无法改为 `0x83`
- ❌ 无法检测卡片

**可能原因**：
1. **硬件问题**：RC522模块天线损坏或焊接问题
2. **供电问题**：电压不足（需要稳定的3.3V）
3. **模块故障**：RC522芯片本身故障

**解决方法**：
- 测量RC522的VCC电压，确保在3.2V-3.4V之间
- 更换RC522模块
- 检查RC522模块的天线焊接

---

### 异常情况3：增益设置失败

```
[INFO] 🔌 初始化SPI和NFC...
[INFO] ✅ NFC初始化成功: 0x92
[INFO]    天线控制: 0x83 (应为0x83，表示Tx1和Tx2开启)
[INFO]    天线增益: 0x40 (应为0x70，表示最大增益)  ← 增益不对

... 用户选择套餐 ...

[INFO] 等待刷卡... [NFC版本: 0x92, 状态: 正常]
[INFO]    天线: 0x83 (bit0&1应=1), 增益: 0x40
. . . .（刷卡，可能距离很近才能检测到）
```

**解读**：
- ✅ 天线已开启（0x83）
- ⚠️ 增益只有0x40（33dB），应该是0x70（48dB）

**影响**：
- 检测距离变短（可能需要贴很近才能读到）
- 对某些卡片可能完全检测不到

**可能原因**：
- `PCD_SetAntennaGain()` 函数调用失败
- 寄存器写入被后续操作覆盖

---

## 📊 寄存器详解

### TxControlReg (0x14) - 天线控制寄存器

```
bit 7-2: 保留
bit 1: Tx2RFEn - 天线2输出使能（1=开启）
bit 0: Tx1RFEn - 天线1输出使能（1=开启）

正常值: 0x83 = 0b10000011
        └─┬─┘     └┬┘ └┬──┘
          │        │   └─ bit1=1, bit0=1 (天线开启)
          └────────┴─────── bit7=1 (固定)

异常值: 0x80 = 0b10000000 (天线关闭)
```

---

### RFCfgReg (0x26) - 射频配置寄存器（增益）

```
bit 7-4: RxGain - 接收增益
bit 3-0: 保留

增益值:
0x00 = 18 dB (最小)
0x10 = 23 dB
0x20 = 18 dB
0x30 = 23 dB
0x40 = 33 dB
0x50 = 38 dB
0x60 = 43 dB
0x70 = 48 dB (最大) ← 我们设置的值

正常值: 0x70 (最大增益)
```

---

## 🔧 手动测试

如果自动诊断无法定位问题，可以在setup()中添加手动测试代码：

```cpp
void testNFCAntenna() {
  logInfo("=== NFC天线手动测试 ===");

  // 1. 读取初始状态
  byte txControl = mfrc522.PCD_ReadRegister(0x14);
  byte gain = mfrc522.PCD_ReadRegister(0x26);
  logInfo("初始状态: TxControl=0x" + String(txControl, HEX) + ", Gain=0x" + String(gain, HEX));

  // 2. 尝试写入
  mfrc522.PCD_WriteRegister(0x14, 0x83);
  delay(10);
  mfrc522.PCD_WriteRegister(0x26, 0x70);
  delay(10);

  // 3. 读取写入后的状态
  txControl = mfrc522.PCD_ReadRegister(0x14);
  gain = mfrc522.PCD_ReadRegister(0x26);
  logInfo("写入后: TxControl=0x" + String(txControl, HEX) + ", Gain=0x" + String(gain, HEX));

  if (txControl == 0x83 && gain == 0x70) {
    logInfo("✅ 天线配置成功");
  } else {
    logError("❌ 天线配置失败，可能是硬件问题");
  }
}

void setup() {
  // ... 现有初始化代码

  // NFC初始化后调用
  if (sysStatus.nfcWorking) {
    testNFCAntenna();
  }

  // ... 其余代码
}
```

---

## 🛠️ 硬件检查清单

如果软件诊断显示天线无法开启，需要检查硬件：

### 1. 电源电压测量

使用万用表测量RC522模块：
- **VCC到GND**: 应为 **3.2V - 3.4V**
- 如果低于3.0V → 供电不足
- 如果高于3.5V → 可能损坏芯片

**解决方法**：
- 在RC522的VCC和GND之间加100μF电容
- 使用独立的3.3V LDO稳压器供电

---

### 2. SPI通信测试

```cpp
// 测试SPI读写是否正常
void testSPI() {
  logInfo("=== SPI通信测试 ===");

  // 写入一个值到测试寄存器（CommandReg）
  mfrc522.PCD_WriteRegister(0x01, 0x00);
  delay(10);
  byte read1 = mfrc522.PCD_ReadRegister(0x01);

  mfrc522.PCD_WriteRegister(0x01, 0x0C);
  delay(10);
  byte read2 = mfrc522.PCD_ReadRegister(0x01);

  logInfo("写入0x00，读取: 0x" + String(read1, HEX));
  logInfo("写入0x0C，读取: 0x" + String(read2, HEX));

  if (read1 == 0x00 && read2 == 0x0C) {
    logInfo("✅ SPI通信正常");
  } else {
    logError("❌ SPI通信异常");
  }
}
```

---

### 3. 天线物理检查

检查RC522模块板载天线：
- 是否有明显的裂痕或断裂？
- 天线走线是否完整？
- 焊接点是否牢固？

如果天线损坏，需要更换模块。

---

### 4. 测试卡片

确保使用的是正确类型的卡片：
- ✅ **13.56MHz** Mifare Classic 1K/4K
- ✅ **13.56MHz** Mifare Ultralight
- ✅ **13.56MHz** NTAG213/215/216
- ❌ **125kHz** EM4100/EM4102（RC522不支持）

**检查方法**：
- 查看卡片背面标注
- 使用手机NFC功能测试（如果能读到说明是13.56MHz）

---

## 📚 相关文档

- [NFC_CARD_DETECTION_DEBUG.md](NFC_CARD_DETECTION_DEBUG.md) - NFC卡片检测调试
- [NFC_INIT_IMPROVEMENT.md](NFC_INIT_IMPROVEMENT.md) - NFC初始化改进
- [config.h](../config.h) - NFC引脚配置

---

## 🎯 下一步

1. **上传新代码** - 包含天线诊断功能
2. **查看启动日志** - 重点关注：
   ```
   [INFO]    天线控制: 0x?? (应为0x83)
   [INFO]    天线增益: 0x?? (应为0x70)
   ```
3. **进入刷卡状态** - 查看：
   ```
   [INFO]    天线: 0x?? (bit0&1应=1), 增益: 0x??
   ```
4. **根据输出判断**：
   - 如果天线值正常（0x83）但仍检测不到卡 → 可能是卡片类型不对或模块硬件问题
   - 如果天线值异常（0x80）→ 天线未开启，需要进一步诊断

---

**文档版本**: v1.0
**最后更新**: 2025-11-02
**状态**: 待测试验证
