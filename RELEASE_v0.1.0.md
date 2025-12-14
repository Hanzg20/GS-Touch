# GoldSky_Lite v0.1.0 - NFC自动恢复版本

**发布日期**: 2025-12-14
**版本号**: v0.1.0
**基于版本**: v0.0.9

## 🎯 本次更新目的

**解决设备重启后第二天刷卡无响应的严重问题**

根据v0.0.9健康度监测数据分析，发现：
- 设备重启后NFC成功率从 100% 骤降至 11-24%
- NFC模块初始化标志为true，但实际通信已失效
- 刷卡失败28次，成功仅9次，空闲时长达114小时

**本版本实施了增强的NFC自动恢复机制，主动检测并修复NFC通信故障。**

## ✨ 新增功能

### 1. NFC健康检查（增强版）⭐ 核心功能

#### verifyNFCHealth() - 四步验证法

不再仅检查版本号，而是全面验证NFC模块健康状态：

**第1步：版本寄存器检查**
- 验证版本号不为 0x00 或 0xFF
- 确认模块正常响应

**第2步：天线状态检查**
- 读取 TxControlReg 寄存器
- 验证天线已开启 (bit0, bit1 = 1)
- 如果关闭，自动重新开启

**第3步：寄存器读写验证**
- 写入测试值到 CommandReg
- 读回验证数据一致性
- 确认SPI通信正常

**第4步：错误寄存器检查**
- 读取 ErrorReg 寄存器
- 检测并清除错误标志
- 确保模块无异常状态

**返回结果**：
- `true` = NFC模块完全健康
- `false` = 存在异常，需要重置

### 2. NFC完全重置机制 ⭐

#### performNFCHardReset() - 硬件级重置

传统初始化存在问题：
```cpp
// ❌ 旧方法：仅软件重新初始化
mfrc522.PCD_Init(RC522_CS, RC522_RST);
byte version = mfrc522.PCD_ReadRegister(mfrc522.VersionReg);
if (version != 0x00 && version != 0xFF) {
  // 标记为成功，但可能仍有通信问题
}
```

**新方法**：硬件复位 + 完整验证
```cpp
// ✅ 新方法：硬件复位 + 健康验证
digitalWrite(RC522_RST, LOW);   // 拉低复位引脚
delay(100);                     // 保持100ms
digitalWrite(RC522_RST, HIGH);  // 释放复位
delay(100);

mfrc522.PCD_Init(RC522_CS, RC522_RST);
mfrc522.PCD_AntennaOn();
mfrc522.PCD_SetAntennaGain(mfrc522.RxGain_max);
mfrc522.PCD_WriteRegister(mfrc522.ErrorReg, 0x00);  // 清除错误

return verifyNFCHealth();  // 完整验证
```

**优势**：
- 更长的复位脉冲（100ms vs 10ms）
- 清除所有错误标志
- 通过4步验证确认成功

### 3. 主动健康监测

#### 定期健康检查（每5分钟）

即使 `sysStatus.nfcWorking = true`，也定期验证实际通信状态：

```cpp
void tryRecoverNFC() {
  if (sysStatus.nfcWorking) {
    static unsigned long lastHealthCheck = 0;
    if (millis() - lastHealthCheck >= 300000) {  // 5分钟
      lastHealthCheck = millis();

      if (!verifyNFCHealth()) {
        // 发现健康检查失败，立即标记为异常
        logWarn("⚠️ NFC健康检查失败，尝试恢复...");
        sysStatus.nfcWorking = false;  // 触发恢复流程
        healthMetrics.nfcInitialized = false;
      }
    }
    return;
  }

  // ... 执行恢复 ...
}
```

**关键改进**：
- ❌ 旧逻辑：`if (sysStatus.nfcWorking) return;` - 盲目信任标志
- ✅ 新逻辑：即使标志为true，也定期验证真实状态

### 4. 基于成功率的自动恢复

#### 智能监控（每10分钟）

在 `loop()` 中新增成功率监控：

```cpp
static unsigned long lastNFCSuccessRateCheck = 0;
if (millis() - lastNFCSuccessRateCheck >= 600000) {  // 10分钟
  lastNFCSuccessRateCheck = millis();

  float successRate = healthMetrics.getNFCSuccessRate();
  int totalReads = healthMetrics.nfcReadSuccessCount + healthMetrics.nfcReadFailCount;

  // 触发条件：至少10次读卡 + 成功率低于50%
  if (totalReads >= 10 && successRate < 50.0 && sysStatus.nfcWorking) {
    logWarn("⚠️ NFC成功率过低 (" + String(successRate, 1) + "%)，触发自动恢复");
    sysStatus.nfcWorking = false;  // 触发恢复流程

    // 重置统计数据
    healthMetrics.nfcReadSuccessCount = 0;
    healthMetrics.nfcReadFailCount = 0;
  }
}
```

**智能阈值设计**：
- **样本量要求**：≥ 10次读卡（避免误判）
- **成功率阈值**：< 50%（正常应 > 95%）
- **统计重置**：恢复后清零，重新统计

### 5. 串口诊断命令

#### nfc test - NFC健康诊断

```
🔍 NFC健康诊断测试...
   版本寄存器: 0x92
   天线状态: 0x83 (ON)
   错误寄存器: 0x00
   增益配置: 0x70
✅ NFC模块健康
```

#### nfc reset - 手动重置NFC

```
🔄 手动重置NFC模块...
🔄 执行NFC硬件完全重置...
✅ NFC健康检查通过 (版本: 0x92, 天线: ON, 寄存器: OK)
✅ NFC重置成功
```

**用途**：
- 现场调试时手动触发恢复
- 验证NFC模块硬件状态
- 快速诊断通信问题

## 🔧 技术实现

### 修改文件

#### 1. GoldSky_Lite.ino

**新增函数**（第168-270行）：
```cpp
bool verifyNFCHealth()        // NFC健康检查（4步验证）
bool performNFCHardReset()    // NFC硬件完全重置
void tryRecoverNFC()          // NFC自动恢复（增强版）
```

**loop()函数新增**（第1328-1345行）：
```cpp
// 基于成功率的NFC自动恢复
static unsigned long lastNFCSuccessRateCheck = 0;
if (millis() - lastNFCSuccessRateCheck >= 600000) {
  // ... 成功率监控逻辑 ...
}
```

**串口命令新增**（第1469-1500行）：
```cpp
else if (cmd == "nfc test") {
  // ... NFC诊断代码 ...
}
else if (cmd == "nfc reset") {
  // ... 手动重置代码 ...
}
```

**help命令更新**（第1512-1513行）：
```cpp
Serial.println("nfc test    - NFC模块健康诊断");
Serial.println("nfc reset   - 手动重置NFC模块");
```

### 新增文件

#### NFC_AUTO_RECOVERY.md

完整的NFC自动恢复文档，包含：
- 问题诊断分析（基于v0.0.9数据）
- 解决方案详细说明
- 实施步骤指南
- SQL诊断查询
- 关键指标说明
- 长期改进建议

## 📊 问题诊断（基于v0.0.9数据）

### 健康度数据分析

**设备ID**: 34:85:18:62:44:84
**运行时长**: 166小时（7天）

#### 记录138之前（正常运行67.89小时）
- ✅ NFC成功率：100%
- ✅ 系统稳定：0次错误
- ✅ 内存正常：298KB → 296KB

#### 记录138（意外重启）
- ⚠️ 运行时长重置：67.89小时 → 0.01小时
- ❌ NFC成功率骤降：100% → 11%

#### 记录138之后（持续7天）
- ❌ NFC成功率稳定低位：11-24%
- ❌ NFC成功次数：仅9次
- ❌ NFC失败次数：28次
- ❌ NFC空闲时长：6,834分钟（114小时）

### 根本原因

**NFC模块初始化标志为true，但SPI通信实际已失效**

可能触发因素：
1. 重启后初始化时序异常
2. SPI总线干扰（WiFi/OLED）
3. NFC固件进入异常状态
4. 电源波动影响模块寄存器

**旧代码问题**：
```cpp
// ❌ 仅检查版本号，未验证通信
byte version = mfrc522.PCD_ReadRegister(mfrc522.VersionReg);
if (version != 0x00 && version != 0xFF) {
  sysStatus.nfcWorking = true;  // 标记成功
  // 但可能天线未开启、寄存器读写异常、错误标志未清除
}
```

## 🔍 验证方法

### 1. 编译上传

```bash
# Arduino CLI
arduino-cli compile --fqbn esp32:esp32:esp32s3 GoldSky_Lite
arduino-cli upload -p COM3 --fqbn esp32:esp32:esp32s3 GoldSky_Lite
```

### 2. 串口测试

打开串口监视器（115200波特率）：

```
nfc test      # 查看NFC详细状态
nfc reset     # 手动重置测试
health        # 查看健康度报告
```

**期望输出**：
```
🔍 NFC健康诊断测试...
   版本寄存器: 0x92
   天线状态: 0x83 (ON)
   错误寄存器: 0x00
   增益配置: 0x70
✅ NFC模块健康
```

### 3. 观察自动恢复日志

设备运行时，每5分钟会自动执行健康检查：

```
✅ NFC健康检查通过 (版本: 0x92, 天线: ON, 寄存器: OK)
```

如果检测到异常：
```
⚠️ NFC健康检查失败，尝试恢复...
🔄 执行NFC硬件完全重置...
✅ NFC恢复成功
```

### 4. Supabase数据验证

运行2-3天后查询：

```sql
SELECT
  timestamp,
  uptime_hours,
  nfc_initialized,
  nfc_success_rate,
  nfc_read_success_count,
  nfc_read_fail_count
FROM system_health_logs
WHERE device_id = '34:85:18:62:44:84'
  AND timestamp > NOW() - INTERVAL '3 days'
ORDER BY timestamp DESC;
```

**期望结果**：
- `nfc_success_rate` 稳定在 95%+
- `nfc_initialized` 始终为 `true`
- 无长时间低成功率记录

## 🐛 Bug修复

无

## 📦 部署指南

### 前提条件
- 已部署v0.0.9版本
- Supabase `system_health_logs` 表已创建

### 部署步骤

1. **备份当前代码**
   ```bash
   git add .
   git commit -m "Backup before v0.1.0 upgrade"
   ```

2. **拉取v0.1.0代码**
   ```bash
   git pull origin main
   ```

3. **编译上传**
   - 使用Arduino IDE或Arduino CLI编译
   - 上传到ESP32-S3设备

4. **验证功能**
   - 串口输入 `nfc test` 验证诊断功能
   - 串口输入 `health` 查看健康度报告
   - 观察5分钟后是否有健康检查日志

5. **长期监控**
   - 让设备运行2-3天
   - 定期查询Supabase健康度数据
   - 验证NFC成功率始终 > 95%

## 🔄 版本兼容性

- **向后兼容**: ✅ 完全兼容v0.0.9
- **数据库迁移**: 无需修改，使用相同表结构
- **配置文件**: 无需修改
- **硬件要求**: 无变化

## 📝 已知问题

### 问题1：频繁自动恢复

**现象**：串口日志每5-10分钟出现NFC恢复日志

**可能原因**：
- 硬件连接不稳定（SPI接线松动）
- 电源电压不足
- 环境电磁干扰严重

**解决方法**：
1. 检查SPI接线（MOSI/MISO/SCK/CS/RST）
2. 测量3.3V电源电压（应 ≥ 3.2V）
3. 远离WiFi路由器/微波炉等干扰源
4. 增加电源滤波电容（100nF + 10μF）

### 问题2：nfc test 显示天线状态OFF

**现象**：`nfc test` 输出 `天线状态: 0x80 (OFF)`

**原因**：天线未正确开启或硬件故障

**解决方法**：
1. 执行 `nfc reset` 手动重置
2. 如果仍然失败，检查MFRC522模块硬件
3. 尝试更换NFC模块

## 🚀 下一步计划

### v0.1.1（计划中）

1. **看门狗定时器**
   - NFC持续失效超过1小时，自动重启ESP32
   - 防止极端情况下系统卡死

2. **Supabase告警**
   - Edge Function监控 `nfc_success_rate < 50%` 持续30分钟
   - 发送邮件/短信告警

3. **硬件断电重启**
   - 使用GPIO控制MOSFET切断NFC模块电源
   - 实现真正的硬件断电重启

### v0.2.0（研究中）

1. **NFC固件升级**
   - 检测MFRC522固件版本
   - 支持OTA升级NFC固件

2. **多NFC模块冗余**
   - 支持连接2个NFC模块
   - 主模块失效自动切换备用模块

## 👥 贡献者

- GoldSky Development Team

## 📄 许可证

[项目许可证]

---

**安装说明**: 参考本文档"部署指南"章节
**技术文档**: 参考 [NFC_AUTO_RECOVERY.md](./NFC_AUTO_RECOVERY.md)
**健康度监测**: 参考 [HEALTH_MONITOR_README.md](./HEALTH_MONITOR_README.md)
