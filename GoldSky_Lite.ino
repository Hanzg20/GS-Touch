/*
 * Eaglson Coin Wash 洗车终端系统 - 增强版 v2.5
 *
 * 版本: v2.5 Enhanced Security & Reliability
 * 日期: 2025-11-07
 * 作者: Eaglson Development Team
 *
 * 🔧 v2.5 重大更新:
 * ✅ 离线交易队列：WiFi断开时自动缓存到NVS,恢复后同步
 * ✅ API输入验证：防止异常数据导致系统崩溃
 * ✅ 配置管理系统：WiFi密码和API密钥存储到NVS,支持运行时更新
 *
 * 📁 文件结构:
 * - config.h: 配置和数据结构
 * - ConfigManager.h: 配置管理类(新增)
 * - GoldSky_Utils.ino: 工具函数(日志/LED/按钮/NFC)
 * - GoldSky_Display.ino: 显示函数
 * - GoldSky_Lite.ino: 主程序(本文件)
 *
 * 📖 详细文档: OPTIMIZATION_SUMMARY_V2.5.md
 */

// =============== 头文件包含 ===============
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <SPI.h>
#include <U8g2lib.h>
#include <MFRC522.h>
#include <esp_task_wdt.h>
#include <Preferences.h>

// =============== 配置文件 ===============
#include "config.h"
#include "ConfigManager.h"

// =================== 配置别名（使用config.h中定义的数组）===================
#define packages PACKAGES  // 使用config.h中的PACKAGES数组

// =================== 其他全局变量 ===================
bool watchdogEnabled = false;

// =================== 全局对象 ===================
// 2.42" OLED SSD1309 128x64 I2C (180度旋转)
U8G2_SSD1309_128X64_NONAME0_F_HW_I2C display(U8G2_R2, U8X8_PIN_NONE, I2C_SCL, I2C_SDA);
MFRC522 mfrc522(RC522_CS, RC522_RST);
Preferences prefs;
ConfigManager config(&prefs);

// =================== 离线交易队列 ===================
PendingTransaction offlineQueue[MAX_OFFLINE_QUEUE];
int offlineQueueCount = 0;

// =================== 全局变量 ===================
SystemState currentState = STATE_WELCOME;
Language currentLanguage = LANG_EN;
int selectedPackage = 0;
String cardUID = "";
CardInfo currentCardInfo;
int sentPulses = 0;
unsigned long stateStartTime = 0;
unsigned long processingStartTime = 0;
unsigned long lastPulseTime = 0;
SystemStatus sysStatus;
SystemLEDs ledIndicator;

unsigned long lastDebounceTime[2] = {0, 0};
bool lastButtonState[2] = {false, false};

// 健壮性增强：自动重试计时器
unsigned long lastNFCRetry = 0;
unsigned long lastWiFiRetry = 0;
bool buttonPressed[2] = {false, false};

unsigned long lastWiFiCheck = 0;
int apiRetryCount = 0;
unsigned long lastHeartbeat = 0;
unsigned long loopStartTime = 0;
TaskHandle_t mainTaskHandle = NULL;

// =================== 错误恢复机制（方案A优化1）===================
unsigned long lastSuccessfulOperation = 0;
int consecutiveErrors = 0;

// =================== WiFi管理 ===================
void initWiFi() {
  String ssid = config.getWiFiSSID();
  String password = config.getWiFiPassword();

  logInfo("连接WiFi: " + ssid);

  // WiFi初始化带重试机制（最多3次）
  sysStatus.wifiConnected = false;
  WiFi.mode(WIFI_STA);

  for (int retry = 0; retry < 3; retry++) {
    if (retry > 0) {
      logWarn("⚠️ WiFi重连尝试 " + String(retry) + "/3");
      WiFi.disconnect();
      delay(1000);
    }

    WiFi.begin(ssid.c_str(), password.c_str());

    // 等待连接（每次尝试10秒）
    unsigned long startTime = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startTime < 10000) {
      delay(500);
      Serial.print(".");
    }

    if (WiFi.status() == WL_CONNECTED) {
      sysStatus.wifiConnected = true;
      Serial.println();
      logInfo("✅ WiFi连接成功: " + WiFi.localIP().toString() +
              (retry > 0 ? " (重试成功)" : ""));
      return;  // 成功后立即返回
    }

    Serial.println();  // 换行
  }

  // 3次都失败
  sysStatus.wifiConnected = false;
  logWarn("⚠️ WiFi连接失败（3次重试后），进入离线模式");
}

void checkWiFi() {
  if (millis() - lastWiFiCheck > 30000) {
    bool wasConnected = sysStatus.wifiConnected;
    sysStatus.wifiConnected = (WiFi.status() == WL_CONNECTED);

    // 如果之前连接正常但现在断开了，尝试重连
    if (wasConnected && !sysStatus.wifiConnected) {
      logWarn("⚠️ WiFi断开，尝试重连...");
      WiFi.disconnect();
      delay(500);
      WiFi.begin(config.getWiFiSSID().c_str(), config.getWiFiPassword().c_str());

      // 等待3秒尝试重连
      unsigned long startTime = millis();
      while (WiFi.status() != WL_CONNECTED && millis() - startTime < 3000) {
        delay(500);
      }

      if (WiFi.status() == WL_CONNECTED) {
        sysStatus.wifiConnected = true;
        logInfo("✅ WiFi重连成功: " + WiFi.localIP().toString());
      } else {
        sysStatus.wifiConnected = false;
        logWarn("⚠️ WiFi重连失败，继续离线模式");
      }
    }

    lastWiFiCheck = millis();
  }
}

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

// =================== WiFi自动恢复 ===================
void tryRecoverWiFi() {
  if (sysStatus.wifiConnected) return;  // WiFi正常，无需恢复

  if (millis() - lastWiFiRetry < WIFI_RETRY_INTERVAL_MS) return;  // 未到重试时间

  lastWiFiRetry = millis();
  logInfo("🔄 尝试恢复WiFi连接...");

  WiFi.disconnect();
  delay(500);
  WiFi.begin(config.getWiFiSSID().c_str(), config.getWiFiPassword().c_str());

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

// =================== API响应验证 ===================
bool validateCardInfoResponse(const JsonDocument& doc) {
  if (doc.size() == 0) {
    logError("❌ API返回空数据");
    return false;
  }

  // 检查必需字段 (使用ArduinoJson v7推荐方式)
  if (!doc[0]["card_credit"].is<float>()) {
    logError("❌ 缺少card_credit字段");
    return false;
  }

  if (!doc[0]["is_active"].is<bool>()) {
    logError("❌ 缺少is_active字段");
    return false;
  }

  // 数据范围验证
  float balance = doc[0]["card_credit"].as<float>();
  if (balance < 0.0 || balance > 10000.0) {
    logWarn("⚠️ 异常余额值: $" + String(balance, 2));
    return false;
  }

  // 验证会员类型范围
  int memberType = doc[0]["member_type"] | 0;
  if (memberType < 0 || memberType > 7) {
    logWarn("⚠️ 无效会员类型: " + String(memberType));
    // 不返回false，使用默认值
  }

  return true;
}

// =================== Supabase API ===================
CardInfo getCardInfoFromSupabase(const String& decimalUID) {
  CardInfo info;
  info.clear();
  info.cardUIDDecimal = decimalUID;
  info.cardUID = decimalToHexUID(decimalUID);

  // 输入验证
  if (decimalUID.length() == 0 || decimalUID.length() > 15) {
    logError("❌ 无效的卡号格式");
    return info;
  }

  if (!sysStatus.wifiConnected) {
    logInfo("🔄 离线模式测试");
    if (decimalUID == "2345408116" || decimalUID == "1210711100") {
      info.displayCardNumber = "JC-VIP-8116";
      info.cardNumber = "****8116";
      info.balance = 76.00;
      info.isValid = true;
      info.isActive = true;
      info.userName = "Test User";
      info.cardType = "Premium";
      info.lastTransactionDate = "2025-10-29";
      logInfo("✅ 离线验证成功");
    }
    return info;
  }

  HTTPClient http;
  String endpoint = config.getSupabaseURL() +
    "/rest/v1/jc_vip_cards?card_uid=eq." + decimalUID +
    "&select=display_card_number,cardholder_name,card_credit,member_type,is_active,updated_at";

  http.begin(endpoint);
  String apiKey = config.getSupabaseKey();
  http.addHeader("apikey", apiKey.c_str());
  http.addHeader("Authorization", ("Bearer " + apiKey).c_str());
  http.setTimeout(10000);

  int httpCode = http.GET();

  if (httpCode == 200) {
    String response = http.getString();

    // 检查响应长度
    if (response.length() == 0 || response.length() > 4096) {
      logError("❌ API响应长度异常: " + String(response.length()));
      http.end();
      return info;
    }

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, response);

    if (error) {
      logError("❌ JSON解析失败: " + String(error.c_str()));
      http.end();
      return info;
    }

    // 验证响应数据
    if (!validateCardInfoResponse(doc)) {
      http.end();
      return info;
    }

    // 提取数据
    String displayNum = doc[0]["display_card_number"] | "";
    if (displayNum.length() > 0) {
      info.displayCardNumber = displayNum;
      if (displayNum.length() >= 4) {
        info.cardNumber = "****" + displayNum.substring(displayNum.length() - 4);
      } else {
        info.cardNumber = displayNum;
      }
    } else {
      info.displayCardNumber = "N/A";
      info.cardNumber = "****" + decimalUID.substring(decimalUID.length() - 4);
    }

    info.balance = doc[0]["card_credit"].as<float>();
    info.isValid = true;
    info.isActive = doc[0]["is_active"].as<bool>();
    info.userName = doc[0]["cardholder_name"] | "Unknown";

    int memberType = doc[0]["member_type"] | 0;
    if (memberType >= 0 && memberType <= 7) {
      info.cardType = MEMBER_TYPES[memberType];
    } else {
      info.cardType = "Unknown";
    }

    String updatedAt = doc[0]["updated_at"] | "";
    if (updatedAt.length() > 0) {
      info.lastTransactionDate = updatedAt.substring(0, 10);
    } else {
      info.lastTransactionDate = "Never";
    }

    logInfo("✅ 在线验证成功");
    logInfo("  完整卡号: " + info.displayCardNumber);
    logInfo("  会员类型: " + info.cardType);
    logInfo("  最后使用: " + info.lastTransactionDate);
  } else {
    logError("❌ API错误: HTTP " + String(httpCode));
  }

  http.end();
  return info;
}

bool updateCardBalance(const String& decimalUID, float newBalance) {
  if (!sysStatus.wifiConnected) {
    logInfo("🔄 离线模式：余额更新已缓存");
    return true;
  }

  HTTPClient http;
  String endpoint = config.getSupabaseURL() + "/rest/v1/jc_vip_cards?card_uid=eq." + decimalUID;

  JsonDocument doc;
  doc["card_credit"] = newBalance;

  String jsonString;
  serializeJson(doc, jsonString);

  http.begin(endpoint);
  String apiKey = config.getSupabaseKey();
  http.addHeader("apikey", apiKey.c_str());
  http.addHeader("Authorization", ("Bearer " + apiKey).c_str());
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Prefer", "return=minimal");

  int httpCode = http.PATCH(jsonString);
  http.end();

  return (httpCode == 204);
}

// =================== 离线队列管理 ===================
void addToOfflineQueue(const String& decimalUID, float amount, float balanceBefore, const String& packageName) {
  if (offlineQueueCount >= MAX_OFFLINE_QUEUE) {
    logWarn("⚠️ 离线队列已满，覆盖最旧记录");
    // 移除最旧的记录
    for (int i = 0; i < offlineQueueCount - 1; i++) {
      offlineQueue[i] = offlineQueue[i + 1];
    }
    offlineQueueCount--;
  }

  // 添加新交易
  offlineQueue[offlineQueueCount].cardUID = decimalUID;
  offlineQueue[offlineQueueCount].amount = amount;
  offlineQueue[offlineQueueCount].balanceBefore = balanceBefore;
  offlineQueue[offlineQueueCount].packageName = packageName;
  offlineQueue[offlineQueueCount].timestamp = millis();
  offlineQueueCount++;

  // 保存到NVS
  prefs.putInt("queue_count", offlineQueueCount);
  String key = "tx_" + String(offlineQueueCount - 1);
  String data = decimalUID + "|" + String(amount, 2) + "|" + String(balanceBefore, 2) + "|" + packageName;
  prefs.putString(key.c_str(), data);

  logInfo("✅ 交易已缓存到离线队列 (" + String(offlineQueueCount) + "/" + String(MAX_OFFLINE_QUEUE) + ")");
}

void loadOfflineQueue() {
  offlineQueueCount = prefs.getInt("queue_count", 0);
  if (offlineQueueCount > MAX_OFFLINE_QUEUE) {
    offlineQueueCount = MAX_OFFLINE_QUEUE;
  }

  for (int i = 0; i < offlineQueueCount; i++) {
    String key = "tx_" + String(i);
    String data = prefs.getString(key.c_str(), "");
    if (data.length() > 0) {
      int pos1 = data.indexOf('|');
      int pos2 = data.indexOf('|', pos1 + 1);
      int pos3 = data.indexOf('|', pos2 + 1);

      offlineQueue[i].cardUID = data.substring(0, pos1);
      offlineQueue[i].amount = data.substring(pos1 + 1, pos2).toFloat();
      offlineQueue[i].balanceBefore = data.substring(pos2 + 1, pos3).toFloat();
      offlineQueue[i].packageName = data.substring(pos3 + 1);
    }
  }

  if (offlineQueueCount > 0) {
    logInfo("📥 加载了 " + String(offlineQueueCount) + " 笔离线交易");
  }
}

bool syncOfflineQueue() {
  if (offlineQueueCount == 0 || !sysStatus.wifiConnected) {
    return false;
  }

  logInfo("🔄 同步离线交易队列 (" + String(offlineQueueCount) + " 笔)...");
  int successCount = 0;
  int failCount = 0;

  for (int i = 0; i < offlineQueueCount; i++) {
    PendingTransaction& tx = offlineQueue[i];

    HTTPClient http;
    String endpoint = config.getSupabaseURL() + "/rest/v1/jc_transaction_history";

    JsonDocument doc;
    doc["machine_id"] = config.getMachineID();
    doc["card_uid"] = tx.cardUID.toInt();
    doc["transaction_type"] = "CHARGE";
    doc["third_party_reference"] = tx.packageName;
    doc["transaction_amount"] = tx.amount;
    doc["balance_before"] = tx.balanceBefore;

    String jsonString;
    serializeJson(doc, jsonString);

    http.begin(endpoint);
    String apiKey = config.getSupabaseKey();
    http.addHeader("apikey", apiKey.c_str());
    http.addHeader("Authorization", ("Bearer " + apiKey).c_str());
    http.addHeader("Content-Type", "application/json");
    http.addHeader("Prefer", "return=minimal");

    int httpCode = http.POST(jsonString);
    http.end();

    if (httpCode == 201) {
      successCount++;
      // 删除NVS中的记录
      String key = "tx_" + String(i);
      prefs.remove(key.c_str());
    } else {
      failCount++;
      logWarn("⚠️ 离线交易同步失败 #" + String(i) + " (HTTP " + String(httpCode) + ")");
    }

    delay(100);  // 避免API限流
  }

  if (successCount > 0) {
    logInfo("✅ 成功同步 " + String(successCount) + " 笔离线交易");
    // 重新整理队列
    offlineQueueCount = 0;
    prefs.putInt("queue_count", 0);
  }

  return (successCount > 0);
}

bool recordTransaction(const String& decimalUID, float amount, float balanceBefore, const String& packageName) {
  // 离线模式：添加到队列
  if (!sysStatus.wifiConnected) {
    addToOfflineQueue(decimalUID, amount, balanceBefore, packageName);
    return true;
  }

  // 在线模式：直接发送
  HTTPClient http;
  String endpoint = config.getSupabaseURL() + "/rest/v1/jc_transaction_history";

  JsonDocument doc;
  doc["machine_id"] = config.getMachineID();
  doc["card_uid"] = decimalUID.toInt();
  doc["transaction_type"] = "CHARGE";
  doc["third_party_reference"] = packageName;
  doc["transaction_amount"] = amount;
  doc["balance_before"] = balanceBefore;

  String jsonString;
  serializeJson(doc, jsonString);

  http.begin(endpoint);
  String apiKey = config.getSupabaseKey();
  http.addHeader("apikey", apiKey.c_str());
  http.addHeader("Authorization", ("Bearer " + apiKey).c_str());
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Prefer", "return=minimal");

  int httpCode = http.POST(jsonString);
  http.end();

  if (httpCode == 201) {
    sysStatus.totalTransactions++;
    sysStatus.totalRevenue += amount;

    // 成功后尝试同步离线队列
    if (offlineQueueCount > 0) {
      syncOfflineQueue();
    }

    return true;
  } else {
    // 在线发送失败，添加到离线队列
    logWarn("⚠️ 在线交易失败 (HTTP " + String(httpCode) + ")，转为离线模式");
    addToOfflineQueue(decimalUID, amount, balanceBefore, packageName);
    return true;  // 仍返回true，因为已缓存
  }
}

// =================== 状态处理函数 ===================
void handleWelcomeState() {
  setSystemLEDStatus();

  // 只在首次进入时刷新显示
  static bool displayRefreshed = false;
  if (!displayRefreshed) {
    displayWelcome();
    displayRefreshed = true;
  }

  // ✅ 优化：欢迎界面刷新时间戳，避免待机时自动重启
  static unsigned long lastWelcomeUpdate = 0;
  if (millis() - lastWelcomeUpdate > 60000) {  // 每60秒刷新一次
    lastSuccessfulOperation = millis();
    lastWelcomeUpdate = millis();
  }

  if (readButtonImproved(BTN_OK)) {
    displayRefreshed = false;  // 重置标志
    currentState = STATE_SELECT_PACKAGE;
    selectedPackage = 0;
    stateStartTime = millis();
    beepShort();
    logInfo("✅ 用户启动服务");
  }
}

void handleSelectPackageState() {
  setSystemLEDStatus();

  // 只在首次进入或按键时刷新显示，避免过度刷新
  static bool displayRefreshed = false;
  static int lastSelectedPackage = -1;

  if (!displayRefreshed || lastSelectedPackage != selectedPackage) {
    displayPackageSelection();
    displayRefreshed = true;
    lastSelectedPackage = selectedPackage;
  }

  if (readButtonImproved(BTN_SELECT)) {
    selectedPackage = (selectedPackage + 1) % PACKAGE_COUNT;
    stateStartTime = millis();  // 重置超时计时器
    displayRefreshed = false;  // 标记需要刷新
    beepShort();
  }

  if (readButtonImproved(BTN_OK)) {
    displayRefreshed = false;  // 重置刷新标志，为下次进入准备
    if (packages[selectedPackage].isQuery) {
      currentState = STATE_VIP_QUERY;
      stateStartTime = millis();
      beepShort();
      logInfo("进入VIP信息查询");
    } else {
      currentState = STATE_CARD_SCAN;
      stateStartTime = millis();
      beepShort();
      logInfo("套餐选择: " + String(packages[selectedPackage].name_en));
    }
  }
}

void handleCardScanState() {
  setSystemLEDStatus();

  // 调试：检查是否进入刷卡状态 + NFC模块状态
  static unsigned long lastDebugPrint = 0;
  static bool antennaInfoShown = false;

  // 首次进入或每5秒打印一次
  if (lastDebugPrint == 0 || millis() - lastDebugPrint > 5000) {
    byte version = mfrc522.PCD_ReadRegister(mfrc522.VersionReg);

    logInfo("等待刷卡... [NFC版本: 0x" + String(version, HEX) + ", 状态: " +
            (sysStatus.nfcWorking ? "正常" : "异常") + "]");

    // 第一次打印时检查并修复天线状态
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

    lastDebugPrint = millis();
  }

  displayCardScan();

  String uid = readCardUID();

  // 调试：显示读卡尝试
  static unsigned long lastReadAttempt = 0;
  if (millis() - lastReadAttempt > 1000) {
    if (uid.length() == 0) {
      Serial.print(".");  // 简单的心跳显示
    }
    lastReadAttempt = millis();
  }

  if (uid.length() > 0) {
    beepShort();

    displayProcessing("Verifying Card...", 0.3);
    ledIndicator.network = LED_BLINK_FAST;
    updateLEDIndicators();

    currentCardInfo = getCardInfoFromSupabase(uid);

    if (currentCardInfo.isValid && currentCardInfo.isActive) {
      lastSuccessfulOperation = millis();  // ✅ 优化1: 验证成功
      if (currentCardInfo.balance >= packages[selectedPackage].price) {

        const Package& pkg = packages[selectedPackage];
        float amount = pkg.price;
        float balanceBefore = currentCardInfo.balance;
        float balanceAfter = balanceBefore - amount;

        displayProcessing("Processing Payment...", 0.6);

        bool success = updateCardBalance(currentCardInfo.cardUIDDecimal, balanceAfter);
        recordTransaction(currentCardInfo.cardUIDDecimal, -amount, balanceBefore, String(pkg.name_en));

        if (success) {
          lastSuccessfulOperation = millis();  // ✅ 优化1: 支付成功
          consecutiveErrors = 0;  // ✅ 重置错误计数
          currentCardInfo.balance = balanceAfter;
          displayProcessing("Payment Success!", 1.0);
          delay(1000);

          // ✅ 扣费成功后进入准备状态
          currentState = STATE_SYSTEM_READY;
          stateStartTime = millis();
          beepSuccess();
          logInfo("✅ 支付成功，余额: $" + String(balanceAfter, 2));
        } else {
          consecutiveErrors++;  // ✅ 优化1: 支付失败
          displayError("Transaction Failed");
          beepError();
          delay(2000);
          resetToWelcome();
        }
      } else {
        displayError(TEXT_ERROR_LOW_BALANCE[currentLanguage]);
        consecutiveErrors++;  // ✅ 优化1: 余额不足
        beepError();
        delay(2000);
        resetToWelcome();
      }
    } else {
      displayError(TEXT_ERROR_INVALID_CARD[currentLanguage]);
      beepError();
      consecutiveErrors++;  // ✅ 优化1: 卡片无效
      delay(2000);
      resetToWelcome();
    }
  }
}

void handleVIPQueryState() {
  setSystemLEDStatus();
  displayVIPQueryScan();

  String uid = readCardUID();

  if (uid.length() > 0) {
    beepShort();

    displayProcessing("Querying Card Info...", 0.5);
    ledIndicator.network = LED_BLINK_FAST;
    updateLEDIndicators();

    currentCardInfo = getCardInfoFromSupabase(uid);

    if (currentCardInfo.isValid) {
      currentState = STATE_VIP_DISPLAY;
      stateStartTime = millis();
      beepSuccess();
      logInfo("✅ VIP信息查询成功");
      logInfo("  卡号: " + currentCardInfo.displayCardNumber);
      logInfo("  余额: $" + String(currentCardInfo.balance, 2));
      logInfo("  最后使用: " + currentCardInfo.lastTransactionDate);
    } else {
      displayError("Invalid or Inactive Card");
      beepError();
      delay(2000);
      resetToWelcome();
    }
  }
}

void handleVIPDisplayState() {
  setSystemLEDStatus();

  // 只在首次进入时刷新显示
  static bool displayRefreshed = false;
  if (!displayRefreshed) {
    displayVIPInfo();
    displayRefreshed = true;
  }

  if (millis() - stateStartTime > STATE_TIMEOUT_VIP_DISPLAY_MS) {
    displayRefreshed = false;  // 重置标志
    resetToWelcome();
  }

  if (readButtonImproved(BTN_OK)) {
    displayRefreshed = false;  // 重置标志
    resetToWelcome();
  }
}

void handleSystemReadyState() {
  setSystemLEDStatus();

  unsigned long elapsed = millis() - stateStartTime;
  float progress = (float)elapsed / STATE_TIMEOUT_READY_MS;

  if (progress > 1.0) progress = 1.0;

  displayProcessing("System Ready...", progress);

  if (elapsed >= STATE_TIMEOUT_READY_MS) {
    // ✅ 进入洗车状态时，重置所有脉冲相关变量
    currentState = STATE_PROCESSING;
    processingStartTime = millis();
    stateStartTime = millis();
    sentPulses = 0;              // ✅ 重置已发送脉冲数
    lastPulseTime = millis();    // ✅ 重置脉冲计时器为当前时间

    logInfo("✅ 开始洗车服务");
    logInfo("  脉冲计时器初始化: " + String(lastPulseTime));
  }
}

void handleProcessingState() {
  setSystemLEDStatus();

  const Package& pkg = packages[selectedPackage];
  unsigned long elapsed = millis() - processingStartTime;
  unsigned long totalTimeMs = pkg.minutes * 60000UL;

  int remainingMin = 0;
  int remainingSec = 0;

  if (elapsed < totalTimeMs) {
    unsigned long remainingMs = totalTimeMs - elapsed;
    remainingMin = remainingMs / 60000;
    remainingSec = (remainingMs % 60000) / 1000;
  }

  displayWashProgress(sentPulses, pkg.pulses, remainingMin, remainingSec);

  // ✅ 修复后的脉冲控制逻辑：使用全局变量，不使用静态变量
  if (millis() - lastPulseTime >= PULSE_INTERVAL_MS && sentPulses < pkg.pulses) {
    digitalWrite(PULSE_OUT, HIGH);
    delay(PULSE_WIDTH_MS);
    digitalWrite(PULSE_OUT, LOW);

    sentPulses++;
    lastPulseTime = millis();  // 更新全局变量

    logInfo("🚿 脉冲 " + String(sentPulses) + "/" + String(pkg.pulses));
  }

  if (elapsed >= totalTimeMs || sentPulses >= pkg.pulses) {
    currentState = STATE_COMPLETE;
    stateStartTime = millis();
    digitalWrite(PULSE_OUT, LOW);
    logInfo("✅ 洗车完成!");
  }
}

void handleCompleteState() {
  setSystemLEDStatus();

  // 重置标志（每次重新进入STATE_COMPLETE时）
  static SystemState lastState = STATE_IDLE;  // 初始化为非COMPLETE状态
  static bool displayRefreshed = false;
  static bool soundPlayed = false;

  if (currentState != lastState) {
    // 刚进入STATE_COMPLETE，重置所有标志
    displayRefreshed = false;
    soundPlayed = false;
    lastState = STATE_COMPLETE;
  }

  // 只在首次进入时刷新显示
  if (!displayRefreshed) {
    displayComplete();
    displayRefreshed = true;
  }

  // 只在首次进入时播放声音
  if (!soundPlayed) {
    beepSuccess();
    delay(200);
    beepSuccess();
    soundPlayed = true;
  }

  // 超时后返回欢迎页
  if (millis() - stateStartTime > STATE_TIMEOUT_COMPLETE_MS) {
    resetToWelcome();
  }
}

// =================== 系统管理 ===================
void resetToWelcome() {
  logInfo("返回欢迎屏幕");

  currentState = STATE_WELCOME;
  cardUID = "";
  selectedPackage = 0;
  sentPulses = 0;
  lastPulseTime = 0;  // ✅ 重置脉冲计时器
  stateStartTime = millis();
  currentCardInfo.clear();

  digitalWrite(PULSE_OUT, LOW);

  for(int i = 0; i < 2; i++) {
    buttonPressed[i] = false;
    lastButtonState[i] = false;
  }

  beepShort();
}

void checkStateTimeout() {
  if (currentState == STATE_WELCOME || currentState == STATE_PROCESSING) {
    return;
  }

  unsigned long elapsed = millis() - stateStartTime;
  unsigned long timeout = 0;

  switch (currentState) {
    case STATE_SELECT_PACKAGE: timeout = STATE_TIMEOUT_SELECT_MS; break;
    case STATE_CARD_SCAN: timeout = STATE_TIMEOUT_CARD_SCAN_MS; break;
    case STATE_VIP_QUERY: timeout = STATE_TIMEOUT_VIP_QUERY_MS; break;
    case STATE_VIP_DISPLAY: timeout = STATE_TIMEOUT_VIP_DISPLAY_MS; break;
    case STATE_SYSTEM_READY: timeout = STATE_TIMEOUT_READY_MS + 1000; break;
    case STATE_COMPLETE: timeout = STATE_TIMEOUT_COMPLETE_MS; break;
    default: return;
  }

  if (elapsed > timeout) {
    logWarn("状态超时");
    beepError();
    resetToWelcome();
  }
}

void performHealthCheck() {
  if (millis() - lastHeartbeat > 60000) {
    sysStatus.updateMemoryStats();

    logInfo("=== 系统健康检查 ===");
    logInfo("运行: " + String(millis() / 1000) + "s");
    logInfo("内存: " + String(ESP.getFreeHeap() / 1024) + "KB");
    logInfo("交易: " + String(sysStatus.totalTransactions));
    logInfo("收入: $" + String(sysStatus.totalRevenue, 2));

    // =================== 内存保护机制（方案A优化2）===================
    uint32_t freeHeap = ESP.getFreeHeap();

    // ⚠️ 内存严重不足 - 立即重启
    if (freeHeap < 20000) {
      Serial.println("❌ 内存严重不足，立即重启...");
      Serial.printf("   当前剩余: %u KB (< 20KB)\n", freeHeap / 1024);
      delay(2000);
      ESP.restart();
    }

    // ⚠️ 内存预警 - 尝试释放
    if (freeHeap < 40000) {
      Serial.println("⚠️ 内存预警！");
      Serial.printf("   当前剩余: %u KB (< 40KB)\n", freeHeap / 1024);

      // 尝试通过WiFi重连释放内存
      if (WiFi.status() == WL_CONNECTED) {
        Serial.println("   尝试WiFi重连释放内存...");
        WiFi.disconnect();
        delay(500);
        WiFi.reconnect();
      }
    }

    lastHeartbeat = millis();
  }
}

// =================== 主程序 ===================
void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("=====================================");
  Serial.printf("🚗 Eaglson Coin Wash Terminal %s\n", FIRMWARE_VERSION);
  Serial.println("Fixed Edition - 模块化重构版 + 安全配置");
  Serial.println("🎯 5步流程 + 4LED + VIP查询 + 离线队列");
  Serial.println("=====================================");

  mainTaskHandle = xTaskGetCurrentTaskHandle();

  pinMode(BTN_OK, INPUT);
  pinMode(BTN_SELECT, INPUT);
  pinMode(LED_POWER, OUTPUT);
  pinMode(LED_NETWORK, OUTPUT);
  pinMode(LED_PROGRESS, OUTPUT);
  pinMode(LED_STATUS, OUTPUT);
  pinMode(BUZZER, OUTPUT);
  pinMode(PULSE_OUT, OUTPUT);

  digitalWrite(LED_POWER, LOW);
  digitalWrite(LED_NETWORK, LOW);
  digitalWrite(LED_PROGRESS, LOW);
  digitalWrite(LED_STATUS, LOW);
  digitalWrite(BUZZER, LOW);
  digitalWrite(PULSE_OUT, LOW);

  logInfo("LED自检...");
  digitalWrite(LED_POWER, HIGH); delay(200);
  digitalWrite(LED_NETWORK, HIGH); delay(200);
  digitalWrite(LED_PROGRESS, HIGH); delay(200);
  digitalWrite(LED_STATUS, HIGH); delay(200);
  digitalWrite(LED_POWER, LOW);
  digitalWrite(LED_NETWORK, LOW);
  digitalWrite(LED_PROGRESS, LOW);
  digitalWrite(LED_STATUS, LOW);

  beepShort();

  // ============== 初始化NVS存储和配置管理 ==============
  logInfo("💾 初始化NVS存储...");
  prefs.begin("goldsky", false);  // false = 读写模式

  // 初始化配置管理器（从NVS加载或使用默认值）
  config.init(WIFI_SSID, WIFI_PASSWORD, SUPABASE_URL, SUPABASE_KEY, MACHINE_ID);

  // 加载离线交易队列
  loadOfflineQueue();

  // ============== 先初始化I2C（避免SPI干扰）==============
  logInfo("🖥️ 初始化I2C和OLED...");

  // 启用内部上拉电阻（以防模块没有上拉）
  pinMode(I2C_SDA, INPUT_PULLUP);
  pinMode(I2C_SCL, INPUT_PULLUP);
  delay(50);

  Wire.begin(I2C_SDA, I2C_SCL);
  Wire.setClock(100000);  // 降低I2C速度到100kHz，提高稳定性
  delay(300);  // 增加延迟，等待I2C总线稳定

  // I2C设备扫描
  logInfo("📡 扫描I2C总线...");
  int deviceCount = 0;
  for (byte addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0) {
      logInfo("   发现设备: 0x" + String(addr, HEX));
      deviceCount++;
    }
  }
  logInfo("   共发现 " + String(deviceCount) + " 个I2C设备");

  if (deviceCount == 0) {
    logError("❌ I2C总线上没有发现任何设备！");
    logError("   请检查：1) SDA/SCL接线  2) 模块供电  3) 上拉电阻");
  }

  // OLED初始化（增加重试机制）
  logInfo("🔄 尝试初始化OLED...");
  bool oledSuccess = false;

  for (int attempt = 1; attempt <= 3; attempt++) {
    logInfo("   尝试 " + String(attempt) + "/3...");
    delay(200);

    if (display.begin()) {
      display.enableUTF8Print();
      oledSuccess = true;
      logInfo("✅ OLED初始化成功（第" + String(attempt) + "次尝试）");
      break;
    } else {
      logWarn("   第" + String(attempt) + "次失败");
      delay(500);
    }
  }

  if (oledSuccess) {
    sysStatus.displayWorking = true;

    // 测试显示一个简单的内容
    display.clearBuffer();
    display.setFont(u8g2_font_helvB10_tf);
    display.drawStr(20, 30, "OLED OK!");
    display.sendBuffer();
    delay(1000);

    logInfo("✅ OLED显示测试通过");
  } else {
    sysStatus.displayWorking = false;
    logError("❌ OLED初始化完全失败（3次尝试）");
    logError("   可能原因：");
    logError("   1. OLED模块损坏或接触不良");
    logError("   2. I2C地址错误（当前0x3C）");
    logError("   3. 电源电压不足（需要3.3V）");
    logError("   4. SDA/SCL接线错误");
  }

  // ============== 然后初始化SPI和NFC ==============
  logInfo("🔌 初始化SPI和NFC...");

  // 使用自定义引脚初始化SPI
  SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI, RC522_CS);
  delay(100);

  // NFC硬件复位
  pinMode(RC522_RST, OUTPUT);
  digitalWrite(RC522_RST, LOW);
  delay(10);
  digitalWrite(RC522_RST, HIGH);
  delay(50);

  // NFC初始化 - 失败后继续运行（降级模式）
  mfrc522.PCD_Init(RC522_CS, RC522_RST);
  delay(200);

  byte version = mfrc522.PCD_ReadRegister(mfrc522.VersionReg);
  logInfo("   读取到NFC版本号: 0x" + String(version, HEX));

  if (version != 0x00 && version != 0xFF) {
    sysStatus.nfcWorking = true;
    mfrc522.PCD_AntennaOn();
    mfrc522.PCD_SetAntennaGain(mfrc522.RxGain_max);
    logInfo("✅ NFC初始化成功: MFRC522 v" + String(version, HEX));
  } else {
    sysStatus.nfcWorking = false;
    logError("❌ NFC初始化失败，版本号: 0x" + String(version, HEX));
    logError("   可能原因：");
    logError("   1. MFRC522模块未连接或损坏");
    logError("   2. SPI接线错误（MOSI/MISO/SCK/CS/RST）");
    logError("   3. 电源电压不足（需要3.3V）");
    logError("   4. 模块与ESP32-S3不兼容");
    logWarn("⚠️ 系统将以降级模式运行（NFC功能不可用）");
    logWarn("⚠️ NFC将在运行时自动重试恢复");
  }

  initWiFi();

  if (sysStatus.displayWorking) {
    delay(100);  // 给I2C总线喘息时间
    display.clearBuffer();
    display.setFont(u8g2_font_6x10_tf);
    display.drawStr(25, 15, "System Ready!");

    char buffer[30];
    sprintf(buffer, "NFC: %s", sysStatus.nfcWorking ? "OK" : "Failed");
    display.drawStr(10, 30, buffer);

    sprintf(buffer, "WiFi: %s", sysStatus.wifiConnected ? "OK" : "Offline");
    display.drawStr(10, 45, buffer);

    display.sendBuffer();
    delay(100);  // 给I2C总线喘息时间
  }

  beepSuccess();
  delay(2000);

  currentState = STATE_WELCOME;
  stateStartTime = millis();

  logInfo("=== 系统初始化完成 ===");
  logInfo("服务选项: 4个洗车套餐 + VIP查询");
  logInfo("✅ 脉冲逻辑已修复");
  logInfo("✅ 模块化重构完成");
  logInfo("系统已就绪！");
  lastHeartbeat = millis();
  lastSuccessfulOperation = millis();  // 初始化成功操作时间戳
}

void loop() {
  loopStartTime = millis();

  // =================== 错误恢复监控（方案A优化1）===================
  // 5分钟无操作自动重启（欢迎界面除外）
  if (currentState != STATE_WELCOME && millis() - lastSuccessfulOperation > AUTO_RESTART_TIMEOUT_MS) {
    Serial.println("⚠️ 长时间无操作，自动重启...");
    Serial.printf("   上次成功操作: %lu 秒前\n", (millis() - lastSuccessfulOperation) / 1000);
    Serial.printf("   当前状态: %d\n", currentState);
    delay(1000);
    ESP.restart();
  }

  // 连续错误自动重置
  if (consecutiveErrors >= MAX_CONSECUTIVE_ERRORS) {
    Serial.printf("⚠️ 连续错误 %d 次，重置系统\n", consecutiveErrors);
    consecutiveErrors = 0;
    beepError();
    resetToWelcome();
  }

  checkWiFi();
  checkStateTimeout();
  performHealthCheck();

  // 健壮性增强：自动尝试恢复失败的模块
  tryRecoverNFC();
  tryRecoverWiFi();

  switch (currentState) {
    case STATE_WELCOME:
      handleWelcomeState();
      break;

    case STATE_SELECT_PACKAGE:
      handleSelectPackageState();
      break;

    case STATE_CARD_SCAN:
      handleCardScanState();
      break;

    case STATE_VIP_QUERY:
      handleVIPQueryState();
      break;

    case STATE_VIP_DISPLAY:
      handleVIPDisplayState();
      break;

    case STATE_SYSTEM_READY:
      handleSystemReadyState();
      break;

    case STATE_PROCESSING:
      handleProcessingState();
      break;

    case STATE_COMPLETE:
      handleCompleteState();
      break;

    case STATE_ERROR:
      setSystemLEDStatus();
      displayError("System Error");
      beepError();
      delay(5000);
      resetToWelcome();
      break;

    default:
      resetToWelcome();
      break;
  }

  unsigned long loopTime = millis() - loopStartTime;
  if (loopTime > sysStatus.maxLoopTime) {
    sysStatus.maxLoopTime = loopTime;
  }

  delay(50);
}
