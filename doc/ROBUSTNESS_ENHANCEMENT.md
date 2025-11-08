# 系统健壮性增强方案

实施日期：2025-11-02
版本：v2.4.1
目标：提升系统容错能力和故障指示

---

## 🎯 问题背景

### 原有问题
1. **NFC初始化失败** → 系统停止运行，无法使用
2. **WiFi连接失败** → 无法进行在线验证，但没有明确提示
3. **无故障指示** → 用户无法判断故障原因
4. **无自动恢复** → 需要手动重启才能恢复

### 影响
- ❌ 系统可用性差
- ❌ 用户体验不佳
- ❌ 运维成本高

---

## ✅ 解决方案

### 方案设计原则
1. **降级运行** - 部分功能失败不影响系统启动
2. **清晰指示** - LED明确显示故障状态
3. **自动恢复** - 运行时自动重试失败的模块
4. **不阻塞启动** - 即使初始化失败也能进入主循环

---

## 📋 实施内容

### 1. 配置参数新增

**[config.h:101-105](../config.h#L101-L105)**

```cpp
// =================== 健壮性增强配置 ===================
#define NFC_RETRY_INTERVAL_MS 30000     // NFC重试间隔：30秒
#define WIFI_RETRY_INTERVAL_MS 60000    // WiFi重试间隔：60秒
#define ALLOW_OFFLINE_MODE true         // 允许离线模式运行
#define FAULT_LED_BLINK_INTERVAL 300    // 故障LED闪烁间隔（ms）
```

**参数说明**：
- `NFC_RETRY_INTERVAL_MS`: NFC模块恢复重试间隔（30秒）
- `WIFI_RETRY_INTERVAL_MS`: WiFi连接恢复重试间隔（60秒）
- `ALLOW_OFFLINE_MODE`: 允许在WiFi断开时继续运行
- `FAULT_LED_BLINK_INTERVAL`: 故障LED闪烁周期

---

### 2. NFC初始化降级模式

**修改前** - 失败后无提示，系统继续但无法使用：
```cpp
byte version = mfrc522.PCD_ReadRegister(mfrc522.VersionReg);
if (version != 0x00 && version != 0xFF) {
  sysStatus.nfcWorking = true;
  logInfo("✅ NFC初始化成功");
} else {
  sysStatus.nfcWorking = false;
  logError("❌ NFC初始化失败");
}
// 系统继续，但NFC功能不可用且无提示
```

**修改后** - 明确告知降级模式：

**[GoldSky_Lite.ino:704-719](../GoldSky_Lite.ino#L704-L719)**

```cpp
// NFC初始化 - 失败后继续运行（降级模式）
mfrc522.PCD_Init(RC522_CS, RC522_RST);
delay(200);

byte version = mfrc522.PCD_ReadRegister(mfrc522.VersionReg);
if (version != 0x00 && version != 0xFF) {
  sysStatus.nfcWorking = true;
  mfrc522.PCD_AntennaOn();
  mfrc522.PCD_SetAntennaGain(mfrc522.RxGain_max);
  logInfo("✅ NFC初始化成功: 0x" + String(version, HEX));
} else {
  sysStatus.nfcWorking = false;
  logError("❌ NFC初始化失败，版本号: 0x" + String(version, HEX));
  logWarn("⚠️ 系统将以降级模式运行（NFC功能不可用）");
  logWarn("⚠️ NFC将在运行时自动重试恢复");
}
// 系统继续运行，不阻塞启动
```

**改进点**：
- ✅ 明确日志提示
- ✅ 说明后续行为（自动重试）
- ✅ 不阻塞系统启动

---

### 3. NFC自动恢复机制

**[GoldSky_Lite.ino:145-167](../GoldSky_Lite.ino#L145-L167)**

```cpp
// =================== NFC自动恢复 ===================
void tryRecoverNFC() {
  if (sysStatus.nfcWorking) return;  // NFC正常，无需恢复

  if (millis() - lastNFCRetry < NFC_RETRY_INTERVAL_MS) return;  // 未到重试时间

  lastNFCRetry = millis();
  logInfo("🔄 尝试恢复NFC模块...");

  mfrc522.PCD_Init(RC522_CS, RC522_RST);
  delay(200);

  byte version = mfrc522.PCD_ReadRegister(mfrc522.VersionReg);
  if (version != 0x00 && version != 0xFF) {
    sysStatus.nfcWorking = true;
    mfrc522.PCD_AntennaOn();
    mfrc522.PCD_SetAntennaGain(mfrc522.RxGain_max);
    logInfo("✅ NFC恢复成功: 0x" + String(version, HEX));
    beepSuccess();
  } else {
    logWarn("⚠️ NFC恢复失败，将在" + String(NFC_RETRY_INTERVAL_MS/1000) + "秒后重试");
  }
}
```

**工作流程**：
1. 检查NFC状态，如果正常则跳过
2. 检查是否到达重试时间（30秒间隔）
3. 尝试重新初始化NFC模块
4. 成功则恢复状态，失败则等待下次重试

---

### 4. WiFi自动恢复机制

**[GoldSky_Lite.ino:169-193](../GoldSky_Lite.ino#L169-L193)**

```cpp
// =================== WiFi自动恢复 ===================
void tryRecoverWiFi() {
  if (sysStatus.wifiConnected) return;  // WiFi正常，无需恢复

  if (millis() - lastWiFiRetry < WIFI_RETRY_INTERVAL_MS) return;  // 未到重试时间

  lastWiFiRetry = millis();
  logInfo("🔄 尝试恢复WiFi连接...");

  WiFi.disconnect();
  delay(500);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  unsigned long startTime = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startTime < 10000) {
    delay(500);
  }

  if (WiFi.status() == WL_CONNECTED) {
    sysStatus.wifiConnected = true;
    logInfo("✅ WiFi恢复成功: " + WiFi.localIP().toString());
  } else {
    logWarn("⚠️ WiFi恢复失败，将在" + String(WIFI_RETRY_INTERVAL_MS/1000) + "秒后重试");
  }
}
```

**工作流程**：
1. 检查WiFi状态，如果正常则跳过
2. 检查是否到达重试时间（60秒间隔）
3. 尝试重新连接WiFi
4. 成功则恢复状态，失败则等待下次重试

---

### 5. loop()中调用自动恢复

**[GoldSky_Lite.ino:843-849](../GoldSky_Lite.ino#L843-L849)**

```cpp
void loop() {
  // ... 现有代码

  checkWiFi();
  checkStateTimeout();
  performHealthCheck();

  // 健壮性增强：自动尝试恢复失败的模块
  tryRecoverNFC();
  tryRecoverWiFi();

  // ... 状态机处理
}
```

**执行顺序**：
1. 每次loop都会检查
2. 如果模块失败且到达重试时间，自动尝试恢复
3. 不影响主流程运行

---

### 6. 故障LED指示

**[GoldSky_Utils.ino:87-175](../GoldSky_Utils.ino#L87-L175)**

#### LED指示规则

| LED | 正常状态 | 故障状态 |
|-----|---------|---------|
| **POWER** | 常亮 | 常亮 |
| **NETWORK** | 连接=常亮<br>断开=慢闪 | 同左 |
| **PROGRESS** | 根据流程状态 | 根据流程状态 |
| **STATUS** | 根据流程状态 | **NFC故障=快闪** ⚠️ |

#### 实现代码

```cpp
void setSystemLEDStatus() {
  ledIndicator.power = true;

  // 网络LED：连接=常亮，断开=慢闪
  if (sysStatus.wifiConnected) {
    ledIndicator.network = LED_ON;
  } else {
    ledIndicator.network = LED_BLINK_SLOW;
  }

  // 故障指示：NFC失败时STATUS LED快闪（优先级最高）
  if (!sysStatus.nfcWorking) {
    ledIndicator.status = LED_BLINK_FAST;  // NFC故障：STATUS快闪
  } else {
    // 正常流程的LED状态
    switch (currentState) {
      case STATE_WELCOME:
        ledIndicator.status = LED_OFF;
        break;
      case STATE_CARD_SCAN:
        ledIndicator.status = LED_BLINK_FAST;
        break;
      // ... 其他状态
    }
  }

  // PROGRESS LED始终根据状态设置（不受NFC故障影响）
  switch (currentState) {
    case STATE_WELCOME:
      ledIndicator.progress = LED_OFF;
      break;
    case STATE_PROCESSING:
      ledIndicator.progress = LED_ON;
      break;
    // ... 其他状态
  }

  updateLEDIndicators();
}
```

**优先级设计**：
1. **NFC故障** → STATUS LED快闪（最高优先级）
2. **WiFi断开** → NETWORK LED慢闪
3. **正常流程** → STATUS和PROGRESS LED按流程显示

---

## 🎯 使用场景

### 场景1：NFC初始化失败

**启动日志**：
```
[INFO] 🔌 初始化SPI和NFC...
[ERROR] ❌ NFC初始化失败，版本号: 0xff
[WARN] ⚠️ 系统将以降级模式运行（NFC功能不可用）
[WARN] ⚠️ NFC将在运行时自动重试恢复
[INFO] 🖥️ 初始化I2C和OLED...
[INFO] ✅ OLED初始化成功
[INFO] 系统已就绪！
```

**LED状态**：
- POWER: 常亮 ✅
- NETWORK: 根据WiFi状态
- PROGRESS: 灭 (欢迎界面)
- STATUS: **快闪** ⚠️ (NFC故障)

**运行时恢复**（30秒后）：
```
[INFO] 🔄 尝试恢复NFC模块...
[INFO] ✅ NFC恢复成功: 0x92
```

**LED恢复**：
- STATUS: 停止快闪，恢复正常 ✅

---

### 场景2：WiFi连接失败

**启动日志**：
```
[INFO] 连接WiFi: hanzg_hanyh
.........
[WARN] ⚠️ WiFi连接失败（3次重试后），进入离线模式
[INFO] 系统已就绪！
```

**LED状态**：
- POWER: 常亮 ✅
- NETWORK: **慢闪** ⚠️ (WiFi断开)
- PROGRESS: 灭
- STATUS: 灭

**运行时恢复**（60秒后）：
```
[INFO] 🔄 尝试恢复WiFi连接...
[INFO] ✅ WiFi恢复成功: 10.0.0.110
```

**LED恢复**：
- NETWORK: 停止慢闪，变为常亮 ✅

---

### 场景3：NFC和WiFi同时失败

**启动日志**：
```
[ERROR] ❌ NFC初始化失败，版本号: 0xff
[WARN] ⚠️ 系统将以降级模式运行（NFC功能不可用）
[WARN] ⚠️ NFC将在运行时自动重试恢复
...
[WARN] ⚠️ WiFi连接失败（3次重试后），进入离线模式
[INFO] 系统已就绪！
```

**LED状态**：
- POWER: 常亮 ✅
- NETWORK: **慢闪** ⚠️ (WiFi断开)
- PROGRESS: 灭
- STATUS: **快闪** ⚠️ (NFC故障)

**故障指示**：
- 用户可以通过STATUS快闪判断NFC故障
- 通过NETWORK慢闪判断WiFi断开
- 系统仍可显示界面和按钮操作

**运行时恢复**：
- 30秒后尝试恢复NFC
- 60秒后尝试恢复WiFi
- 成功后LED恢复正常

---

## 📊 状态转换图

```
┌─────────────────┐
│  系统启动       │
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│ NFC初始化       │
└────┬────────┬───┘
     │        │
   成功      失败
     │        │
     │        ▼
     │  ┌─────────────────┐
     │  │ NFC故障模式     │
     │  │ - STATUS快闪    │
     │  │ - 系统继续运行  │
     │  │ - 30秒后自动重试│
     │  └────────┬────────┘
     │           │
     │           │ 恢复成功
     │           ▼
     │  ┌─────────────────┐
     │  │ NFC恢复         │
     │  │ - 停止快闪      │
     │  │ - 蜂鸣器提示    │
     │  └─────────────────┘
     │
     ▼
┌─────────────────┐
│ WiFi连接        │
└────┬────────┬───┘
     │        │
   成功      失败
     │        │
     │        ▼
     │  ┌─────────────────┐
     │  │ 离线模式        │
     │  │ - NETWORK慢闪   │
     │  │ - 系统继续运行  │
     │  │ - 60秒后自动重试│
     │  └────────┬────────┘
     │           │
     │           │ 恢复成功
     │           ▼
     │  ┌─────────────────┐
     │  │ WiFi恢复        │
     │  │ - NETWORK常亮   │
     │  └─────────────────┘
     │
     ▼
┌─────────────────┐
│ 系统正常运行    │
│ - 所有功能可用  │
│ - LED正常指示   │
└─────────────────┘
```

---

## ✅ 改进效果

### 可用性提升
| 项目 | 修改前 | 修改后 |
|------|--------|--------|
| NFC失败后 | ❌ 系统无法使用 | ✅ 系统降级运行，自动恢复 |
| WiFi失败后 | ⚠️ 无提示，离线可用 | ✅ LED提示，离线可用，自动恢复 |
| 故障指示 | ❌ 无 | ✅ LED快闪/慢闪指示 |
| 恢复方式 | ❌ 手动重启 | ✅ 自动重试恢复 |

### 用户体验提升
- ✅ 清晰的故障指示（LED闪烁）
- ✅ 详细的日志提示
- ✅ 自动恢复，无需人工干预
- ✅ 系统不会"卡住"

### 运维成本降低
- ✅ 减少现场维护次数
- ✅ 故障自动恢复
- ✅ 日志清晰，易于远程诊断

---

## 🧪 测试验证

### 测试场景1：NFC初始化失败

**步骤**：
1. 断开NFC模块连接
2. 上电启动系统
3. 观察LED和日志
4. 重新连接NFC模块
5. 等待30秒

**预期结果**：
- ✅ 启动时STATUS LED快闪
- ✅ 日志显示降级模式提示
- ✅ OLED显示正常
- ✅ 30秒后自动恢复NFC
- ✅ STATUS LED停止快闪
- ✅ 蜂鸣器提示恢复成功

---

### 测试场景2：WiFi连接失败

**步骤**：
1. 修改WiFi密码为错误值
2. 上电启动系统
3. 观察LED和日志
4. 修复WiFi配置
5. 等待60秒

**预期结果**：
- ✅ 启动时NETWORK LED慢闪
- ✅ 日志显示离线模式提示
- ✅ 系统正常运行
- ✅ 60秒后自动恢复WiFi
- ✅ NETWORK LED变为常亮

---

### 测试场景3：运行时NFC断开

**步骤**：
1. 系统正常运行
2. 断开NFC模块连接
3. 观察LED变化
4. 重新连接NFC模块
5. 等待30秒

**预期结果**：
- ✅ STATUS LED开始快闪
- ✅ 刷卡功能不可用
- ✅ 其他功能正常
- ✅ 30秒后自动恢复
- ✅ STATUS LED恢复正常

---

## 📚 相关文档

- [NFC_ANTENNA_OFF_FIX.md](NFC_ANTENNA_OFF_FIX.md) - NFC天线修复
- [WIFI_INIT_FIX.md](../WIFI_INIT_FIX.md) - WiFi初始化修复
- [config.h](../config.h) - 配置参数

---

## 🎯 后续优化建议

### 短期优化
1. **OLED故障指示** - 在屏幕上显示故障图标
2. **故障计数器** - 记录故障次数用于诊断
3. **远程监控** - 上报故障状态到服务器

### 长期优化
1. **看门狗增强** - 更完善的死锁检测
2. **日志上传** - 故障日志自动上传到云端
3. **固件OTA** - 远程固件更新能力

---

**文档版本**: v1.0
**最后更新**: 2025-11-02
**状态**: ✅ 已实现，待测试验证
