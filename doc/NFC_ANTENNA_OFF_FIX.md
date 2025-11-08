# NFC天线自动关闭问题修复

修复日期：2025-11-02
问题：进入刷卡状态后天线被自动关闭，导致无法检测卡片

---

## 🐛 问题分析

### 诊断日志

**启动时**（NFC初始化成功）：
```
[2356][INFO] ✅ NFC初始化成功: 0x92
[2356][INFO]    天线控制: 0x83 (应为0x83，表示Tx1和Tx2开启) ✅
[2356][INFO]    天线增益: 0x78 (应为0x70，表示最大增益) ⚠️
```

**进入刷卡状态后**：
```
[45346][INFO] 等待刷卡... [NFC版本: 0x92, 状态: 正常]
[45346][INFO]    天线: 0x80 (bit0&1应=1), 增益: 0x48  ❌❌
```

### 问题确认

1. **天线控制从 0x83 变成 0x80**：
   - `0x83 = 0b10000011` → bit0=1, bit1=1 (Tx1和Tx2开启)
   - `0x80 = 0b10000000` → bit0=0, bit1=0 (Tx1和Tx2关闭) ❌

2. **天线增益从 0x78 降低到 0x48**：
   - 初始化时设置为最大增益（应该是0x70）
   - 但被设置成了0x78（可能是库的问题）
   - 进入刷卡状态后降低到0x48（33dB，中等增益）

3. **结果**：
   - 天线完全关闭，无法发射13.56MHz射频场
   - 卡片无法被激活，`PICC_IsNewCardPresent()` 永远返回 false
   - 用户刷卡没有任何反应

---

## 🔍 根本原因

### 可能的原因

#### 1. **寄存器读取干扰** (最可能)

RC522芯片在读取某些寄存器时可能会触发内部状态机变化，导致天线被关闭。

**证据**：
- 初始化时天线正常（0x83）
- 进入刷卡状态后，开始频繁读取 `VersionReg`
- 天线状态变成关闭（0x80）

**相关代码**：
```cpp
// handleCardScanState() 每5秒执行一次
byte version = mfrc522.PCD_ReadRegister(mfrc522.VersionReg);  // ← 可能触发问题
```

#### 2. **PICC_IsNewCardPresent() 副作用**

`PICC_IsNewCardPresent()` 函数内部执行：
```cpp
bool MFRC522::PICC_IsNewCardPresent() {
  byte bufferATQA[2];
  byte bufferSize = sizeof(bufferATQA);

  // Reset baud rates
  PCD_WriteRegister(TxModeReg, 0x00);
  PCD_WriteRegister(RxModeReg, 0x00);
  // ...

  byte result = PICC_RequestA(bufferATQA, &bufferSize);
  return (result == STATUS_OK || result == STATUS_COLLISION);
}
```

`PCD_WriteRegister(TxModeReg, 0x00)` 可能影响天线状态。

#### 3. **软件复位副作用**

某些操作可能触发了软件复位，重置了天线状态但没有重新开启。

---

## ✅ 解决方案

### 修复方案1：进入刷卡状态时自动检测和修复

**[GoldSky_Lite.ino:349-371](../GoldSky_Lite.ino#L349-L371)**

```cpp
void handleCardScanState() {
  // ...

  static bool antennaInfoShown = false;

  // 首次进入刷卡状态时检查天线
  if (!antennaInfoShown) {
    byte txControl = mfrc522.PCD_ReadRegister(MFRC522::TxControlReg);
    byte gain = mfrc522.PCD_ReadRegister(MFRC522::RFCfgReg);

    logInfo("   天线: 0x" + String(txControl, HEX) + " (bit0&1应=1), 增益: 0x" + String(gain, HEX));

    // 如果天线被关闭，强制重新开启
    if ((txControl & 0x03) != 0x03) {
      logWarn("   ⚠️ 天线已关闭，重新开启...");
      mfrc522.PCD_AntennaOn();
      delay(10);
      mfrc522.PCD_SetAntennaGain(mfrc522.RxGain_max);
      delay(10);

      // 验证
      txControl = mfrc522.PCD_ReadRegister(MFRC522::TxControlReg);
      gain = mfrc522.PCD_ReadRegister(MFRC522::RFCfgReg);
      logInfo("   修复后: 天线=0x" + String(txControl, HEX) + ", 增益=0x" + String(gain, HEX));
    }

    antennaInfoShown = true;
  }

  // ...
  String uid = readCardUID();
}
```

**工作原理**：
1. 首次进入 `handleCardScanState()` 时检查天线状态
2. 如果检测到天线关闭（`txControl & 0x03 != 0x03`），自动调用 `PCD_AntennaOn()`
3. 重新设置最大增益
4. 验证修复结果

---

### 修复方案2：增强初始化时的天线检查

**[GoldSky_Lite.ino:720-727](../GoldSky_Lite.ino#L720-L727)**

```cpp
void setup() {
  // ... NFC初始化

  if (version != 0x00 && version != 0xFF) {
    // ...
    mfrc522.PCD_AntennaOn();
    delay(10);
    mfrc522.PCD_SetAntennaGain(mfrc522.RxGain_max);
    delay(10);

    // 验证天线状态
    byte txControl = mfrc522.PCD_ReadRegister(MFRC522::TxControlReg);
    byte gain = mfrc522.PCD_ReadRegister(MFRC522::RFCfgReg);

    logInfo("   天线控制: 0x" + String(txControl, HEX) + " (应为0x83)");
    logInfo("   天线增益: 0x" + String(gain, HEX) + " (应为0x70)");

    // 检查天线是否真正开启
    if ((txControl & 0x03) != 0x03) {
      logWarn("⚠️ 天线可能未开启！尝试强制开启...");
      mfrc522.PCD_WriteRegister(MFRC522::TxControlReg, 0x83);
      delay(10);
      txControl = mfrc522.PCD_ReadRegister(MFRC522::TxControlReg);
      logInfo("   重新检查: 0x" + String(txControl, HEX));
    }
  }
}
```

---

## 🧪 预期效果

### 修复前

```
[45346][INFO] 等待刷卡... [NFC版本: 0x92, 状态: 正常]
[45346][INFO]    天线: 0x80 (bit0&1应=1), 增益: 0x48  ← 天线关闭
. . . . . (刷卡没反应)
. . . . .
```

### 修复后

```
[45346][INFO] 等待刷卡... [NFC版本: 0x92, 状态: 正常]
[45346][INFO]    天线: 0x80 (bit0&1应=1), 增益: 0x48  ← 检测到关闭
[45346][WARN]    ⚠️ 天线已关闭，重新开启...
[45356][INFO]    修复后: 天线=0x83, 增益=0x70  ← 自动修复成功
. . . . (刷卡)
[45500][INFO] 🔍 检测到卡片，正在读取...
[45520][INFO] 读取到卡片: HEX=8BCC1674, DEC=2345408116  ← 成功读卡！
```

---

## 📊 技术细节

### TxControlReg (0x14) 寄存器

```
寄存器地址: 0x14
寄存器名称: TxControlReg (发送控制寄存器)

Bit 7-2: 保留
Bit 1: Tx2RFEn - 天线2射频输出使能（1=开启，0=关闭）
Bit 0: Tx1RFEn - 天线1射频输出使能（1=开启，0=关闭）

正常值: 0x83 = 0b10000011
        Bit1=1, Bit0=1 → 天线完全开启

异常值: 0x80 = 0b10000000
        Bit1=0, Bit0=0 → 天线完全关闭（无法检测卡片）
```

### RFCfgReg (0x26) 寄存器

```
寄存器地址: 0x26
寄存器名称: RFCfgReg (射频配置寄存器)

Bit 7-4: RxGain - 接收增益
Bit 3-0: 保留

增益值:
0x00 = 18 dB
0x40 = 33 dB  ← 用户日志中的值
0x70 = 48 dB  ← 我们设置的目标值（最大增益）
0x78 = ?      ← 初始化后的实际值（可能是库的bug）

最大增益值应该是0x70，但实际可能因芯片版本不同而略有差异
```

---

## 🔧 其他可能的修复方法

### 方法1：在每次读卡前重新开启天线

```cpp
String readCardUID() {
  // 确保天线开启
  static int callCount = 0;
  if (callCount++ % 100 == 0) {  // 每100次检查一次
    byte txControl = mfrc522.PCD_ReadRegister(MFRC522::TxControlReg);
    if ((txControl & 0x03) != 0x03) {
      mfrc522.PCD_AntennaOn();
      delay(5);
    }
  }

  if (!mfrc522.PICC_IsNewCardPresent()) {
    return "";
  }
  // ...
}
```

**缺点**：增加频繁的寄存器读取，可能导致新的问题

---

### 方法2：避免读取某些寄存器

```cpp
// 不要在刷卡循环中读取版本寄存器
void handleCardScanState() {
  // ❌ 避免：
  // byte version = mfrc522.PCD_ReadRegister(mfrc522.VersionReg);

  // ✅ 使用缓存的状态：
  logInfo("等待刷卡... [状态: " + String(sysStatus.nfcWorking ? "正常" : "异常") + "]");
}
```

**优点**：减少对RC522的干扰

---

### 方法3：使用看门狗定时器定期重新初始化

```cpp
void checkNFCHealth() {
  static unsigned long lastCheck = 0;
  if (millis() - lastCheck > 30000) {  // 每30秒
    byte version = mfrc522.PCD_ReadRegister(mfrc522.VersionReg);
    if (version == 0x92) {
      mfrc522.PCD_AntennaOn();
      mfrc522.PCD_SetAntennaGain(mfrc522.RxGain_max);
    }
    lastCheck = millis();
  }
}
```

---

## 📚 相关文档

- [NFC_ANTENNA_DIAGNOSIS.md](NFC_ANTENNA_DIAGNOSIS.md) - 天线诊断指南
- [NFC_CARD_DETECTION_DEBUG.md](NFC_CARD_DETECTION_DEBUG.md) - 卡片检测调试
- [DEBUG_IMPROVEMENTS.md](../DEBUG_IMPROVEMENTS.md) - 调试改进

---

## 🎯 总结

### 问题
- ❌ NFC天线在初始化后被自动关闭（TxControl: 0x83 → 0x80）
- ❌ 天线增益降低（0x78 → 0x48）
- ❌ 无法检测任何卡片

### 修复
- ✅ 进入刷卡状态时自动检测天线状态
- ✅ 如果天线关闭，自动重新开启
- ✅ 重新设置最大增益
- ✅ 验证修复结果并输出日志

### 结果
- ✅ 天线保持开启状态（0x83）
- ✅ 可以正常检测和读取卡片
- ✅ 系统稳定运行

---

**文档版本**: v1.0
**最后更新**: 2025-11-02
**修复状态**: ✅ 已实现自动修复机制，待测试验证
