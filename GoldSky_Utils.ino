/*
 * GoldSky_Utils.ino
 * 工具函数集合
 *
 * 包含：日志、蜂鸣器、LED控制、UID转换、按钮读取、NFC读卡
 */

// =================== 日志函数 ===================
void logMessage(const String& level, const String& message) {
  Serial.printf("[%lu][%s] %s\n", millis(), level.c_str(), message.c_str());
}

void logInfo(const String& message) { logMessage("INFO", message); }
void logWarn(const String& message) { logMessage("WARN", message); }
void logError(const String& message) { logMessage("ERROR", message); }

// =================== 蜂鸣器函数 ===================
void beepShort() {
  digitalWrite(BUZZER, HIGH);
  delay(100);
  digitalWrite(BUZZER, LOW);
}

void beepSuccess() {
  for(int i = 0; i < 2; i++) {
    digitalWrite(BUZZER, HIGH);
    delay(100);
    digitalWrite(BUZZER, LOW);
    delay(100);
  }
}

void beepError() {
  for(int i = 0; i < 3; i++) {
    digitalWrite(BUZZER, HIGH);
    delay(150);
    digitalWrite(BUZZER, LOW);
    delay(100);
  }
}

// =================== LED控制函数 ===================
// LED输出辅助函数 - 根据极性配置自动反相
inline void ledWrite(int pin, bool state) {
  #if LED_COMMON_ANODE
    digitalWrite(pin, !state);  // 共阳极: 反相输出
  #else
    digitalWrite(pin, state);   // 共阴极: 直接输出
  #endif
}

void updateSingleLED(int pin, LEDStatus status) {
  switch (status) {
    case LED_OFF:
      ledWrite(pin, LOW);
      break;

    case LED_ON:
      ledWrite(pin, HIGH);
      break;

    case LED_BLINK_SLOW:
      if (millis() - ledIndicator.lastBlinkTime > 500) {
        ledIndicator.blinkState = !ledIndicator.blinkState;
        ledIndicator.lastBlinkTime = millis();
      }
      ledWrite(pin, ledIndicator.blinkState ? HIGH : LOW);
      break;

    case LED_BLINK_FAST:
      if (millis() - ledIndicator.lastBlinkTime > 100) {
        ledIndicator.blinkState = !ledIndicator.blinkState;
        ledIndicator.lastBlinkTime = millis();
      }
      ledWrite(pin, ledIndicator.blinkState ? HIGH : LOW);
      break;
  }
}

void updateLEDIndicators() {
  ledWrite(LED_POWER, ledIndicator.power ? HIGH : LOW);
  updateSingleLED(LED_NETWORK, ledIndicator.network);
  updateSingleLED(LED_PROGRESS, ledIndicator.progress);
  updateSingleLED(LED_STATUS, ledIndicator.status);
}

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
    // PROGRESS LED根据状态设置，STATUS被故障占用
  } else {
    // 正常流程的LED状态
    switch (currentState) {
      case STATE_WELCOME:
        ledIndicator.progress = LED_OFF;
        ledIndicator.status = LED_OFF;
        break;

      case STATE_SELECT_PACKAGE:
        ledIndicator.progress = LED_ON;
        ledIndicator.status = LED_OFF;
        break;

      case STATE_CARD_SCAN:
        ledIndicator.progress = LED_ON;
        ledIndicator.status = LED_BLINK_FAST;
        break;

      case STATE_VIP_QUERY:
        ledIndicator.progress = LED_ON;
        ledIndicator.status = LED_BLINK_FAST;
        break;

      case STATE_VIP_DISPLAY:
        ledIndicator.progress = LED_BLINK_SLOW;
        ledIndicator.status = LED_ON;
        break;

      case STATE_SYSTEM_READY:
        ledIndicator.progress = LED_ON;
        ledIndicator.status = LED_ON;
        break;

      case STATE_PROCESSING:
        ledIndicator.power = true;
        ledIndicator.network = LED_ON;
        ledIndicator.progress = LED_ON;
        ledIndicator.status = LED_ON;
        break;

      case STATE_COMPLETE:
        ledIndicator.progress = LED_BLINK_SLOW;
        ledIndicator.status = LED_OFF;
        break;

      case STATE_ERROR:
        ledIndicator.progress = LED_OFF;
        ledIndicator.status = LED_BLINK_FAST;
        break;
    }
  }

  // PROGRESS LED始终根据状态设置（不受NFC故障影响）
  switch (currentState) {
    case STATE_WELCOME:
      ledIndicator.progress = LED_OFF;
      break;
    case STATE_SELECT_PACKAGE:
    case STATE_CARD_SCAN:
    case STATE_VIP_QUERY:
    case STATE_SYSTEM_READY:
    case STATE_PROCESSING:
      ledIndicator.progress = LED_ON;
      break;
    case STATE_VIP_DISPLAY:
    case STATE_COMPLETE:
      ledIndicator.progress = LED_BLINK_SLOW;
      break;
    case STATE_ERROR:
      ledIndicator.progress = LED_OFF;
      break;
  }

  updateLEDIndicators();
}

// =================== UID转换函数 ===================
String hexUIDToDecimal(const String& hexUID) {
  unsigned long decimalValue = 0;
  for (int i = 0; i < hexUID.length(); i++) {
    char c = hexUID.charAt(i);
    decimalValue *= 16;
    if (c >= '0' && c <= '9') {
      decimalValue += c - '0';
    } else if (c >= 'A' && c <= 'F') {
      decimalValue += c - 'A' + 10;
    } else if (c >= 'a' && c <= 'f') {
      decimalValue += c - 'a' + 10;
    }
  }
  return String(decimalValue);
}

String decimalToHexUID(const String& decimalUID) {
  unsigned long decimalValue = decimalUID.toInt();
  String hexUID = String(decimalValue, HEX);
  hexUID.toUpperCase();
  while(hexUID.length() < 8) {
    hexUID = "0" + hexUID;
  }
  return hexUID;
}

// =================== 按钮读取 ===================
bool readButtonImproved(int pin) {
  int pinIndex = (pin == BTN_OK) ? 0 : 1;
  bool reading = digitalRead(pin);

  if (reading != lastButtonState[pinIndex]) {
    lastDebounceTime[pinIndex] = millis();
  }

  if ((millis() - lastDebounceTime[pinIndex]) > 50) {
    if (reading && !buttonPressed[pinIndex]) {
      buttonPressed[pinIndex] = true;
      lastButtonState[pinIndex] = reading;
      logInfo("按钮按下: GPIO" + String(pin));
      return true;
    } else if (!reading) {
      buttonPressed[pinIndex] = false;
    }
  }

  lastButtonState[pinIndex] = reading;
  return false;
}

// =================== NFC健康检查和恢复 ===================
bool checkAndRecoverNFC() {
  byte version = mfrc522.PCD_ReadRegister(mfrc522.VersionReg);

  // 版本号正常（通常是0x91或0x92）
  if (version != 0x00 && version != 0xFF) {
    // 健康，静默返回（避免过多日志）
    return true;
  }

  // NFC模块异常，尝试重新初始化
  logWarn("⚠️ NFC模块异常(版本: 0x" + String(version, HEX) + ")，尝试恢复...");

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

// =================== NFC读卡 ===================
// 全局变量：记录上次健康检查时间和连续失败次数
static unsigned long lastNFCHealthCheck = 0;
static int nfcReadFailCount = 0;

String readCardUID() {
  // 首次调用时初始化健康检查时间戳
  if (lastNFCHealthCheck == 0) {
    lastNFCHealthCheck = millis();
  }

  // 只在以下情况检查NFC健康：
  // 1. 连续读卡失败5次以上
  // 2. 距离上次检查超过60秒
  if (nfcReadFailCount > 5 || (millis() - lastNFCHealthCheck > 60000)) {
    if (!checkAndRecoverNFC()) {
      nfcReadFailCount++;
      return "";
    }
    lastNFCHealthCheck = millis();
    nfcReadFailCount = 0;  // 恢复成功后重置计数
  }

  if (!mfrc522.PICC_IsNewCardPresent()) {
    return "";
  }

  // 调试：检测到卡片存在
  logInfo("🔍 检测到卡片，正在读取...");

  if (!mfrc522.PICC_ReadCardSerial()) {
    nfcReadFailCount++;
    logError("❌ 读取卡片序列号失败");
    return "";
  }

  String hexUID = "";
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    if (mfrc522.uid.uidByte[i] < 0x10) hexUID += "0";
    hexUID += String(mfrc522.uid.uidByte[i], HEX);
  }

  hexUID.toUpperCase();
  String decimalUID = hexUIDToDecimal(hexUID);

  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();

  logInfo("读取到卡片: HEX=" + hexUID + ", DEC=" + decimalUID);

  // 成功读卡后重置失败计数
  nfcReadFailCount = 0;

  return decimalUID;
}
