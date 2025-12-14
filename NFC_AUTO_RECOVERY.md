# NFC自动恢复机制 - 解决刷卡无响应问题

## 🎯 问题诊断

根据健康度监测数据分析：

### 问题现象
- **时间点**：设备重启后运行24-48小时
- **症状**：刷卡完全无响应，但OLED显示正常
- **健康度数据**：
  - NFC成功率从 100% → 11-24%
  - NFC失败次数累积到 28次
  - NFC成功次数仅 9次
  - NFC初始化标志 = true（但实际通信失效）

### 根本原因
**NFC模块初始化标志为true，但SPI通信已经失效**

可能的触发因素：
1. **SPI总线干扰**：WiFi重连、OLED频繁刷新可能干扰SPI通信
2. **NFC模块进入异常状态**：长时间空闲后固件进入低功耗/挂死状态
3. **电磁干扰**：环境干扰导致NFC芯片寄存器值异常
4. **初始化不完整**：仅检查版本号，未验证实际读卡能力

## 🛠️ 解决方案

### 方案1：增强NFC健康检查（推荐）⭐

**核心思路**：不仅检查版本号，还要验证实际读卡能力

#### 修改位置：GoldSky_Lite.ino

在 `tryRecoverNFC()` 函数之前添加新函数：

```cpp
// =================== NFC健康检查（增强版）===================
bool verifyNFCHealth() {
  // 第1步：检查版本寄存器
  byte version = mfrc522.PCD_ReadRegister(mfrc522.VersionReg);
  if (version == 0x00 || version == 0xFF) {
    logWarn("⚠️ NFC版本号异常: 0x" + String(version, HEX));
    return false;
  }

  // 第2步：检查天线状态
  byte antennaStatus = mfrc522.PCD_ReadRegister(mfrc522.TxControlReg);
  if ((antennaStatus & 0x03) != 0x03) {
    logWarn("⚠️ NFC天线未开启，尝试重新开启");
    mfrc522.PCD_AntennaOn();
    delay(50);
    antennaStatus = mfrc522.PCD_ReadRegister(mfrc522.TxControlReg);
    if ((antennaStatus & 0x03) != 0x03) {
      logWarn("❌ NFC天线开启失败");
      return false;
    }
  }

  // 第3步：验证寄存器读写功能
  // 写入测试值到CommandReg
  byte testValue = 0x20;  // Idle command
  mfrc522.PCD_WriteRegister(mfrc522.CommandReg, testValue);
  delay(10);
  byte readBack = mfrc522.PCD_ReadRegister(mfrc522.CommandReg);
  if (readBack != testValue) {
    logWarn("❌ NFC寄存器读写验证失败 (写入: 0x" + String(testValue, HEX) +
            ", 读回: 0x" + String(readBack, HEX) + ")");
    return false;
  }

  // 第4步：检查关键寄存器值
  byte errorReg = mfrc522.PCD_ReadRegister(mfrc522.ErrorReg);
  if (errorReg != 0x00) {
    logWarn("⚠️ NFC错误寄存器异常: 0x" + String(errorReg, HEX));
    // 清除错误标志
    mfrc522.PCD_WriteRegister(mfrc522.ErrorReg, 0x00);
  }

  logDebug("✅ NFC健康检查通过 (版本: 0x" + String(version, HEX) +
           ", 天线: ON, 寄存器: OK)");
  return true;
}

// =================== NFC完全重置 ===================
bool performNFCHardReset() {
  logInfo("🔄 执行NFC硬件完全重置...");

  // 硬件复位
  digitalWrite(RC522_RST, LOW);
  delay(100);  // 延长复位时间
  digitalWrite(RC522_RST, HIGH);
  delay(100);

  // 重新初始化
  mfrc522.PCD_Init(RC522_CS, RC522_RST);
  delay(200);

  // 配置最佳参数
  mfrc522.PCD_AntennaOn();
  mfrc522.PCD_SetAntennaGain(mfrc522.RxGain_max);

  // 清除所有错误标志
  mfrc522.PCD_WriteRegister(mfrc522.ErrorReg, 0x00);

  // 验证健康状态
  return verifyNFCHealth();
}
```

#### 修改 `tryRecoverNFC()` 函数

```cpp
// =================== NFC自动恢复（增强版）===================
void tryRecoverNFC() {
  if (sysStatus.nfcWorking) {
    // 即使标志为true，也定期验证健康度
    static unsigned long lastHealthCheck = 0;
    if (millis() - lastHealthCheck >= 300000) {  // 每5分钟检查一次
      lastHealthCheck = millis();

      if (!verifyNFCHealth()) {
        logWarn("⚠️ NFC健康检查失败，尝试恢复...");
        sysStatus.nfcWorking = false;  // 标记为失效，触发恢复流程
        healthMetrics.nfcInitialized = false;
      }
    }
    return;
  }

  if (millis() - lastNFCRetry < NFC_RETRY_INTERVAL_MS) return;

  lastNFCRetry = millis();

  // 使用增强版硬件重置
  if (performNFCHardReset()) {
    sysStatus.nfcWorking = true;
    healthMetrics.nfcInitialized = true;
    logInfo("✅ NFC恢复成功");
    beepSuccess();
  } else {
    logWarn("⚠️ NFC恢复失败，将在" + String(NFC_RETRY_INTERVAL_MS/1000) + "秒后重试");
    healthMetrics.nfcInitialized = false;
  }
}
```

### 方案2：基于成功率的自动恢复

在 `loop()` 中添加成功率监控：

```cpp
// =================== 主循环（在健康度监测之后）===================
void loop() {
  // ... 现有代码 ...

  // 健康度监测
  healthMonitor.checkAndUpload();

  // 新增：基于成功率的NFC自动恢复
  static unsigned long lastNFCSuccessRateCheck = 0;
  if (millis() - lastNFCSuccessRateCheck >= 600000) {  // 每10分钟检查一次
    lastNFCSuccessRateCheck = millis();

    float successRate = healthMetrics.getNFCSuccessRate();
    int totalReads = healthMetrics.nfcReadSuccessCount + healthMetrics.nfcReadFailCount;

    // 如果有足够的样本数据，且成功率低于50%
    if (totalReads >= 10 && successRate < 50.0 && sysStatus.nfcWorking) {
      logWarn("⚠️ NFC成功率过低 (" + String(successRate, 1) + "%)，触发自动恢复");
      sysStatus.nfcWorking = false;  // 触发恢复流程

      // 重置统计数据
      healthMetrics.nfcReadSuccessCount = 0;
      healthMetrics.nfcReadFailCount = 0;
    }
  }

  // ... 现有代码 ...
}
```

### 方案3：增加串口命令用于手动诊断

在串口命令处理中添加：

```cpp
else if (cmd == "nfc test") {
  Serial.println("🔍 NFC健康诊断测试...");

  byte version = mfrc522.PCD_ReadRegister(mfrc522.VersionReg);
  Serial.println("   版本寄存器: 0x" + String(version, HEX));

  byte antenna = mfrc522.PCD_ReadRegister(mfrc522.TxControlReg);
  Serial.println("   天线状态: 0x" + String(antenna, HEX) + " (" +
                 String((antenna & 0x03) == 0x03 ? "ON" : "OFF") + ")");

  byte errorReg = mfrc522.PCD_ReadRegister(mfrc522.ErrorReg);
  Serial.println("   错误寄存器: 0x" + String(errorReg, HEX));

  byte gainReg = mfrc522.PCD_ReadRegister(mfrc522.RFCfgReg);
  Serial.println("   增益配置: 0x" + String(gainReg, HEX));

  if (verifyNFCHealth()) {
    Serial.println("✅ NFC模块健康");
  } else {
    Serial.println("❌ NFC模块异常");
  }
}
else if (cmd == "nfc reset") {
  Serial.println("🔄 手动重置NFC模块...");
  if (performNFCHardReset()) {
    sysStatus.nfcWorking = true;
    healthMetrics.nfcInitialized = true;
    Serial.println("✅ NFC重置成功");
  } else {
    Serial.println("❌ NFC重置失败");
  }
}
```

## 📊 实施步骤

### 步骤1：添加新代码
1. 将 `verifyNFCHealth()` 函数添加到 `GoldSky_Lite.ino` 第169行之前
2. 将 `performNFCHardReset()` 函数添加到 `tryRecoverNFC()` 之前
3. 替换 `tryRecoverNFC()` 函数为增强版
4. 在 `loop()` 中添加成功率监控代码
5. 添加串口诊断命令

### 步骤2：测试验证
编译上传后，通过串口测试：
```
nfc test      - 查看NFC详细状态
nfc reset     - 手动重置NFC
health        - 查看健康度报告
```

### 步骤3：观察健康度日志
运行2-3天后，在Supabase查询：
```sql
SELECT
  timestamp,
  uptime_hours,
  nfc_initialized,
  nfc_success_rate,
  nfc_read_success_count,
  nfc_read_fail_count,
  nfc_idle_minutes
FROM system_health_logs
WHERE device_id = '34:85:18:62:44:84'
ORDER BY timestamp DESC
LIMIT 100;
```

**期望结果**：
- `nfc_success_rate` 应该稳定在 95%+
- `nfc_initialized` 始终保持 true
- 如果出现成功率下降，应该能看到自动恢复记录

## 🔍 诊断问题的关键指标

### 正常状态
- NFC成功率：> 95%
- NFC版本号：0x92 或 0x91（MFRC522标准版本）
- NFC天线状态：0x83（天线开启）
- 错误寄存器：0x00（无错误）

### 异常状态（需要自动恢复）
- NFC成功率：< 50%
- NFC版本号：0x00 或 0xFF（模块失联）
- NFC天线状态：0x80 或其他（天线关闭）
- 错误寄存器：非0（存在错误）

## 💡 长期改进建议

### 1. 硬件改进
- **添加外部复位电路**：使用一个GPIO控制MOSFET来控制NFC模块电源，实现真正的断电重启
- **电源去耦电容**：在NFC模块3.3V电源引脚添加100nF和10μF电容
- **屏蔽干扰**：检查NFC模块与WiFi天线的物理距离

### 2. 软件改进
- **看门狗定时器**：如果NFC持续失效超过1小时，触发ESP32重启
- **读卡超时机制**：设置5秒读卡超时，避免死循环
- **I2C/SPI总线优先级**：降低OLED刷新频率，减少对SPI总线的干扰

### 3. 监控告警
- **Supabase Edge Function**：当 `nfc_success_rate < 50%` 持续30分钟时发送告警
- **设备自检**：每小时自动执行一次 `nfc test`，记录到日志

## 📝 版本记录

- **v1.1** (2025-12-14)
  - 新增NFC健康检查函数
  - 新增基于成功率的自动恢复
  - 新增串口诊断命令
  - 解决重启后刷卡无响应问题

---

**作者**: GoldSky Development Team
**日期**: 2025-12-14
**用途**: 解决NFC刷卡无响应问题
