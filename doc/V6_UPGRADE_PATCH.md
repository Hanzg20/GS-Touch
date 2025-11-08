# 📝 GoldSky_Lite v6.0 升级补丁说明

**当前状态：** 部分完成
**版本：** v5.2 → v6.0
**日期：** 2025-10-30

---

## ✅ 已完成的修改

### 1. 引脚定义更新 ✅
```cpp
// 旧版（v5.2）
#define LED_GREEN 5
#define LED_BLUE 6
#define LED_RED 7
#define PULSE_OUT 4

// 新版（v6.0）✅ 已修改
#define LED_POWER 3      // L1
#define LED_NETWORK 4    // L2
#define LED_PROGRESS 5   // L3
#define LED_STATUS 6     // L4
#define PULSE_OUT 7      // 解决冲突
```

### 2. 状态枚举更新 ✅
```cpp
// 旧版（8状态）
STATE_IDLE, STATE_LANGUAGE_SELECT, STATE_SELECT_PACKAGE,
STATE_CARD_SCAN, STATE_CARD_INFO, STATE_CONFIRM,
STATE_PROCESSING, STATE_COMPLETE, STATE_ERROR

// 新版（6状态）✅ 已修改
STATE_WELCOME = 0,
STATE_SELECT_PACKAGE = 1,
STATE_CARD_SCAN = 2,
STATE_SYSTEM_READY = 3,  // 新增
STATE_PROCESSING = 4,
STATE_COMPLETE = 5,
STATE_ERROR = 99
```

### 3. LED控制函数 ✅
已添加：
- `setLedPower(bool)`
- `setLedNetwork(bool)`
- `setLedProgress(bool)`
- `setLedStatus(bool)`
- `blinkLedProgress(int)`
- `blinkLedStatus(int)`
- `updateLedsByState(SystemState)`

### 4. setup()初始化 ✅
已更新GPIO初始化，使用4个独立LED

### 5. handleWelcomeState() ✅
已重命名并修改，直接进入STATE_SELECT_PACKAGE

---

## ⏳ 待完成的修改

### 1. 删除/注释 handleLanguageSelectState()

**位置：** 第852-869行
**操作：** 删除整个函数或注释掉

```cpp
// 旧代码（需删除）
void handleLanguageSelectState() {
  setLED(false, false, true);
  displayLanguageSelect();
  // ... 17行代码 ...
}

// 新代码（替换为）
// handleLanguageSelectState() - v6.0已移除，直接进入套餐选择
```

### 2. 更新 handleSelectPackageState()

**位置：** 第871-886行
**操作：** 删除 `setLED()` 调用（LED由updateLedsByState统一管理）

```cpp
void handleSelectPackageState() {
  // setLED(false, false, true);  // ❌ 删除这行
  displayPackageSelection();

  if (readButtonImproved(BTN_SELECT)) {
    selectedPackage = (selectedPackage + 1) % PACKAGE_COUNT;
    beepShort();
  }

  if (readButtonImproved(BTN_OK)) {
    currentState = STATE_CARD_SCAN;
    stateStartTime = millis();
    beepShort();
    logInfo("套餐选择: " + String(packages[selectedPackage].name_en));
  }
}
```

### 3. 重写 handleCardScanState()（合并自动处理）

**位置：** 第888-920行
**操作：** 合并 CardInfo + Confirm 逻辑，自动扣款

```cpp
void handleCardScanState() {
  // setLED(false, false, true);  // ❌ 删除（LED由updateLedsByState管理）
  displayCardScan();

  String uid = readCardUID();

  if (uid.length() > 0) {
    beepShort();
    currentCardInfo = getCardInfoFromSupabase(uid);

    // 优化1: 标记成功操作
    lastSuccessfulOperation = millis();
    consecutiveErrors = 0;

    if (currentCardInfo.isValid && currentCardInfo.isActive) {
      if (currentCardInfo.balance >= packages[selectedPackage].price) {
        // =================== v6.0: 自动处理支付 ===================
        const Package& pkg = packages[selectedPackage];
        float amount = pkg.price;
        float balanceBefore = currentCardInfo.balance;
        float balanceAfter = balanceBefore - amount;

        logInfo("🔄 自动处理支付...");
        display.clearBuffer();
        display.setFont(u8g2_font_ncenB10_tr);
        display.drawStr(10, 30, "Processing...");
        display.sendBuffer();

        bool balanceUpdated = updateCardBalance(currentCardInfo.cardUIDDecimal, balanceAfter);
        bool transactionRecorded = recordTransactionToSupabase(
          currentCardInfo.cardUIDDecimal, -amount, balanceBefore, String(pkg.name_en)
        );

        if (balanceUpdated && transactionRecorded) {
          currentCardInfo.balance = balanceAfter;
          currentState = STATE_SYSTEM_READY;  // v6.0: 进入准备状态
          stateStartTime = millis();
          sentPulses = 0;

          lastSuccessfulOperation = millis();
          consecutiveErrors = 0;

          beepSuccess();
          logInfo("✅ 支付成功，进入准备状态");
        } else {
          // 支付失败
          consecutiveErrors++;
          Serial.printf("⚠️ 支付失败，连续错误: %d/%d\n", consecutiveErrors, MAX_CONSECUTIVE_ERRORS);

          displayError("Payment Failed");
          beepError();
          delay(2000);
          resetToIdle();
        }
      } else {
        displayError(TEXT_ERROR_LOW_BALANCE[currentLanguage]);
        beepError();
        delay(2000);
        resetToIdle();
      }
    } else {
      displayError(TEXT_ERROR_INVALID_CARD[currentLanguage]);
      beepError();
      delay(2000);
      resetToIdle();
    }
  }
}
```

### 4. 删除 handleCardInfoState()

**位置：** 第922-931行
**操作：** 删除整个函数

```cpp
// handleCardInfoState() - v6.0已移除，逻辑合并到handleCardScanState()
```

### 5. 删除 handleConfirmState()

**位置：** 第933-975行
**操作：** 删除整个函数

```cpp
// handleConfirmState() - v6.0已移除，逻辑合并到handleCardScanState()
```

### 6. 新增 handleSystemReadyState()

**位置：** 在handleCardScanState()之后添加
**操作：** 添加新函数

```cpp
void handleSystemReadyState() {
  static unsigned long readyStartTime = 0;
  static bool readyInitialized = false;

  if (!readyInitialized) {
    readyStartTime = millis();
    readyInitialized = true;

    // 显示准备界面
    display.clearBuffer();
    display.setFont(u8g2_font_ncenB12_tr);
    display.drawStr(10, 30, "System Ready");
    display.setFont(u8g2_font_ncenB08_tr);
    display.drawStr(15, 50, "Starting...");
    display.sendBuffer();

    Serial.println("🔧 系统准备中...");
  }

  // 1.5秒后自动进入洗车流程
  if (millis() - readyStartTime >= 1500) {
    Serial.println("✅ 系统准备完成，开始洗车");
    beepSuccess();
    currentState = STATE_PROCESSING;
    processingStartTime = millis();
    stateStartTime = millis();
    readyInitialized = false;
  }
}
```

### 7. 更新 handleProcessingState()

**位置：** 第977-1015行
**操作：** 删除 `setLED()` 调用

```cpp
void handleProcessingState() {
  // setLED(false, true, true);  // ❌ 删除这行

  const Package& pkg = packages[selectedPackage];
  unsigned long elapsed = millis() - processingStartTime;
  unsigned long totalTimeMs = pkg.minutes * 60000UL;

  // ... 其余代码保持不变 ...
}
```

### 8. 更新 handleCompleteState()

**位置：** 第1017-1026行
**操作：** 无需修改，保持不变

### 9. 更新 resetToIdle()

**位置：** 搜索 `void resetToIdle()`
**操作：** 重命名为 `resetToWelcome()` 并更新状态

```cpp
// 旧代码
void resetToIdle() {
  currentState = STATE_IDLE;
  // ...
}

// 新代码
void resetToWelcome() {
  currentState = STATE_WELCOME;  // v6.0: 使用新状态名
  stateStartTime = millis();
  selectedPackage = 0;
  cardUID = "";
  currentCardInfo = CardInfo();
  sentPulses = 0;
  logInfo("🔄 系统重置到欢迎界面");
}
```

**全局替换：** `resetToIdle()` → `resetToWelcome()`

### 10. 更新 loop() 主循环

**位置：** 搜索 `switch (currentState)`
**操作：** 更新case语句

```cpp
// 旧代码
switch (currentState) {
  case STATE_IDLE:
    handleIdleState();
    break;
  case STATE_LANGUAGE_SELECT:
    handleLanguageSelectState();
    break;
  case STATE_SELECT_PACKAGE:
    handleSelectPackageState();
    break;
  case STATE_CARD_SCAN:
    handleCardScanState();
    break;
  case STATE_CARD_INFO:
    handleCardInfoState();
    break;
  case STATE_CONFIRM:
    handleConfirmState();
    break;
  case STATE_PROCESSING:
    handleProcessingState();
    break;
  case STATE_COMPLETE:
    handleCompleteState();
    break;
}

// 新代码（v6.0）
switch (currentState) {
  case STATE_WELCOME:
    handleWelcomeState();
    break;

  case STATE_SELECT_PACKAGE:
    handleSelectPackageState();
    break;

  case STATE_CARD_SCAN:
    handleCardScanState();  // 已合并自动处理逻辑
    break;

  case STATE_SYSTEM_READY:
    handleSystemReadyState();  // 新增状态
    break;

  case STATE_PROCESSING:
    handleProcessingState();
    break;

  case STATE_COMPLETE:
    handleCompleteState();
    break;

  default:
    logError("⚠️ 未知状态，重置到欢迎界面");
    resetToWelcome();
    break;
}

// 统一LED控制（每次循环更新）
updateLedsByState(currentState);
```

### 11. 更新超时检测逻辑

**位置：** 搜索 `STATE_TIMEOUT`
**操作：** 更新超时case

```cpp
// 旧代码
switch (currentState) {
  case STATE_LANGUAGE_SELECT: timeout = STATE_TIMEOUT_LANGUAGE_MS; break;
  case STATE_SELECT_PACKAGE: timeout = STATE_TIMEOUT_SELECT_MS; break;
  case STATE_CARD_SCAN: timeout = STATE_TIMEOUT_CARD_SCAN_MS; break;
  case STATE_CARD_INFO: timeout = STATE_TIMEOUT_CARD_INFO_MS; break;
  case STATE_CONFIRM: timeout = STATE_TIMEOUT_CONFIRM_MS; break;
  case STATE_PROCESSING: timeout = STATE_TIMEOUT_PROCESSING_MS; break;
}

// 新代码（v6.0）
switch (currentState) {
  case STATE_WELCOME: timeout = 0; break;  // 欢迎界面无超时
  case STATE_SELECT_PACKAGE: timeout = STATE_TIMEOUT_SELECT_MS; break;
  case STATE_CARD_SCAN: timeout = STATE_TIMEOUT_CARD_SCAN_MS; break;
  case STATE_SYSTEM_READY: timeout = 2000; break;  // 2秒超时（理论上1.5秒自动跳转）
  case STATE_PROCESSING: timeout = STATE_TIMEOUT_PROCESSING_MS; break;
  case STATE_COMPLETE: timeout = STATE_TIMEOUT_COMPLETE_MS; break;
}
```

---

## 🔍 需要全局搜索替换的项

### 搜索并替换：

| 旧代码 | 新代码 | 数量（估计） |
|--------|--------|-------------|
| `STATE_IDLE` | `STATE_WELCOME` | ~10处 |
| `resetToIdle()` | `resetToWelcome()` | ~15处 |
| `handleIdleState()` | `handleWelcomeState()` | 2-3处 |

### 搜索并删除/注释：

| 代码模式 | 操作 |
|---------|------|
| `handleLanguageSelectState()` | 删除整个函数 |
| `handleCardInfoState()` | 删除整个函数 |
| `handleConfirmState()` | 删除整个函数 |
| `case STATE_LANGUAGE_SELECT:` | 删除case |
| `case STATE_CARD_INFO:` | 删除case |
| `case STATE_CONFIRM:` | 删除case |

---

## 📋 快速检查清单

完成以上修改后，检查：

- [ ] 没有编译错误
- [ ] 没有 `STATE_IDLE`、`STATE_LANGUAGE_SELECT`、`STATE_CARD_INFO`、`STATE_CONFIRM` 残留
- [ ] 所有 `setLED(r,g,b)` 调用已删除或保留为兼容
- [ ] loop()末尾添加了 `updateLedsByState(currentState)`
- [ ] setup()中LED初始化使用4个独立LED
- [ ] resetToWelcome()替换了所有resetToIdle()

---

## ⚡ 快速实施命令（手动操作）

1. **全局替换（使用编辑器）：**
   - `STATE_IDLE` → `STATE_WELCOME`
   - `resetToIdle()` → `resetToWelcome()`
   - `handleIdleState` → `handleWelcomeState`

2. **删除函数：**
   - 删除 `handleLanguageSelectState()`（第852-869行）
   - 删除 `handleCardInfoState()`（第922-931行）
   - 删除 `handleConfirmState()`（第933-975行）

3. **重写函数：**
   - 重写 `handleCardScanState()`（合并支付逻辑）

4. **新增函数：**
   - 添加 `handleSystemReadyState()`

5. **更新loop()：**
   - 修改switch case语句
   - 末尾添加 `updateLedsByState(currentState)`

---

## 🧪 测试步骤

完成后测试：

1. **编译测试**：无错误和警告
2. **LED测试**：启动时4个LED依次点亮
3. **欢迎界面**：按OK直接进入套餐选择
4. **刷卡测试**：刷卡后自动扣款，进入准备状态
5. **系统准备**：显示1.5秒后自动进入洗车
6. **脉冲输出**：GPIO 7输出脉冲
7. **完成返回**：8秒后返回欢迎界面

---

**文档版本：** v1.0
**创建日期：** 2025-10-30
**状态：** 部分完成，等待手动实施剩余修改
