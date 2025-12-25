# GoldSky_Lite v0.1.1 - Bug修复版本

**发布日期**: 2025-12-24
**版本号**: v0.1.1
**基于版本**: v0.1.0

## 🐛 问题描述

### Bug: WELCOME状态下NFC检测失效导致健康度数据失真

**问题现象**：
- 用户从VIP查询完成后返回WELCOME界面
- 此时NFC模块停止检测卡片
- 健康度日志中NFC成功率虚高（85%+）
- **实际上NFC根本没有在工作**，而不是成功率真的高

**数据证据**（来自健康度日志分析）：
```
记录952-958: 运行22-25小时
- NFC成功从 2次 → 6次（有用户活动）
- 状态：WELCOME

记录958-967: 运行25-29.5小时
- NFC成功/失败数完全不变（固定6成功/1失败）
- 状态：WELCOME
- **4.5小时内NFC计数器完全不动 = NFC检测已停止**
```

### 根本原因

**代码设计问题**：`handleWelcomeState()` 函数没有调用 `readCardUID()`

```cpp
// ❌ v0.1.0 的 handleWelcomeState() - 不检测NFC
void handleWelcomeState() {
  setSystemLEDStatus();
  displayWelcome();

  // 只处理按钮，不检测NFC
  if (readButtonImproved(BTN_OK)) {
    currentState = STATE_SELECT_PACKAGE;
    // ...
  }
}
```

**当前状态机设计**：
- ✅ `STATE_CARD_SCAN` - 调用 `readCardUID()`
- ✅ `STATE_VIP_QUERY` - 调用 `readCardUID()`
- ❌ **`STATE_WELCOME` - 不调用 `readCardUID()`** ← 问题所在
- ❌ `STATE_SELECT_PACKAGE` - 不调用 `readCardUID()`
- ❌ `STATE_VIP_DISPLAY` - 不调用 `readCardUID()`

**影响**：
1. **健康度数据失真**：设备大部分时间处于WELCOME状态，NFC不检测导致数据不准确
2. **NFC故障无法被发现**：设备在WELCOME状态下NFC可能已经失效，但健康度监测无法发现
3. **自动恢复机制失效**：因为没有检测，就不会产生失败记录，无法触发自动恢复

## ✅ 修复方案

### 方案：在WELCOME状态下持续检测NFC

在 `handleWelcomeState()` 中添加定期NFC检测（仅用于健康度监测，不触发业务逻辑）

#### 修改文件：[GoldSky_Lite.ino:640-670](GoldSky_Lite.ino#L640-L670)

```cpp
// ✅ v0.1.1 的 handleWelcomeState() - 新增NFC健康检测
void handleWelcomeState() {
  setSystemLEDStatus();
  displayWelcome();

  // 欢迎界面刷新时间戳
  static unsigned long lastWelcomeUpdate = 0;
  if (millis() - lastWelcomeUpdate > 60000) {
    lastSuccessfulOperation = millis();
    lastWelcomeUpdate = millis();
  }

  // ✅ 新增：WELCOME状态下持续检测NFC（用于健康度监测）
  // 说明：即使在待机状态也检测NFC，确保健康度数据准确
  //      检测到卡片不会触发业务逻辑，仅用于验证NFC模块工作状态
  static unsigned long lastNFCCheck = 0;
  if (millis() - lastNFCCheck >= 2000) {  // 每2秒检测一次
    String uid = readCardUID();  // 调用检测，会更新成功/失败计数
    // 不做任何处理，仅用于测试NFC是否响应
    lastNFCCheck = millis();
  }

  if (readButtonImproved(BTN_OK)) {
    currentState = STATE_SELECT_PACKAGE;
    selectedPackage = 0;
    stateStartTime = millis();
    beepShort();
    logDebug("✅ 用户启动服务");
  }
}
```

### 关键设计细节

#### 1. 检测频率：每2秒
```cpp
if (millis() - lastNFCCheck >= 2000) {  // 2秒
  String uid = readCardUID();
  lastNFCCheck = millis();
}
```

**为什么是2秒？**
- ✅ 不会过于频繁（避免影响性能）
- ✅ 足够快速发现NFC故障
- ✅ 每小时检测1800次，样本量充足
- ✅ 30分钟内至少900次检测，远超自动恢复阈值（10次）

#### 2. 仅检测，不处理结果
```cpp
String uid = readCardUID();  // 调用检测
// 不做任何处理，仅用于测试NFC是否响应
```

**为什么不处理结果？**
- ✅ 用户在WELCOME界面，没有选择套餐
- ✅ 检测到卡片也不应该触发业务逻辑
- ✅ 目的是**验证NFC硬件是否正常**，而不是处理业务

#### 3. 自动更新健康度指标

`readCardUID()` 内部会自动更新：
```cpp
// 在 GoldSky_Utils.ino 的 readCardUID() 中
if (成功) {
  healthMonitor.recordNFCSuccess();  // 自动更新成功计数
} else {
  healthMonitor.recordNFCFailure();  // 自动更新失败计数
}
```

所以**无需手动更新健康度指标**，调用 `readCardUID()` 就够了！

## 📊 修复效果

### 修复前（v0.1.0）

```
WELCOME状态：
- NFC检测：❌ 不检测
- 健康度数据：❌ 失真（计数器不动）
- 自动恢复：❌ 无法触发（没有失败记录）

用户操作：VIP查询 → 显示结果 → 返回WELCOME
结果：NFC检测停止，健康度数据冻结
```

**健康度日志表现**：
```
记录958: 15:09:46 | NFC成功6次, 失败1次 | WELCOME状态
记录959: 15:39:46 | NFC成功6次, 失败1次 | WELCOME状态  ← 完全一样
记录960: 16:09:46 | NFC成功6次, 失败1次 | WELCOME状态  ← 完全一样
...
记录967: 19:39:45 | NFC成功6次, 失败1次 | WELCOME状态  ← 4.5小时数据完全不变
```

### 修复后（v0.1.1）

```
WELCOME状态：
- NFC检测：✅ 每2秒检测一次
- 健康度数据：✅ 准确（持续更新）
- 自动恢复：✅ 可触发（检测到故障立即恢复）

用户操作：VIP查询 → 显示结果 → 返回WELCOME
结果：NFC持续检测，健康度数据实时更新
```

**健康度日志表现**（预期）：
```
记录X: 15:09:46 | NFC成功120次, 失败3次 | WELCOME状态
记录X+1: 15:39:46 | NFC成功1020次, 失败8次 | WELCOME状态  ← 30分钟内900次检测
记录X+2: 16:09:46 | NFC成功1920次, 失败15次 | WELCOME状态  ← 持续增长
```

**关键改进**：
- ✅ NFC计数器持续增长（说明检测正常）
- ✅ 成功率数据准确（基于真实检测）
- ✅ 如果NFC故障，立即产生大量失败记录
- ✅ 触发自动恢复（10次失败 且 成功率<50%）

## 🔍 验证方法

### 1. 编译上传

```bash
arduino-cli compile --fqbn esp32:esp32:esp32s3 GoldSky_Lite
arduino-cli upload -p COM3 --fqbn esp32:esp32:esp32s3 GoldSky_Lite
```

### 2. 串口测试

打开串口监视器（115200波特率），观察日志：

**在WELCOME状态下，应该看到**：
```
等待刷卡... [NFC版本: 0x92, 状态: 正常]  ← 每5秒
.                                        ← 每1秒（心跳）
```

**如果NFC正常，每2秒会检测一次（但不会有明显日志）**

### 3. 验证NFC计数器

方法1：通过串口命令
```
health      # 查看健康度报告
```

观察输出：
```
NFC成功次数: 120   ← 应该持续增长
NFC失败次数: 3     ← 偶尔增长（正常）
NFC成功率: 97.56%  ← 应该稳定在95%+
```

方法2：通过Supabase数据库

查询最近2小时的健康度日志：
```sql
SELECT
  timestamp,
  uptime_hours,
  nfc_read_success_count,
  nfc_read_fail_count,
  nfc_success_rate,
  current_state
FROM system_health_logs
WHERE device_id = '34:85:18:62:44:84'
  AND timestamp > NOW() - INTERVAL '2 hours'
ORDER BY timestamp DESC;
```

**期望结果**：
```
timestamp            | uptime_hours | success | fail | rate   | state
---------------------|--------------|---------|------|--------|--------
2025-12-24 20:09:46  | 32.00        | 5400    | 15   | 99.72% | WELCOME
2025-12-24 19:39:46  | 31.50        | 4500    | 12   | 99.73% | WELCOME
2025-12-24 19:09:46  | 31.00        | 3600    | 10   | 99.72% | WELCOME
```

**判断标准**：
- ✅ 成功次数每30分钟增长约900次（2秒/次 × 30分钟）
- ✅ 成功率稳定在 95-100%
- ❌ 如果成功次数不变 = bug未修复

### 4. 测试自动恢复

**模拟NFC故障**：
- 拔掉RC522模块或断开SPI连线
- 等待2-3分钟

**期望行为**：
```
串口日志：
⚠️ NFC成功率过低 (0%)，触发自动恢复
🔄 执行NFC硬件完全重置...
⚠️ NFC天线未开启，尝试重新开启
❌ NFC天线开启失败
⚠️ NFC恢复失败，将在30秒后重试
```

**健康度日志**：
```
nfc_read_success_count: 5400
nfc_read_fail_count: 15     ← 开始快速增长
nfc_success_rate: 99.72%    ← 开始下降
...
nfc_read_fail_count: 75     ← 2分钟后（60次失败）
nfc_success_rate: 98.63%    ← 触发自动恢复
```

## 🔄 版本升级

### 从 v0.1.0 升级到 v0.1.1

**无需修改配置**：
- ✅ 数据库表结构不变
- ✅ 硬件接线不变
- ✅ 完全向后兼容

**升级步骤**：
1. 拉取v0.1.1代码
2. 编译上传
3. 验证NFC计数器持续增长

### 影响范围

**修改的代码**：
- [GoldSky_Lite.ino:653-661](GoldSky_Lite.ino#L653-L661) - 新增8行代码

**未修改的代码**：
- ✅ NFC自动恢复机制（v0.1.0）
- ✅ NFC健康检查（v0.1.0）
- ✅ 健康度监测系统（v0.0.9）
- ✅ 所有业务逻辑

## 📝 已知限制

### 限制1：WELCOME状态下检测到卡片不会有反应

**现象**：
- 用户在WELCOME界面刷卡
- 没有蜂鸣声
- 没有进入业务流程

**原因**：
- 设计如此，必须先点击OK进入套餐选择

**如果需要改变**（可选）：
```cpp
// 在 handleWelcomeState() 中修改
if (millis() - lastNFCCheck >= 2000) {
  String uid = readCardUID();

  // 可选：检测到卡片自动进入VIP查询
  if (uid.length() > 0) {
    currentState = STATE_VIP_QUERY;
    stateStartTime = millis();
    beepShort();
    logInfo("✅ 检测到卡片，进入VIP查询");
  }

  lastNFCCheck = millis();
}
```

### 限制2：检测频率影响功耗

**说明**：
- 每2秒检测一次NFC
- 会增加SPI通信频率
- 可能略微增加功耗

**如果需要优化**：
```cpp
// 降低检测频率到5秒
if (millis() - lastNFCCheck >= 5000) {  // 改为5秒
  String uid = readCardUID();
  lastNFCCheck = millis();
}
```

## 🎯 总结

### Bug修复

- ✅ 修复WELCOME状态下NFC检测失效问题
- ✅ 修复健康度数据失真问题
- ✅ 修复自动恢复机制无法触发问题

### 技术改进

- ✅ WELCOME状态每2秒检测NFC
- ✅ 健康度数据持续更新
- ✅ 自动恢复机制可正常工作

### 测试要求

- ✅ 验证NFC计数器持续增长
- ✅ 验证成功率数据准确（95%+）
- ✅ 验证自动恢复可触发

### 下一步

建议部署v0.1.1后，运行48-72小时，观察健康度数据是否符合预期。

---

**作者**: GoldSky Development Team
**日期**: 2025-12-24
**版本**: v0.1.1
