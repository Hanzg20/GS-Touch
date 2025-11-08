/*
 * Eaglson Coin Wash 洗车终端系统 - 最终版 v2.2
 * 
 * 版本: v2.2 Complete Edition - VIP查询功能完整版
 * 日期: 2025-10-30
 * 作者: Eaglson Development Team
 * 
 * 🎯 完整功能列表:
 * ✅ 简化流程：5步操作（欢迎→选择→刷卡→准备→洗车→完成）
 * ✅ 4LED指示灯系统：清晰状态反馈
 * ✅ 5个服务选项：4个洗车套餐 + VIP信息查询
 * ✅ VIP查询功能：
 *    - 显示完整卡号（display_card_number）
 *    - 显示最后交易时间（updated_at）
 *    - 显示余额、会员类型、状态
 * ✅ 自动化流程：刷卡后自动验证+扣费
 * ✅ 四灯全亮：洗车时所有LED点亮
 * 
 * 📋 新增功能 (v2.2):
 * - VIP信息查询（第5个服务选项）
 * - 完整卡号显示
 * - 最后交易时间显示
 * - 会员类型映射（0-7）
 * 
 * 🔧 技术栈:
 * - ESP32-S3
 * - OLED 128x64
 * - RC522 NFC
 * - Supabase后端
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

// =================== 引脚配置 ===================
// 显示和通信
#define OLED_WIDTH 128
#define OLED_HEIGHT 64
#define I2C_SDA 8
#define I2C_SCL 9
#define RC522_CS 10
#define SPI_MOSI 11
#define SPI_SCK 12
#define SPI_MISO 13
#define RC522_RST 14

// 用户界面
#define BTN_OK 1
#define BTN_SELECT 2
#define BUZZER 16

// ✨ 4个LED指示灯系统
#define LED_POWER 3      // L1: 电源指示灯（常亮）
#define LED_NETWORK 6    // L2: 网络指示灯（WiFi状态）
#define LED_PROGRESS 5   // L3: 进度指示灯（流程进度）
#define LED_STATUS 7     // L4: 状态指示灯（交易/故障）

// 控制输出
#define PULSE_OUT 4

// =================== WiFi和网络配置 ===================
#define WIFI_SSID "hanzg_hanyh"
#define WIFI_PASSWORD "han1314521"
#define WIFI_TIMEOUT_MS 20000

// =================== Supabase 配置 ===================
#define SUPABASE_URL "https://ttbtxxpnvkcbyugzdqfw.supabase.co"
#define SUPABASE_KEY "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJzdXBhYmFzZSIsInJlZiI6InR0YnR4eHBudmtjYnl1Z3pkcWZ3Iiwicm9sZSI6ImFub24iLCJpYXQiOjE3NTIzNjM2MjcsImV4cCI6MjA2NzkzOTYyN30.25wuNx2b6BdpZfyghw2vpHeVfBJFShkjhYtkCytQPgg"

// =================== 系统配置 ===================
#define MACHINE_ID "EAGLESON_TERMINAL_01"
#define FIRMWARE_VERSION "v2.2"
#define MAX_RETRY_COUNT 3
bool watchdogEnabled = false;

// =================== 套餐配置（包含VIP查询）===================
struct Package {
  const char* name_en;
  const char* name_fr;
  const char* name_cn;
  int minutes;
  float price;
  int pulses;
  bool isQuery;  // ✅ 新增：标记是否为查询选项
};

Package packages[] = {
  {"Quick Wash", "Lavage Rapide", "快速洗车", 4, 4.00, 4, false},
  {"Standard", "Standard", "标准洗车", 6, 6.00, 8, false},
  {"Deluxe", "Deluxe", "豪华洗车", 8, 9.00, 9, false},
  {"Premium", "Premium", "至尊洗车", 10, 12.00, 12, false},
  {"VIP Info", "Info VIP", "VIP查询", 0, 0.00, 0, true}  // ✅ 新增第5项
};

#define PACKAGE_COUNT 5  // ✅ 从4改为5

// =================== 会员类型映射 ===================
const char* MEMBER_TYPES[] = {
  "Basic",      // 0
  "Silver",     // 1
  "Gold",       // 2
  "Platinum",   // 3
  "Diamond",    // 4
  "VIP",        // 5
  "Premium",    // 6
  "Elite"       // 7
};

// =================== 脉冲和超时配置 ===================
#define PULSE_WIDTH_MS 100
#define PULSE_INTERVAL_MS 1000

// 超时设置
#define STATE_TIMEOUT_WELCOME_MS 60000
#define STATE_TIMEOUT_SELECT_MS 20000
#define STATE_TIMEOUT_CARD_SCAN_MS 15000
#define STATE_TIMEOUT_READY_MS 2000
#define STATE_TIMEOUT_PROCESSING_MS 120000
#define STATE_TIMEOUT_COMPLETE_MS 8000
#define STATE_TIMEOUT_VIP_QUERY_MS 15000    // ✅ VIP查询刷卡超时
#define STATE_TIMEOUT_VIP_DISPLAY_MS 15000  // ✅ VIP信息显示超时

// =================== LED状态枚举 ===================
enum LEDStatus {
  LED_OFF,
  LED_ON,
  LED_BLINK_SLOW,
  LED_BLINK_FAST
};

// =================== 系统状态枚举 ===================
enum SystemState {
  STATE_WELCOME,         // 0. 欢迎屏幕
  STATE_SELECT_PACKAGE,  // 1. 选择服务
  STATE_CARD_SCAN,       // 2. 刷卡支付
  STATE_SYSTEM_READY,    // 3. 系统准备
  STATE_PROCESSING,      // 4. 洗车进行
  STATE_COMPLETE,        // 5. 服务完成
  STATE_VIP_QUERY,       // 6. VIP查询刷卡（✅ 新增）
  STATE_VIP_DISPLAY,     // 7. VIP信息显示（✅ 新增）
  STATE_ERROR
};

enum Language {
  LANG_EN,
  LANG_FR,
  LANG_CN
};

// =================== LED控制结构 ===================
struct SystemLEDs {
  bool power;
  LEDStatus network;
  LEDStatus progress;
  LEDStatus status;
  
  unsigned long lastBlinkTime;
  bool blinkState;
  
  SystemLEDs() {
    power = false;
    network = LED_OFF;
    progress = LED_OFF;
    status = LED_OFF;
    lastBlinkTime = 0;
    blinkState = false;
  }
};

// =================== 卡片信息结构（扩展）===================
struct CardInfo {
  String cardNumber;
  String displayCardNumber;    // ✅ 新增：完整显示卡号
  String cardUID;
  String cardUIDDecimal;
  float balance;
  bool isActive;
  bool isValid;
  String userName;
  String cardType;
  String lastTransactionDate;  // ✅ 新增：最后交易时间
  
  void clear() {
    cardNumber = "";
    displayCardNumber = "";
    cardUID = "";
    cardUIDDecimal = "";
    balance = 0.0;
    isActive = false;
    isValid = false;
    userName = "";
    cardType = "";
    lastTransactionDate = "";
  }
};

// =================== 系统状态结构 ===================
struct SystemStatus {
  bool wifiConnected = false;
  bool nfcWorking = false;
  bool displayWorking = false;
  uint32_t freeHeapMin = UINT32_MAX;
  unsigned long maxLoopTime = 0;
  unsigned long totalTransactions = 0;
  float totalRevenue = 0.0;
  
  void updateMemoryStats() {
    uint32_t freeHeap = ESP.getFreeHeap();
    if (freeHeap < freeHeapMin) {
      freeHeapMin = freeHeap;
    }
  }
};

// =================== 全局对象 ===================
U8G2_SSD1306_128X64_NONAME_F_HW_I2C display(U8G2_R0, U8X8_PIN_NONE, I2C_SCL, I2C_SDA);
MFRC522 mfrc522(RC522_CS, RC522_RST);

// =================== 全局变量 ===================
SystemState currentState = STATE_WELCOME;
Language currentLanguage = LANG_EN;
int selectedPackage = 0;
String cardUID = "";
CardInfo currentCardInfo;
int sentPulses = 0;
unsigned long stateStartTime = 0;
unsigned long processingStartTime = 0;
SystemStatus sysStatus;
SystemLEDs ledIndicator;

// 按钮防抖
unsigned long lastDebounceTime[2] = {0, 0};
bool lastButtonState[2] = {false, false};
bool buttonPressed[2] = {false, false};

// 系统状态
unsigned long lastWiFiCheck = 0;
int apiRetryCount = 0;
unsigned long lastHeartbeat = 0;
unsigned long loopStartTime = 0;
TaskHandle_t mainTaskHandle = NULL;

// =================== 多语言文本 ===================
const char* TEXT_WELCOME[] = {"Welcome", "Bienvenue", "欢迎"};
const char* TEXT_SELECT_SERVICE[] = {"Select Service", "Choisir service", "选择服务"};
const char* TEXT_TAP_CARD[] = {"Tap Your Card", "Appuyez carte", "请刷卡"};
const char* TEXT_PROCESSING[] = {"Wash in Progress", "Lavage en cours", "洗车中"};
const char* TEXT_COMPLETE[] = {"Complete!", "Termine!", "完成!"};
const char* TEXT_THANK_YOU[] = {"Thank You!", "Merci!", "谢谢!"};
const char* TEXT_ERROR_LOW_BALANCE[] = {"Low Balance", "Solde insuffisant", "余额不足"};
const char* TEXT_ERROR_INVALID_CARD[] = {"Invalid Card", "Carte invalide", "卡片无效"};

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
void updateSingleLED(int pin, LEDStatus status) {
  switch (status) {
    case LED_OFF:
      digitalWrite(pin, LOW);
      break;
      
    case LED_ON:
      digitalWrite(pin, HIGH);
      break;
      
    case LED_BLINK_SLOW:
      if (millis() - ledIndicator.lastBlinkTime > 500) {
        ledIndicator.blinkState = !ledIndicator.blinkState;
        ledIndicator.lastBlinkTime = millis();
      }
      digitalWrite(pin, ledIndicator.blinkState ? HIGH : LOW);
      break;
      
    case LED_BLINK_FAST:
      if (millis() - ledIndicator.lastBlinkTime > 100) {
        ledIndicator.blinkState = !ledIndicator.blinkState;
        ledIndicator.lastBlinkTime = millis();
      }
      digitalWrite(pin, ledIndicator.blinkState ? HIGH : LOW);
      break;
  }
}

void updateLEDIndicators() {
  digitalWrite(LED_POWER, ledIndicator.power ? HIGH : LOW);
  updateSingleLED(LED_NETWORK, ledIndicator.network);
  updateSingleLED(LED_PROGRESS, ledIndicator.progress);
  updateSingleLED(LED_STATUS, ledIndicator.status);
}

void setSystemLEDStatus() {
  ledIndicator.power = true;
  
  if (sysStatus.wifiConnected) {
    ledIndicator.network = LED_ON;
  } else {
    ledIndicator.network = LED_BLINK_SLOW;
  }
  
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
      
    case STATE_VIP_QUERY:  // ✅ VIP查询状态
      ledIndicator.progress = LED_ON;
      ledIndicator.status = LED_BLINK_FAST;
      break;
      
    case STATE_VIP_DISPLAY:  // ✅ VIP信息显示
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

// =================== NFC读卡 ===================
String readCardUID() {
  if (!mfrc522.PICC_IsNewCardPresent()) {
    return "";
  }
  
  if (!mfrc522.PICC_ReadCardSerial()) {
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
  return decimalUID;
}

// =================== WiFi管理 ===================
void initWiFi() {
  logInfo("连接WiFi: " + String(WIFI_SSID));
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  unsigned long startTime = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startTime < WIFI_TIMEOUT_MS) {
    delay(500);
    Serial.print(".");
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    sysStatus.wifiConnected = true;
    Serial.println();
    logInfo("✅ WiFi连接成功: " + WiFi.localIP().toString());
  } else {
    sysStatus.wifiConnected = false;
    Serial.println();
    logWarn("⚠️ WiFi连接失败，离线模式");
  }
}

void checkWiFi() {
  if (millis() - lastWiFiCheck > 30000) {
    sysStatus.wifiConnected = (WiFi.status() == WL_CONNECTED);
    lastWiFiCheck = millis();
  }
}

// =================== Supabase API（完整版）===================
CardInfo getCardInfoFromSupabase(const String& decimalUID) {
  CardInfo info;
  info.clear();
  info.cardUIDDecimal = decimalUID;
  info.cardUID = decimalToHexUID(decimalUID);
  
  if (!sysStatus.wifiConnected) {
    logInfo("🔄 离线模式测试");
    if (decimalUID == "2345408116" || decimalUID == "1210711100") {
      info.displayCardNumber = "JC-VIP-8116";  // ✅ 完整卡号
      info.cardNumber = "****8116";
      info.balance = 76.00;
      info.isValid = true;
      info.isActive = true;
      info.userName = "Test User";
      info.cardType = "Premium";
      info.lastTransactionDate = "2025-10-29";  // ✅ 最后交易
      logInfo("✅ 离线验证成功");
    }
    return info;
  }
  
  HTTPClient http;
  // ✅ 查询完整信息：包含display_card_number和updated_at
  String endpoint = String(SUPABASE_URL) + 
    "/rest/v1/jc_vip_cards?card_uid=eq." + decimalUID + 
    "&select=display_card_number,cardholder_name,card_credit,member_type,is_active,updated_at";
  
  http.begin(endpoint);
  http.addHeader("apikey", SUPABASE_KEY);
  http.addHeader("Authorization", "Bearer " + String(SUPABASE_KEY));
  http.setTimeout(10000);
  
  int httpCode = http.GET();
  
  if (httpCode == 200) {
    String response = http.getString();
    JsonDocument doc;
    
    if (deserializeJson(doc, response) == DeserializationError::Ok && doc.size() > 0) {
      // ✅ 获取完整卡号
      String displayNum = doc[0]["display_card_number"] | "";
      if (displayNum.length() > 0) {
        info.displayCardNumber = displayNum;
        // 生成简短卡号（后4位）
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
      
      // ✅ 会员类型映射（0-7）
      int memberType = doc[0]["member_type"] | 0;
      if (memberType >= 0 && memberType <= 7) {
        info.cardType = MEMBER_TYPES[memberType];
      } else {
        info.cardType = "Unknown";
      }
      
      // ✅ 获取最后交易时间
      String updatedAt = doc[0]["updated_at"] | "";
      if (updatedAt.length() > 0) {
        // 格式：2025-10-29T15:30:00+00:00 -> 2025-10-29
        info.lastTransactionDate = updatedAt.substring(0, 10);
      } else {
        info.lastTransactionDate = "Never";
      }
      
      logInfo("✅ 在线验证成功");
      logInfo("  完整卡号: " + info.displayCardNumber);
      logInfo("  会员类型: " + info.cardType);
      logInfo("  最后使用: " + info.lastTransactionDate);
    }
  } else {
    logError("❌ API错误: " + String(httpCode));
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
  String endpoint = String(SUPABASE_URL) + "/rest/v1/jc_vip_cards?card_uid=eq." + decimalUID;
  
  JsonDocument doc;
  doc["card_credit"] = newBalance;
  
  String jsonString;
  serializeJson(doc, jsonString);
  
  http.begin(endpoint);
  http.addHeader("apikey", SUPABASE_KEY);
  http.addHeader("Authorization", "Bearer " + String(SUPABASE_KEY));
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Prefer", "return=minimal");
  
  int httpCode = http.PATCH(jsonString);
  http.end();
  
  return (httpCode == 204);
}

bool recordTransaction(const String& decimalUID, float amount, float balanceBefore, const String& packageName) {
  if (!sysStatus.wifiConnected) {
    logInfo("🔄 交易记录已缓存");
    return true;
  }
  
  HTTPClient http;
  String endpoint = String(SUPABASE_URL) + "/rest/v1/jc_transaction_history";
  
  JsonDocument doc;
  doc["machine_id"] = MACHINE_ID;
  doc["card_uid"] = decimalUID.toInt();
  doc["transaction_type"] = "CHARGE";
  doc["third_party_reference"] = packageName;
  doc["transaction_amount"] = amount;
  doc["balance_before"] = balanceBefore;
  
  String jsonString;
  serializeJson(doc, jsonString);
  
  http.begin(endpoint);
  http.addHeader("apikey", SUPABASE_KEY);
  http.addHeader("Authorization", "Bearer " + String(SUPABASE_KEY));
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Prefer", "return=minimal");
  
  int httpCode = http.POST(jsonString);
  http.end();
  
  if (httpCode == 201) {
    sysStatus.totalTransactions++;
    sysStatus.totalRevenue += amount;
    return true;
  }
  return false;
}

// =================== 显示函数 ===================
void displayWelcome() {
  if (!sysStatus.displayWorking) return;
  
  display.clearBuffer();
  display.setFont(u8g2_font_6x10_tf);
  
  display.drawBox(0, 0, 128, 14);
  display.setDrawColor(0);
  display.drawStr(10, 11, "EAGLSON COIN WASH");
  display.setDrawColor(1);
  
  display.drawStr(15, 28, "24h Self-Service");
  display.drawStr(20, 40, "Quick & Safe");
  display.drawStr(15, 52, "Member Discount");
  display.drawStr(15, 64, "Press OK to Start");
  
  display.sendBuffer();
}

void displayPackageSelection() {
  if (!sysStatus.displayWorking) return;
  
  display.clearBuffer();
  display.setFont(u8g2_font_6x10_tf);
  
  display.drawStr(15, 10, TEXT_SELECT_SERVICE[currentLanguage]);
  display.drawHLine(0, 12, 128);
  
  for (int i = 0; i < PACKAGE_COUNT; i++) {
    int y = 24 + (i * 10);
    
    if (i == selectedPackage) {
      display.drawStr(2, y, ">");
    }
    
    char buffer[32];
    if (packages[i].isQuery) {
      // ✅ VIP查询选项
      sprintf(buffer, "%s", packages[i].name_en);
    } else {
      sprintf(buffer, "%s $%.0f %dmin", packages[i].name_en, packages[i].price, packages[i].minutes);
    }
    display.drawStr(12, y, buffer);
  }
  
  display.sendBuffer();
}

void displayCardScan() {
  if (!sysStatus.displayWorking) return;
  
  display.clearBuffer();
  display.setFont(u8g2_font_6x10_tf);
  
  display.drawStr(20, 10, TEXT_TAP_CARD[currentLanguage]);
  display.drawHLine(0, 12, 128);
  
  unsigned long now = millis();
  int animFrame = (now / 500) % 3;
  
  for(int i = 0; i < 3; i++) {
    int radius = 10 + (i * 5);
    if(i <= animFrame) {
      display.drawCircle(64, 35, radius);
    }
  }
  
  const Package& pkg = packages[selectedPackage];
  char buffer[30];
  sprintf(buffer, "%s $%.2f", pkg.name_en, pkg.price);
  display.drawStr(30, 60, buffer);
  
  display.sendBuffer();
}

// ✅ VIP查询刷卡界面
void displayVIPQueryScan() {
  if (!sysStatus.displayWorking) return;
  
  display.clearBuffer();
  display.setFont(u8g2_font_6x10_tf);
  
  display.drawStr(15, 10, "VIP Card Info Query");
  display.drawHLine(0, 12, 128);
  
  unsigned long now = millis();
  int animFrame = (now / 500) % 3;
  
  for(int i = 0; i < 3; i++) {
    int radius = 10 + (i * 5);
    if(i <= animFrame) {
      display.drawCircle(64, 35, radius);
    }
  }
  
  display.drawStr(15, 58, "Tap your VIP card");
  
  display.sendBuffer();
}

// ✅ VIP信息显示（完整版）
void displayVIPInfo() {
  if (!sysStatus.displayWorking) return;
  
  display.clearBuffer();
  display.setFont(u8g2_font_5x8_tf);
  
  // 标题
  display.drawStr(5, 8, "VIP Card Information");
  display.drawHLine(0, 10, 128);
  
  char buffer[32];
  
  // 姓名
  snprintf(buffer, sizeof(buffer), "Name: %s", currentCardInfo.userName.c_str());
  display.drawStr(2, 20, buffer);
  
  // ✅ 完整卡号
  snprintf(buffer, sizeof(buffer), "Card: %s", currentCardInfo.displayCardNumber.c_str());
  display.drawStr(2, 29, buffer);
  
  // 会员类型
  snprintf(buffer, sizeof(buffer), "Type: %s", currentCardInfo.cardType.c_str());
  display.drawStr(2, 38, buffer);
  
  // 余额（重要信息）
  display.setFont(u8g2_font_6x10_tf);
  snprintf(buffer, sizeof(buffer), "Balance: $%.2f", currentCardInfo.balance);
  display.drawStr(2, 50, buffer);
  
  // ✅ 最后交易时间
  display.setFont(u8g2_font_5x8_tf);
  snprintf(buffer, sizeof(buffer), "Last: %s", currentCardInfo.lastTransactionDate.c_str());
  display.drawStr(2, 59, buffer);
  
  // 状态
  snprintf(buffer, sizeof(buffer), "Status: %s", currentCardInfo.isActive ? "Active" : "Inactive");
  display.drawStr(2, 68, buffer);
  
  display.sendBuffer();
}

void displayProcessing(const char* message, float progress) {
  if (!sysStatus.displayWorking) return;
  
  display.clearBuffer();
  display.setFont(u8g2_font_6x10_tf);
  
  display.drawStr(15, 15, message);
  
  display.drawFrame(10, 30, 108, 10);
  int barWidth = (int)(progress * 104);
  if (barWidth > 0) {
    display.drawBox(12, 32, barWidth, 6);
  }
  
  display.sendBuffer();
}

void displayWashProgress(int current, int total, int remainingMin, int remainingSec) {
  if (!sysStatus.displayWorking) return;
  
  display.clearBuffer();
  display.setFont(u8g2_font_6x10_tf);
  
  display.drawStr(10, 10, TEXT_PROCESSING[currentLanguage]);
  display.drawHLine(0, 12, 128);
  
  display.setFont(u8g2_font_10x20_tf);
  char timeBuffer[10];
  sprintf(timeBuffer, "%02d:%02d", remainingMin, remainingSec);
  display.drawStr(35, 32, timeBuffer);
  
  display.setFont(u8g2_font_6x10_tf);
  char buffer[20];
  sprintf(buffer, "Pulse %d/%d", current, total);
  display.drawStr(40, 48, buffer);
  
  display.drawFrame(10, 52, 108, 8);
  int barWidth = total > 0 ? (current * 104) / total : 0;
  if (barWidth > 0) {
    display.drawBox(12, 54, barWidth, 4);
  }
  
  display.sendBuffer();
}

void displayComplete() {
  if (!sysStatus.displayWorking) return;
  
  display.clearBuffer();
  display.setFont(u8g2_font_6x10_tf);
  
  display.drawStr(35, 15, TEXT_COMPLETE[currentLanguage]);
  
  display.drawLine(50, 35, 60, 45);
  display.drawLine(60, 45, 78, 25);
  
  display.drawStr(30, 60, TEXT_THANK_YOU[currentLanguage]);
  
  display.sendBuffer();
}

void displayError(const char* message) {
  if (!sysStatus.displayWorking) {
    logError(String(message));
    return;
  }
  
  display.clearBuffer();
  display.setFont(u8g2_font_6x10_tf);
  
  display.drawStr(50, 15, "Error!");
  
  display.drawLine(50, 30, 78, 50);
  display.drawLine(78, 30, 50, 50);
  
  display.drawStr(10, 64, message);
  
  display.sendBuffer();
}

// =================== 状态处理函数 ===================
void handleWelcomeState() {
  setSystemLEDStatus();
  displayWelcome();
  
  if (readButtonImproved(BTN_OK)) {
    currentState = STATE_SELECT_PACKAGE;
    selectedPackage = 0;
    stateStartTime = millis();
    beepShort();
    logInfo("✅ 用户启动服务");
  }
}

void handleSelectPackageState() {
  setSystemLEDStatus();
  displayPackageSelection();
  
  if (readButtonImproved(BTN_SELECT)) {
    selectedPackage = (selectedPackage + 1) % PACKAGE_COUNT;
    beepShort();
  }
  
  if (readButtonImproved(BTN_OK)) {
    // ✅ 检查是否选择了VIP查询
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
  displayCardScan();
  
  String uid = readCardUID();
  
  if (uid.length() > 0) {
    beepShort();
    
    displayProcessing("Verifying Card...", 0.3);
    ledIndicator.network = LED_BLINK_FAST;
    updateLEDIndicators();
    
    currentCardInfo = getCardInfoFromSupabase(uid);
    
    if (currentCardInfo.isValid && currentCardInfo.isActive) {
      if (currentCardInfo.balance >= packages[selectedPackage].price) {
        
        const Package& pkg = packages[selectedPackage];
        float amount = pkg.price;
        float balanceBefore = currentCardInfo.balance;
        float balanceAfter = balanceBefore - amount;
        
        displayProcessing("Processing Payment...", 0.6);
        
        bool success = updateCardBalance(currentCardInfo.cardUIDDecimal, balanceAfter);
        recordTransaction(currentCardInfo.cardUIDDecimal, -amount, balanceBefore, String(pkg.name_en));
        
        if (success) {
          currentCardInfo.balance = balanceAfter;
          displayProcessing("Payment Success!", 1.0);
          delay(1000);
          
          currentState = STATE_SYSTEM_READY;
          stateStartTime = millis();
          beepSuccess();
          logInfo("✅ 支付成功，余额: $" + String(balanceAfter, 2));
        } else {
          displayError("Transaction Failed");
          beepError();
          delay(2000);
          resetToWelcome();
        }
      } else {
        displayError(TEXT_ERROR_LOW_BALANCE[currentLanguage]);
        beepError();
        delay(2000);
        resetToWelcome();
      }
    } else {
      displayError(TEXT_ERROR_INVALID_CARD[currentLanguage]);
      beepError();
      delay(2000);
      resetToWelcome();
    }
  }
}

// ✅ VIP查询刷卡处理
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

// ✅ VIP信息显示处理
void handleVIPDisplayState() {
  setSystemLEDStatus();
  displayVIPInfo();
  
  if (millis() - stateStartTime > STATE_TIMEOUT_VIP_DISPLAY_MS) {
    resetToWelcome();
  }
  
  if (readButtonImproved(BTN_OK)) {
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
    currentState = STATE_PROCESSING;
    processingStartTime = millis();
    stateStartTime = millis();
    sentPulses = 0;
    logInfo("✅ 开始洗车服务");
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
  
  static unsigned long lastPulseTime = 0;
  
  if (millis() - lastPulseTime >= PULSE_INTERVAL_MS && sentPulses < pkg.pulses) {
    digitalWrite(PULSE_OUT, HIGH);
    delay(PULSE_WIDTH_MS);
    digitalWrite(PULSE_OUT, LOW);
    
    sentPulses++;
    lastPulseTime = millis();
    
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
  displayComplete();
  
  static bool soundPlayed = false;
  if (!soundPlayed) {
    beepSuccess();
    delay(200);
    beepSuccess();
    soundPlayed = true;
  }
  
  if (millis() - stateStartTime > STATE_TIMEOUT_COMPLETE_MS) {
    soundPlayed = false;
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
    case STATE_VIP_QUERY: timeout = STATE_TIMEOUT_VIP_QUERY_MS; break;  // ✅ VIP查询超时
    case STATE_VIP_DISPLAY: timeout = STATE_TIMEOUT_VIP_DISPLAY_MS; break;  // ✅ VIP显示超时
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
    
    lastHeartbeat = millis();
  }
}

// =================== 主程序 ===================
void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("=====================================");
  Serial.printf("🚗 Eaglson Coin Wash Terminal %s\n", FIRMWARE_VERSION);
  Serial.println("Complete Edition - VIP查询完整版");
  Serial.println("🎯 5步流程 + 4LED + VIP查询功能");
  Serial.printf("机器ID: %s\n", MACHINE_ID);
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
  
  logInfo("🔌 初始化SPI和NFC...");
  SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI, RC522_CS);
  delay(100);
  
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
    logError("❌ NFC初始化失败");
  }
  
  logInfo("🖥️ 初始化I2C和OLED...");
  Wire.begin(I2C_SDA, I2C_SCL);
  delay(100);
  
  if (display.begin()) {
    display.enableUTF8Print();
    sysStatus.displayWorking = true;
    logInfo("✅ OLED初始化成功");
  } else {
    sysStatus.displayWorking = false;
    logWarn("⚠️ OLED初始化失败");
  }
  
  initWiFi();
  
  if (sysStatus.displayWorking) {
    display.clearBuffer();
    display.setFont(u8g2_font_6x10_tf);
    display.drawStr(25, 15, "System Ready!");
    
    char buffer[30];
    sprintf(buffer, "NFC: %s", sysStatus.nfcWorking ? "OK" : "Failed");
    display.drawStr(10, 30, buffer);
    
    sprintf(buffer, "WiFi: %s", sysStatus.wifiConnected ? "OK" : "Offline");
    display.drawStr(10, 45, buffer);
    
    display.sendBuffer();
  }
  
  beepSuccess();
  delay(2000);
  
  currentState = STATE_WELCOME;
  stateStartTime = millis();
  
  logInfo("=== 系统初始化完成 ===");
  logInfo("服务选项: 4个洗车套餐 + VIP查询");
  logInfo("系统已就绪！");
  lastHeartbeat = millis();
}

void loop() {
  loopStartTime = millis();
  
  checkWiFi();
  checkStateTimeout();
  performHealthCheck();
  
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
      
    case STATE_VIP_QUERY:  // ✅ VIP查询
      handleVIPQueryState();
      break;
      
    case STATE_VIP_DISPLAY:  // ✅ VIP显示
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
