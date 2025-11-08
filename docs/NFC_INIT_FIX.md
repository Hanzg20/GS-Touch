# NFC初始化修复 - 自动重试机制

修复日期：2025-11-02
问题：第一次启动NFC检测失败，需要手动复位后才能成功

---

## 🐛 问题

```
第一次启动 NFC 检测失败
需要手动reset一下，第二遍才能检测成功
```

---

## ✅ 解决方案

### 1. 启动时自动重试3次

**[GoldSky_Lite.ino:615-643](GoldSky_Lite.ino#L615-L643)**

```cpp
// NFC初始化带重试机制
sysStatus.nfcWorking = false;
for (int retry = 0; retry < 3; retry++) {
  if (retry > 0) {
    logWarn("⚠️ NFC初始化重试 " + String(retry) + "/3");
    delay(500);  // 等待模块稳定
  }

  mfrc522.PCD_Init(RC522_CS, RC522_RST);
  delay(100);

  // 软复位
  mfrc522.PCD_Reset();
  delay(100);

  byte version = mfrc522.PCD_ReadRegister(mfrc522.VersionReg);
  if (version != 0x00 && version != 0xFF) {
    sysStatus.nfcWorking = true;
    mfrc522.PCD_AntennaOn();
    delay(10);
    mfrc522.PCD_SetAntennaGain(mfrc522.RxGain_max);
    logInfo("✅ NFC初始化成功: 0x" + String(version, HEX) +
            (retry > 0 ? " (重试成功)" : ""));
    break;  // 成功后退出
  }
}
```

**改进**：
- ✅ 最多重试3次（第一次失败后还有2次机会）
- ✅ 每次重试前延迟500ms等待模块稳定
- ✅ 添加软复位`PCD_Reset()`确保模块状态正确
- ✅ 显示重试状态日志

---

### 2. 运行时自动健康检查

**[GoldSky_Utils.ino:199-260](GoldSky_Utils.ino#L199-L260)**

新增`checkAndRecoverNFC()`函数：
```cpp
bool checkAndRecoverNFC() {
  byte version = mfrc522.PCD_ReadRegister(mfrc522.VersionReg);

  // 版本号正常
  if (version != 0x00 && version != 0xFF) {
    return true;
  }

  // NFC模块异常，尝试恢复
  logWarn("⚠️ NFC模块异常，尝试恢复...");
  mfrc522.PCD_Init(RC522_CS, RC522_RST);
  delay(50);
  mfrc522.PCD_Reset();
  delay(50);

  version = mfrc522.PCD_ReadRegister(mfrc522.VersionReg);
  if (version != 0x00 && version != 0xFF) {
    mfrc522.PCD_AntennaOn();
    delay(10);
    mfrc522.PCD_SetAntennaGain(mfrc522.RxGain_max);
    logInfo("✅ NFC恢复成功: 0x" + String(version, HEX));
    sysStatus.nfcWorking = true;
    return true;
  }

  logError("❌ NFC恢复失败");
  sysStatus.nfcWorking = false;
  return false;
}
```

**在readCardUID()中集成**：
```cpp
String readCardUID() {
  // 每次读卡前健康检查
  if (!checkAndRecoverNFC()) {
    return "";
  }

  // ... 原有读卡逻辑
}
```

**优势**：
- ✅ 每次读卡前自动检查NFC模块状态
- ✅ 如果异常自动恢复，无需手动复位
- ✅ 长期运行稳定性提升

---

## 📊 改进对比

### 改进前

```
启动 → NFC初始化 → 失败 → ❌ 放弃
                   → 成功 → ✅ 工作

问题：失败后需要手动复位
```

### 改进后

```
启动 → NFC初始化（重试1） → 失败 → 延迟500ms
                         → 成功 → ✅ 工作
       ↓
       NFC初始化（重试2） → 失败 → 延迟500ms
                         → 成功 → ✅ 工作
       ↓
       NFC初始化（重试3） → 失败 → ❌ 放弃
                         → 成功 → ✅ 工作

运行时 → 读卡 → 健康检查 → 正常 → 读卡
                        → 异常 → 自动恢复 → 读卡
```

---

## 🧪 预期日志

**第一次成功**：
```
[INFO] 🔌 初始化SPI和NFC...
[INFO] ✅ NFC初始化成功: 0x92
```

**第二次重试成功**：
```
[INFO] 🔌 初始化SPI和NFC...
[WARN] ⚠️ NFC初始化重试 1/3
[INFO] ✅ NFC初始化成功: 0x92 (重试成功)
```

**运行时自动恢复**：
```
[WARN] ⚠️ NFC模块异常，尝试恢复...
[INFO] ✅ NFC恢复成功: 0x92
[INFO] 读取到卡片: HEX=8BCC1674, DEC=2345408116
```

---

## 📝 修改文件

1. ✅ **GoldSky_Lite.ino** - setup()中添加重试循环
2. ✅ **GoldSky_Utils.ino** - 添加checkAndRecoverNFC()函数
3. ✅ **GoldSky_Utils.ino** - 修改readCardUID()集成健康检查

---

## 🎯 解决的问题

- ✅ 第一次启动NFC失败问题 → **自动重试3次**
- ✅ 运行时NFC异常问题 → **自动健康检查和恢复**
- ✅ 需要手动复位问题 → **完全自动化处理**

---

**详细技术文档**: [NFC_INIT_IMPROVEMENT.md](doc/NFC_INIT_IMPROVEMENT.md)

**修改日期**: 2025-11-02
**状态**: ✅ 已完成，待测试
