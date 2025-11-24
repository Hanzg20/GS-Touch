# 离线缓存读取使用指南

**创建日期**: 2025-11-11
**适用版本**: v0.6+

---

## 📋 目录
1. [离线缓存概述](#离线缓存概述)
2. [如何读取缓存](#如何读取缓存)
3. [缓存数据结构](#缓存数据结构)
4. [代码示例](#代码示例)
5. [故障排查](#故障排查)

---

## 📦 离线缓存概述

### 什么是离线缓存？
当WiFi断开时，系统会将交易记录保存到ESP32的**NVS存储**（Non-Volatile Storage），等WiFi恢复后自动同步到Supabase数据库。

### 缓存能保存多少笔交易？
- **最大容量**: 50笔交易
- **存储位置**: ESP32 NVS（断电不丢失）
- **自动同步**: WiFi恢复后自动上传

### 缓存的工作流程
```
用户刷卡 → WiFi断开？
              ├─ 是 → 保存到NVS → 等待WiFi恢复 → 自动同步到数据库
              └─ 否 → 直接保存到数据库
```

---

## 🔍 如何读取缓存

### 方法1: 通过串口监视器查看（推荐）✅

#### 步骤：
1. 打开Arduino IDE的串口监视器（波特率: 115200）
2. 系统启动时会自动显示缓存信息

#### 输出示例：
```
💾 初始化NVS存储...
📥 加载了 3 笔离线交易
```

---

### 方法2: 查看当前缓存状态

在代码中添加以下函数来查看缓存：

```cpp
void printOfflineQueue() {
  Serial.println("=== 离线缓存队列 ===");
  Serial.printf("队列数量: %d/%d\n", offlineQueueCount, MAX_OFFLINE_QUEUE);

  if (offlineQueueCount == 0) {
    Serial.println("✅ 队列为空，所有交易已同步");
    return;
  }

  for (int i = 0; i < offlineQueueCount; i++) {
    PendingTransaction& tx = offlineQueue[i];
    Serial.printf("\n交易 #%d:\n", i + 1);
    Serial.printf("  卡号: %s\n", tx.cardUID.c_str());
    Serial.printf("  金额: $%.2f\n", tx.amount);
    Serial.printf("  扣费前余额: $%.2f\n", tx.balanceBefore);
    Serial.printf("  套餐: %s\n", tx.packageName.c_str());
    Serial.printf("  时间戳: %lu\n", tx.timestamp);
  }
  Serial.println("=====================");
}
```

#### 调用方法：
在 `loop()` 函数中添加按键检测：
```cpp
// 按下某个按钮时打印缓存
if (digitalRead(SOME_BUTTON) == LOW) {
  printOfflineQueue();
  delay(500);  // 防抖
}
```

或者在串口监视器中添加命令（需要实现串口命令处理）：
```cpp
if (Serial.available()) {
  String cmd = Serial.readStringUntil('\n');
  if (cmd == "cache") {
    printOfflineQueue();
  }
}
```

---

### 方法3: 直接从NVS读取

如果你想从其他代码直接读取NVS中的缓存：

```cpp
void readCacheFromNVS() {
  Preferences prefs;
  prefs.begin("goldsky", true);  // true = 只读模式

  int count = prefs.getInt("queue_count", 0);
  Serial.printf("缓存交易数量: %d\n", count);

  for (int i = 0; i < count; i++) {
    String key = "tx_" + String(i);
    String data = prefs.getString(key.c_str(), "");

    if (data.length() > 0) {
      Serial.printf("交易 #%d: %s\n", i + 1, data.c_str());

      // 解析数据格式：cardUID|amount|balanceBefore|packageName
      int pos1 = data.indexOf('|');
      int pos2 = data.indexOf('|', pos1 + 1);
      int pos3 = data.indexOf('|', pos2 + 1);

      String cardUID = data.substring(0, pos1);
      float amount = data.substring(pos1 + 1, pos2).toFloat();
      float balanceBefore = data.substring(pos2 + 1, pos3).toFloat();
      String packageName = data.substring(pos3 + 1);

      Serial.printf("  卡号: %s\n", cardUID.c_str());
      Serial.printf("  金额: $%.2f\n", amount);
      Serial.printf("  扣费前: $%.2f\n", balanceBefore);
      Serial.printf("  套餐: %s\n", packageName.c_str());
    }
  }

  prefs.end();
}
```

---

## 📊 缓存数据结构

### PendingTransaction 结构体
```cpp
struct PendingTransaction {
  String cardUID;        // 卡片UID（十进制字符串）
  float amount;          // 交易金额（负数表示扣费）
  float balanceBefore;   // 扣费前余额
  String packageName;    // 套餐名称
  unsigned long timestamp; // 时间戳（毫秒）
};
```

### NVS存储格式
```
Key                 Value
─────────────────────────────────────────────────
queue_count         3 (整数，表示缓存数量)
tx_0                "2345408116|-5.00|550.00|5 Min Wash"
tx_1                "1234567890|-4.00|100.00|4 Min Wash"
tx_2                "9876543210|-8.00|200.00|8 Min Wash"
```

### 数据格式解析
```
"cardUID|amount|balanceBefore|packageName"
   ↓       ↓         ↓              ↓
示例: "2345408116|-5.00|550.00|5 Min Wash"
```

---

## 💻 代码示例

### 示例1: 启动时自动加载并显示

**位置**: `setup()` 函数

```cpp
void setup() {
  // ... 其他初始化代码 ...

  // 加载离线交易队列
  loadOfflineQueue();

  // 如果有缓存，显示详情
  if (offlineQueueCount > 0) {
    printOfflineQueue();  // 使用上面定义的函数
  }
}
```

---

### 示例2: 定时检查缓存状态

```cpp
void loop() {
  // ... 主循环代码 ...

  // 每60秒检查一次缓存
  static unsigned long lastCacheCheck = 0;
  if (millis() - lastCacheCheck > 60000) {
    if (offlineQueueCount > 0) {
      logInfo("📦 离线缓存: " + String(offlineQueueCount) + " 笔待同步");
    }
    lastCacheCheck = millis();
  }
}
```

---

### 示例3: 手动触发同步

```cpp
// 按OK键时手动同步
if (readButtonImproved(BTN_OK) && offlineQueueCount > 0) {
  logInfo("🔄 手动触发同步...");
  if (syncOfflineQueue()) {
    logInfo("✅ 同步成功!");
  } else {
    logWarn("⚠️ 同步失败，请检查WiFi连接");
  }
}
```

---

### 示例4: 清空缓存（调试用）

⚠️ **警告**: 这会删除所有未同步的交易！

```cpp
void clearOfflineQueue() {
  offlineQueueCount = 0;
  prefs.putInt("queue_count", 0);

  // 清除所有交易记录
  for (int i = 0; i < MAX_OFFLINE_QUEUE; i++) {
    String key = "tx_" + String(i);
    prefs.remove(key.c_str());
  }

  logInfo("🗑️ 离线缓存已清空");
}
```

---

## 🔧 故障排查

### 问题1: 缓存没有保存

**症状**: WiFi断开后交易丢失

**检查**:
1. 确认NVS已初始化
```cpp
prefs.begin("goldsky", false);  // 必须在setup()中调用
```

2. 检查 `addToOfflineQueue()` 是否被调用
```cpp
// 在 recordTransaction() 中应该有：
if (!sysStatus.wifiConnected) {
  addToOfflineQueue(cardUID, amount, balanceBefore, packageName);
}
```

---

### 问题2: 缓存没有自动同步

**症状**: WiFi恢复后缓存仍在

**检查**:
1. 查看 `syncOfflineQueue()` 是否被调用
```cpp
// 在 recordTransaction() 成功后应该有：
if (offlineQueueCount > 0) {
  syncOfflineQueue();
}
```

2. 检查WiFi状态
```cpp
Serial.printf("WiFi状态: %s\n", sysStatus.wifiConnected ? "已连接" : "未连接");
```

3. 查看同步日志
```
🔄 同步离线交易队列 (3 笔)...
✅ 同步交易 #1: 卡号=2345408116
✅ 同步交易 #2: 卡号=1234567890
✅ 同步交易 #3: 卡号=9876543210
✅ 成功同步 3 笔离线交易
```

---

### 问题3: 缓存队列已满

**症状**: 日志显示"离线队列已满，覆盖最旧记录"

**原因**:
- 超过50笔交易未同步
- WiFi长时间断开

**解决**:
1. 检查WiFi连接
2. 手动触发同步
3. 如果确认已同步但计数器未清零，可以手动清空：
```cpp
if (offlineQueueCount >= MAX_OFFLINE_QUEUE) {
  clearOfflineQueue();  // 仅在确认数据已同步后使用
}
```

---

### 问题4: 读取到的数据格式错误

**症状**: 解析数据时崩溃或显示乱码

**检查**:
```cpp
String data = prefs.getString("tx_0", "");
Serial.println("原始数据: " + data);

// 验证格式
if (data.indexOf('|') == -1) {
  Serial.println("❌ 数据格式错误，缺少分隔符");
}
```

**修复**: 清空NVS重新开始
```cpp
prefs.clear();  // ⚠️ 会删除所有数据
```

---

## 📱 实用工具函数

### 完整的缓存管理工具

将以下代码添加到 `GoldSky_Lite.ino`:

```cpp
// =================== 缓存管理工具 ===================

// 打印缓存详情
void printOfflineQueue() {
  Serial.println("\n=== 离线缓存队列 ===");
  Serial.printf("队列数量: %d/%d\n", offlineQueueCount, MAX_OFFLINE_QUEUE);
  Serial.printf("WiFi状态: %s\n", sysStatus.wifiConnected ? "已连接" : "未连接");

  if (offlineQueueCount == 0) {
    Serial.println("✅ 队列为空");
    Serial.println("=====================\n");
    return;
  }

  float totalAmount = 0;
  for (int i = 0; i < offlineQueueCount; i++) {
    PendingTransaction& tx = offlineQueue[i];
    Serial.printf("\n[交易 #%d]\n", i + 1);
    Serial.printf("  卡号: %s\n", tx.cardUID.c_str());
    Serial.printf("  金额: $%.2f\n", tx.amount);
    Serial.printf("  扣费前: $%.2f → 扣费后: $%.2f\n",
                  tx.balanceBefore, tx.balanceBefore + tx.amount);
    Serial.printf("  套餐: %s\n", tx.packageName.c_str());
    Serial.printf("  时间: %lu ms\n", tx.timestamp);
    totalAmount += tx.amount;
  }

  Serial.printf("\n总金额: $%.2f\n", totalAmount);
  Serial.println("=====================\n");
}

// 串口命令处理
void handleSerialCommands() {
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();

    if (cmd == "cache") {
      printOfflineQueue();
    }
    else if (cmd == "sync") {
      if (offlineQueueCount > 0) {
        Serial.println("🔄 手动触发同步...");
        if (syncOfflineQueue()) {
          Serial.println("✅ 同步成功!");
        } else {
          Serial.println("❌ 同步失败");
        }
      } else {
        Serial.println("✅ 无待同步交易");
      }
    }
    else if (cmd == "clear") {
      Serial.println("⚠️ 确认清空缓存？ (y/n)");
      delay(5000);
      if (Serial.available()) {
        String confirm = Serial.readStringUntil('\n');
        if (confirm == "y" || confirm == "Y") {
          clearOfflineQueue();
        } else {
          Serial.println("❌ 已取消");
        }
      }
    }
    else if (cmd == "help") {
      Serial.println("\n=== 可用命令 ===");
      Serial.println("cache  - 显示离线缓存");
      Serial.println("sync   - 手动同步缓存");
      Serial.println("clear  - 清空缓存");
      Serial.println("help   - 显示帮助");
      Serial.println("================\n");
    }
  }
}
```

### 在 loop() 中调用
```cpp
void loop() {
  // ... 其他代码 ...

  handleSerialCommands();  // 处理串口命令

  // ... 其他代码 ...
}
```

---

## 🎯 使用示例

### 通过串口监视器查看缓存

1. 打开串口监视器（115200波特率）
2. 输入命令：
   ```
   cache    ← 查看缓存
   sync     ← 手动同步
   clear    ← 清空缓存
   help     ← 查看帮助
   ```

3. 输出示例：
```
=== 离线缓存队列 ===
队列数量: 2/50
WiFi状态: 未连接

[交易 #1]
  卡号: 2345408116
  金额: $-5.00
  扣费前: $550.00 → 扣费后: $545.00
  套餐: 5 Min Wash
  时间: 123456 ms

[交易 #2]
  卡号: 1234567890
  金额: $-4.00
  扣费前: $100.00 → 扣费后: $96.00
  套餐: 4 Min Wash
  时间: 234567 ms

总金额: $-9.00
=====================
```

---

## ✅ 最佳实践

1. **定期检查缓存状态**
   - 每天启动时查看 `offlineQueueCount`
   - 如果长期积累，主动同步

2. **监控WiFi状态**
   - 确保WiFi稳定连接
   - WiFi恢复后自动触发同步

3. **备份重要数据**
   - 离线缓存是临时存储
   - 不要长期依赖NVS

4. **调试时使用日志**
   - 保持串口监视器开启
   - 观察同步过程

---

## 📄 相关文档

- [CONFIGMANAGER_GUIDE.md](CONFIGMANAGER_GUIDE.md) - 配置管理指南
- [PRODUCTION_CONFIG_GUIDE.md](PRODUCTION_CONFIG_GUIDE.md) - 生产环境配置

---

**文档版本**: v1.0
**创建时间**: 2025-11-11
**作者**: Claude Code
