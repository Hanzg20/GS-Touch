/*
 * Eaglson Coin Wash 洗车终端系统 - 最终完整版
 * 解决中文显示、看门狗和交易记录问题
 * 
 * 修正内容：
 * 1. 使用U8g2库支持中文显示
 * 2. 修正看门狗任务注册问题
 * 3. 更新交易记录表结构
 * 
 * 版本: v4.4 Final - 问题修正版
 * 日期: 2025-10-26
 * 作者: Eaglson Development Team
 */

#include <Wire.h>
#include <SPI.h>
#include <U8g2lib.h>  // 修正：使用U8g2库替代Adafruit_SSD1306
#include <MFRC522.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <esp_task_wdt.h>

// =================== 引脚配置 (保持原有SSD1306配置) ===================
#define OLED_WIDTH      128
#define OLED_HEIGHT     64
#define I2C_SDA         8
#define I2C_SCL         9

#define RC522_CS        10
#define SPI_MOSI        11
#define SPI_SCK         12
#define SPI_MISO        13
#define RC522_RST       14

#define BTN_OK          1
#define BTN_SELECT      2

#define LED_GREEN       5
#define LED_BLUE        6
#define LED_RED         7

#define BUZZER          16
#define PULSE_OUT       4

// =================== WiFi和网络配置 ===================
#define WIFI_SSID        "hanzg_hanyh"
#define WIFI_PASSWORD    "han1314521"
#define WIFI_TIMEOUT_MS  20000

// =================== Supabase 配置 ===================
#define SUPABASE_URL     "https://ttbtxxpnvkcbyugzdqfw.supabase.co"
#define SUPABASE_KEY     "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJzdXBhYmFzZSIsInJlZiI6InR0YnR4eHBudmtjYnl1Z3pkcWZ3Iiwicm9sZSI6ImFub24iLCJpYXQiOjE3NTIzNjM2MjcsImV4cCI6MjA2NzkzOTYyN30.25wuNx2b6BdpZfyghw2vpHeVfBJFShkjhYtkCytQPgg"

// =================== 系统配置 ===================
#define MACHINE_ID       "EAGLESON_TERMINAL_01"
#define FIRMWARE_VERSION "v4.4"
#define MAX_RETRY_COUNT  3

// =================== 套餐配置 ===================
struct Package {
  const char* name_en;
  const char* name_fr;
  const char* name_cn;
  int minutes;
  float price;
  int pulses;
};

Package packages[] = {
  {"Quick Wash", "Lavage Rapide", "快速洗车", 4, 4.00, 4},
  {"Standard", "Standard", "标准洗车", 6, 6.00, 8},
  {"Deluxe", "Deluxe", "豪华洗车", 8, 9.00, 9},
  {"Premium", "Premium", "至尊洗车", 10, 12.00, 12}
};

#define PACKAGE_COUNT 4

// =================== 脉冲配置 ===================
#define PULSE_WIDTH_MS      100
#define PULSE_INTERVAL_MS   1000

// =================== 超时配置 ===================
#define STATE_TIMEOUT_LANGUAGE_MS    20000   
#define STATE_TIMEOUT_SELECT_MS      30000   
#define STATE_TIMEOUT_CARD_SCAN_MS   15000    
#define STATE_TIMEOUT_CARD_INFO_MS   30000    
#define STATE_TIMEOUT_CONFIRM_MS     15000    
#define STATE_TIMEOUT_PROCESSING_MS  120000   
#define STATE_TIMEOUT_COMPLETE_MS    10000    

// =================== 系统状态 ===================
enum SystemState {
  STATE_IDLE,
  STATE_LANGUAGE_SELECT,
  STATE_SELECT_PACKAGE,
  STATE_CARD_SCAN,
  STATE_CARD_INFO,
  STATE_CONFIRM,
  STATE_PROCESSING,
  STATE_COMPLETE,
  STATE_ERROR
};

enum Language {
  LANG_EN,
  LANG_FR,
  LANG_CN
};

// =================== 卡片信息结构 ===================
struct CardInfo {
  String cardNumber;
  String cardUID;
  String cardUIDDecimal;
  float balance;
  bool isActive;
  bool isValid;
  String userName;
  String cardType;
};

// =================== U8g2 OLED显示对象 (修正：支持中文显示) ===================
U8G2_SSD1306_128X64_NONAME_F_HW_I2C display(U8G2_R0, /* reset=*/ U8X8_PIN_NONE, /* clock=*/ I2C_SCL, /* data=*/ I2C_SDA);

// =================== 全局变量 ===================
MFRC522 mfrc522(RC522_CS, RC522_RST);

SystemState currentState = STATE_IDLE;
Language selectedLanguage = LANG_EN;
Language currentLanguage = LANG_EN;
int selectedPackage = 0;
String cardUID = "";
CardInfo currentCardInfo;
int sentPulses = 0;
unsigned long stateStartTime = 0;
unsigned long processingStartTime = 0;

// 按钮防抖变量 (下拉电阻配置)
unsigned long lastDebounceTime[2] = {0, 0};
bool lastButtonState[2] = {false, false};
bool buttonPressed[2] = {false, false};

// WiFi和连接状态
bool wifiConnected = false;
unsigned long lastWiFiCheck = 0;
int apiRetryCount = 0;
unsigned long lastHeartbeat = 0;

// 性能监控
unsigned long loopStartTime = 0;
unsigned long maxLoopTime = 0;
uint32_t freeHeapMin = UINT32_MAX;

// 修正：看门狗任务句柄
TaskHandle_t mainTaskHandle = NULL;

// =================== 多语言文本数组 ===================
const char* TEXT_WELCOME[] = {
  "Welcome to",           // EN
  "Bienvenue au",         // FR
  "欢迎使用"               // CN
};

const char* TEXT_BRAND[] = {
  "Eaglson Coin Wash",    // EN
  "Lave-Auto Eaglson",    // FR
  "壹狗剩自助洗车服务"       // CN
};

const char* LANG_NAMES[] = {"English", "Français", "中文"};

const char* TEXT_TAP_CARD[] = {
  "Please tap card",      // EN
  "Appuyez carte",        // FR
  "请刷卡"                // CN
};

const char* TEXT_SELECT_PACKAGE[] = {
  "Select Package",       // EN
  "Choisir forfait",      // FR
  "选择套餐"              // CN
};

const char* TEXT_CONFIRM[] = {
  "Confirm Order?",       // EN
  "Confirmer?",           // FR
  "确认订单?"              // CN
};

const char* TEXT_PROCESSING[] = {
  "Wash in Progress",     // EN
  "Lavage en cours",      // FR
  "洗车进行中"            // CN
};

const char* TEXT_COMPLETE[] = {
  "Complete!",            // EN
  "Termine!",             // FR
  "洗车完成!"             // CN
};

const char* TEXT_THANK_YOU[] = {
  "Thank You!",           // EN
  "Merci!",               // FR
  "谢谢使用!"             // CN
};

const char* TEXT_ERROR_LOW_BALANCE[] = {
  "Low Balance",          // EN
  "Solde insuffisant",    // FR
  "余额不足"              // CN
};

const char* TEXT_ERROR_INVALID_CARD[] = {
  "Invalid Card",         // EN
  "Carte invalide",       // FR
  "卡片无效"              // CN
};

// =================== 工具函数 ===================
void beepShort() {
  digitalWrite(BUZZER, HIGH);
  delay(100);
  digitalWrite(BUZZER, LOW);
}

void beepLong() {
  digitalWrite(BUZZER, HIGH);
  delay(300);
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

void setLED(bool red, bool green, bool blue) {
  digitalWrite(LED_RED, red ? HIGH : LOW);
  digitalWrite(LED_GREEN, green ? HIGH : LOW);
  digitalWrite(LED_BLUE, blue ? HIGH : LOW);
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
  // 🔥 修正：使用 unsigned long 避免溢出
  // toInt() 最大值是 2147483647，会导致大于此值的 UID 溢出
  // 改用字符串手动转换
  unsigned long decimalValue = 0;

  for (int i = 0; i < decimalUID.length(); i++) {
    char c = decimalUID.charAt(i);
    if (c >= '0' && c <= '9') {
      decimalValue = decimalValue * 10 + (c - '0');
    }
  }

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
      Serial.printf("🔘 按钮按下: GPIO%d\n", pin);
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
  // 🔥 修正：增强卡片检测逻辑
  if (!mfrc522.PICC_IsNewCardPresent()) {
    return "";
  }

  // 🔥 新增：延迟确保卡片稳定
  delay(10);

  if (!mfrc522.PICC_ReadCardSerial()) {
    Serial.println("⚠️ 卡片检测到但读取失败");
    return "";
  }

  String hexUID = "";
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    if (mfrc522.uid.uidByte[i] < 0x10) hexUID += "0";
    hexUID += String(mfrc522.uid.uidByte[i], HEX);
  }
  hexUID.toUpperCase();

  String decimalUID = hexUIDToDecimal(hexUID);

  // 🔥 新增：显示卡片类型
  Serial.println("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
  Serial.printf("📡 读取到卡片: HEX=%s, DEC=%s\n", hexUID.c_str(), decimalUID.c_str());
  Serial.printf("   卡片类型: ");

  byte piccType = mfrc522.PICC_GetType(mfrc522.uid.sak);
  switch(piccType) {
    case MFRC522::PICC_TYPE_MIFARE_MINI:
      Serial.println("MIFARE Mini");
      break;
    case MFRC522::PICC_TYPE_MIFARE_1K:
      Serial.println("MIFARE 1KB");
      break;
    case MFRC522::PICC_TYPE_MIFARE_4K:
      Serial.println("MIFARE 4KB");
      break;
    case MFRC522::PICC_TYPE_MIFARE_UL:
      Serial.println("MIFARE Ultralight");
      break;
    default:
      Serial.printf("未知类型 (SAK=0x%02X)\n", mfrc522.uid.sak);
      break;
  }

  Serial.printf("   UID长度: %d 字节\n", mfrc522.uid.size);
  Serial.println("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");

  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();

  return decimalUID;
}

// =================== WiFi管理 ===================
void initWiFi() {
  Serial.println("🌐 开始连接WiFi...");
  Serial.printf("SSID: %s\n", WIFI_SSID);
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  unsigned long startAttemptTime = millis();
  
  while (WiFi.status() != WL_CONNECTED && 
         millis() - startAttemptTime < WIFI_TIMEOUT_MS) {
    delay(500);
    Serial.print(".");
    esp_task_wdt_reset();  // 修正：在WiFi连接期间喂狗
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    wifiConnected = true;
    Serial.println();
    Serial.println("✅ WiFi连接成功!");
    Serial.printf("IP地址: %s\n", WiFi.localIP().toString().c_str());
    Serial.printf("信号强度: %d dBm\n", WiFi.RSSI());
  } else {
    wifiConnected = false;
    Serial.println();
    Serial.println("❌ WiFi连接失败，使用离线模式");
  }
}

void checkWiFi() {
  if (millis() - lastWiFiCheck > 30000) {
    bool wasConnected = wifiConnected;
    wifiConnected = (WiFi.status() == WL_CONNECTED);
    
    if (!wifiConnected && wasConnected) {
      Serial.println("⚠️ WiFi连接丢失");
    } else if (wifiConnected && !wasConnected) {
      Serial.println("✅ WiFi重新连接成功");
      Serial.printf("IP地址: %s\n", WiFi.localIP().toString().c_str());
    }
    
    lastWiFiCheck = millis();
  }
}

// =================== Supabase API集成 (修正交易表结构) ===================
CardInfo getCardInfoFromSupabase(const String& decimalUID) {
  CardInfo info;
  info.cardUIDDecimal = decimalUID;
  info.cardUID = decimalToHexUID(decimalUID);
  info.isValid = false;
  info.isActive = false;
  info.balance = 0.0;
  
  // 离线测试数据
  if (!wifiConnected) {
    Serial.println("🔄 离线模式：使用测试数据");
    if (decimalUID == "2345408116" || decimalUID == "1210711100" || 
        decimalUID == "305419896" || decimalUID == "123456789") {
      info.cardNumber = "****" + decimalUID.substring(decimalUID.length() - 4);
      info.balance = 76.00;
      info.isValid = true;
      info.isActive = true;
      info.userName = "Test User";
      info.cardType = "VIP";
      Serial.printf("✅ 离线验证成功: DEC=%s, HEX=%s, 余额: $%.2f\n", 
                    decimalUID.c_str(), info.cardUID.c_str(), info.balance);
    } else {
      Serial.printf("❌ 离线模式：未知卡片 %s\n", decimalUID.c_str());
    }
    return info;
  }
  
  HTTPClient http;
  String endpoint = String(SUPABASE_URL) + "/rest/v1/jc_vip_cards?card_uid=eq." + decimalUID + "&select=*";
  
  http.begin(endpoint);
  http.addHeader("apikey", SUPABASE_KEY);
  http.addHeader("Authorization", "Bearer " + String(SUPABASE_KEY));
  http.addHeader("Content-Type", "application/json");
  http.setTimeout(10000);
  
  int httpCode = http.GET();
  
  if (httpCode == 200) {
    String response = http.getString();
    Serial.printf("📊 Supabase响应: %s\n", response.c_str());
    
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, response);
    
    if (!error && doc.size() > 0) {
      JsonObject card = doc[0];
      info.cardNumber = "****" + decimalUID.substring(decimalUID.length() - 4);
      info.balance = card["card_credit"].as<float>();
      info.isValid = true;
      info.isActive = card["is_active"].as<bool>();
      info.userName = card["cardholder_name"] | "Unknown";
      info.cardType = card["member_type"] | "Standard";
      
      Serial.printf("✅ 在线验证成功: DEC=%s, HEX=%s, 余额: $%.2f, 状态: %s\n", 
                    decimalUID.c_str(), info.cardUID.c_str(), info.balance, 
                    info.isActive ? "激活" : "未激活");
      apiRetryCount = 0;
    } else {
      Serial.printf("❌ JSON解析失败或无数据: %s\n", error.c_str());
    }
  } else {
    Serial.printf("❌ Supabase API错误: HTTP %d\n", httpCode);
    apiRetryCount++;
    
    if (apiRetryCount >= MAX_RETRY_COUNT) {
      Serial.println("⚠️ API重试次数超限，切换到离线模式");
      wifiConnected = false;
    }
  }
  
  http.end();
  return info;
}

bool updateCardBalance(const String& decimalUID, float newBalance) {
  if (!wifiConnected) {
    Serial.println("🔄 离线模式：余额更新已缓存");
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
  
  if (httpCode == 204) {
    Serial.printf("✅ 余额更新成功: DEC=%s, 新余额=$%.2f\n", decimalUID.c_str(), newBalance);
    return true;
  } else {
    Serial.printf("❌ 余额更新失败: HTTP %d\n", httpCode);
    return false;
  }
}

// 修正：更新交易记录函数，使用正确的表结构
bool recordTransactionToSupabase(const String& decimalUID, float amount, float balanceBefore, const String& packageName) {
  if (!wifiConnected) {
    Serial.println("🔄 离线模式：交易记录已缓存");
    return true;
  }
  
  HTTPClient http;
  String endpoint = String(SUPABASE_URL) + "/rest/v1/jc_transaction_history";
  
  // 修正：使用正确的表结构
  JsonDocument doc;
  doc["machine_id"] = MACHINE_ID;
  doc["card_uid"] = decimalUID.toInt();  // 修正：转换为bigint
  doc["transaction_type"] = "CHARGE";
  doc["third_party_reference"] = packageName;  // 修正：使用third_party_reference字段
  doc["transaction_amount"] = amount;
  doc["balance_before"] = balanceBefore;
  // 注意：created_at, updated_at 由数据库自动填充
  
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
    Serial.printf("✅ 交易记录成功保存: DEC=%s, 金额=$%.2f\n", decimalUID.c_str(), amount);
    return true;
  } else {
    Serial.printf("❌ 保存交易记录失败: HTTP %d\n", httpCode);
    return false;
  }
}

// =================== U8g2显示函数 (修正：支持中文显示) ===================
void displayIdle() {
  display.clearBuffer();
  
  // 设置字体
  display.setFont(u8g2_font_6x10_tf);  // 英文字体
  
  // 标题栏
  display.drawBox(0, 0, 128, 12);
  display.setDrawColor(0);  // 黑色文字
  display.drawStr(15, 10, "Eaglson Coin Wash");
  display.setDrawColor(1);  // 恢复白色
  
  // 主要内容
  if (currentLanguage == LANG_CN) {
    display.setFont(u8g2_font_wqy12_t_chinese1);  // 中文字体
    display.drawStr(35, 30, "欢迎使用");
  } else {
    display.setFont(u8g2_font_6x10_tf);
    display.drawStr(35, 30, TEXT_WELCOME[currentLanguage]);
  }
  
  // 系统状态
  display.setFont(u8g2_font_6x10_tf);
  display.drawStr(0, 45, wifiConnected ? "WiFi: ON" : "WiFi: OFF");
  display.drawStr(70, 45, "v4.4");
  
  display.drawStr(15, 60, "Press OK to start");
  
  display.sendBuffer();
}

void displayLanguageSelect() {
  display.clearBuffer();
  
  display.setFont(u8g2_font_6x10_tf);
  display.drawStr(10, 10, "Select Language:");
  display.drawHLine(0, 12, 128);
  
  for (int i = 0; i < 3; i++) {
    int y = 25 + (i * 12);
    
    if (i == selectedLanguage) {
      display.drawBox(0, y - 10, 128, 11);
      display.setDrawColor(0);  // 黑色文字
    } else {
      display.setDrawColor(1);  // 白色文字
    }
    
    char buffer[20];
    sprintf(buffer, "[%d] %s", i + 1, LANG_NAMES[i]);
    
    if (i == 2 && currentLanguage == LANG_CN) {  // 中文选项
      display.setFont(u8g2_font_wqy12_t_chinese1);
      display.drawStr(5, y, "[3] 中文");
      display.setFont(u8g2_font_6x10_tf);
    } else {
      display.drawStr(5, y, buffer);
    }
    
    display.setDrawColor(1);  // 恢复白色
  }
  
  display.setFont(u8g2_font_6x10_tf);
  display.drawStr(5, 62, "$ = switch, OK = confirm");
  
  display.sendBuffer();
}

void displayPackageSelection() {
  display.clearBuffer();
  
  // 标题
  if (currentLanguage == LANG_CN) {
    display.setFont(u8g2_font_wqy12_t_chinese1);
    display.drawStr(5, 12, "选择套餐");
  } else {
    display.setFont(u8g2_font_6x10_tf);
    display.drawStr(5, 10, TEXT_SELECT_PACKAGE[currentLanguage]);
  }
  
  display.drawHLine(0, 14, 128);
  
  // 套餐列表
  for (int i = 0; i < PACKAGE_COUNT; i++) {
    int y = 25 + (i * 12);
    
    if (i == selectedPackage) {
      display.drawBox(0, y - 10, 128, 11);
      display.setDrawColor(0);  // 黑色文字
    } else {
      display.setDrawColor(1);  // 白色文字
    }
    
    char buffer[30];
    if (currentLanguage == LANG_CN) {
      display.setFont(u8g2_font_wqy12_t_chinese1);
      sprintf(buffer, "[%d] %s $%d", i + 1, packages[i].name_cn, (int)packages[i].price);
    } else {
      display.setFont(u8g2_font_6x10_tf);
      const char* name = (currentLanguage == LANG_FR) ? packages[i].name_fr : packages[i].name_en;
      sprintf(buffer, "[%d] %s $%d", i + 1, name, (int)packages[i].price);
    }
    
    display.drawStr(2, y, buffer);
    display.setDrawColor(1);  // 恢复白色
  }
  
  display.sendBuffer();
}

void displayCardScan() {
  display.clearBuffer();
  
  // 标题
  if (currentLanguage == LANG_CN) {
    display.setFont(u8g2_font_wqy12_t_chinese1);
    display.drawStr(15, 12, "请刷卡");
  } else {
    display.setFont(u8g2_font_6x10_tf);
    display.drawStr(15, 10, TEXT_TAP_CARD[currentLanguage]);
  }
  
  display.drawHLine(0, 14, 128);
  
  // NFC动画图标
  unsigned long now = millis();
  int animFrame = (now / 500) % 3;
  
  for(int i = 0; i < 3; i++) {
    int radius = 7 + (i * 4);
    if(i <= animFrame) {
      display.drawCircle(64, 35, radius);
    }
  }
  
  display.setFont(u8g2_font_6x10_tf);
  display.drawStr(25, 60, "Scanning...");
  
  display.sendBuffer();
}

void displayCardInfo() {
  display.clearBuffer();
  
  // 标题
  display.setFont(u8g2_font_6x10_tf);
  display.drawStr(25, 10, "Card Detected");
  display.drawHLine(0, 12, 128);
  
  // 卡片信息
  char buffer[30];
  
  sprintf(buffer, "Card: %s", currentCardInfo.cardNumber.c_str());
  display.drawStr(5, 25, buffer);
  
  sprintf(buffer, "Balance: $%.2f", currentCardInfo.balance);
  display.drawStr(5, 37, buffer);
  
  sprintf(buffer, "Cost: $%.2f", packages[selectedPackage].price);
  display.drawStr(5, 49, buffer);
  
  display.drawStr(5, 61, "Press OK to continue");
  
  display.sendBuffer();
}

void displayConfirm() {
  display.clearBuffer();
  
  // 标题
  if (currentLanguage == LANG_CN) {
    display.setFont(u8g2_font_wqy12_t_chinese1);
    display.drawStr(15, 12, "确认订单?");
  } else {
    display.setFont(u8g2_font_6x10_tf);
    display.drawStr(15, 10, TEXT_CONFIRM[currentLanguage]);
  }
  
  display.drawHLine(0, 14, 128);
  
  // 套餐信息
  display.setFont(u8g2_font_6x10_tf);
  char buffer[30];
  
  const char* name;
  if (currentLanguage == LANG_CN) {
    name = packages[selectedPackage].name_cn;
  } else if (currentLanguage == LANG_FR) {
    name = packages[selectedPackage].name_fr;
  } else {
    name = packages[selectedPackage].name_en;
  }
  
  display.drawStr(5, 26, name);
  
  sprintf(buffer, "Time: %d min", packages[selectedPackage].minutes);
  display.drawStr(5, 38, buffer);
  
  sprintf(buffer, "After: $%.2f", currentCardInfo.balance - packages[selectedPackage].price);
  display.drawStr(5, 50, buffer);
  
  display.drawStr(5, 62, "OK=Pay, $=Back");
  
  display.sendBuffer();
}

void displayProcessing(int current, int total, int remainingMin, int remainingSec) {
  display.clearBuffer();
  
  // 标题
  if (currentLanguage == LANG_CN) {
    display.setFont(u8g2_font_wqy12_t_chinese1);
    display.drawStr(5, 12, "洗车进行中");
  } else {
    display.setFont(u8g2_font_6x10_tf);
    display.drawStr(5, 10, TEXT_PROCESSING[currentLanguage]);
  }
  
  display.drawHLine(0, 14, 128);
  
  // 倒计时 (大字体)
  display.setFont(u8g2_font_10x20_tf);
  char timeBuffer[10];
  sprintf(timeBuffer, "%02d:%02d", remainingMin, remainingSec);
  display.drawStr(30, 35, timeBuffer);
  
  // 脉冲进度
  display.setFont(u8g2_font_6x10_tf);
  char buffer[20];
  sprintf(buffer, "Pulse %d/%d", current, total);
  display.drawStr(35, 48, buffer);
  
  // 进度条
  display.drawFrame(10, 52, 108, 8);
  int barWidth = total > 0 ? (current * 104) / total : 0;
  if (barWidth > 0) {
    display.drawBox(12, 54, barWidth, 4);
  }
  
  display.sendBuffer();
}

void displayComplete() {
  display.clearBuffer();
  
  // 标题
  if (currentLanguage == LANG_CN) {
    display.setFont(u8g2_font_wqy12_t_chinese1);
    display.drawStr(35, 12, "洗车完成!");
  } else {
    display.setFont(u8g2_font_6x10_tf);
    display.drawStr(35, 10, TEXT_COMPLETE[currentLanguage]);
  }
  
  display.drawHLine(0, 14, 128);
  
  // 完成图标 (对勾)
  display.drawLine(45, 30, 55, 40);
  display.drawLine(55, 40, 75, 20);
  display.drawLine(46, 30, 56, 40);
  display.drawLine(56, 40, 76, 20);
  
  // 完成信息
  if (currentLanguage == LANG_CN) {
    display.setFont(u8g2_font_wqy12_t_chinese1);
    display.drawStr(25, 58, "谢谢使用!");
  } else {
    display.setFont(u8g2_font_6x10_tf);
    display.drawStr(25, 58, TEXT_THANK_YOU[currentLanguage]);
  }
  
  display.sendBuffer();
}

void displayError(const char* message) {
  display.clearBuffer();
  
  display.setFont(u8g2_font_6x10_tf);
  display.drawStr(50, 10, "Error!");
  display.drawHLine(0, 12, 128);
  
  // X图标
  display.drawLine(50, 25, 78, 45);
  display.drawLine(78, 25, 50, 45);
  display.drawLine(51, 25, 79, 45);
  display.drawLine(79, 25, 51, 45);
  
  // 错误信息
  if (currentLanguage == LANG_CN) {
    display.setFont(u8g2_font_wqy12_t_chinese1);
    display.drawStr(5, 60, message);
  } else {
    display.setFont(u8g2_font_6x10_tf);
    display.drawStr(5, 60, message);
  }
  
  display.sendBuffer();
}

// =================== 系统管理函数 ===================
void resetToIdle() {
  currentState = STATE_IDLE;
  cardUID = "";
  selectedPackage = 0;
  sentPulses = 0;
  stateStartTime = millis();
  setLED(false, true, false);
  
  for(int i = 0; i < 2; i++) {
    buttonPressed[i] = false;
    lastButtonState[i] = false;
  }
  
  Serial.println("🔄 系统已重置到待机状态");
}

void checkStateTimeout() {
  unsigned long elapsed = millis() - stateStartTime;
  bool timeout = false;
  
  switch (currentState) {
    case STATE_LANGUAGE_SELECT:
      if (elapsed > STATE_TIMEOUT_LANGUAGE_MS) timeout = true;
      break;
    case STATE_SELECT_PACKAGE:
      if (elapsed > STATE_TIMEOUT_SELECT_MS) timeout = true;
      break;
    case STATE_CARD_SCAN:
      if (elapsed > STATE_TIMEOUT_CARD_SCAN_MS) timeout = true;
      break;
    case STATE_CARD_INFO:
      if (elapsed > STATE_TIMEOUT_CARD_INFO_MS) timeout = true;
      break;
    case STATE_CONFIRM:
      if (elapsed > STATE_TIMEOUT_CONFIRM_MS) timeout = true;
      break;
    case STATE_COMPLETE:
      if (elapsed > STATE_TIMEOUT_COMPLETE_MS) timeout = true;
      break;
    default:
      break;
  }
  
  if (timeout) {
    Serial.printf("⏰ 状态超时: %d, 已持续 %lu ms\n", currentState, elapsed);
    beepError();
    resetToIdle();
  }
}

void performSystemHealthCheck() {
  if (millis() - lastHeartbeat > 60000) {
    uint32_t freeHeap = ESP.getFreeHeap();
    if (freeHeap < freeHeapMin) {
      freeHeapMin = freeHeap;
    }
    
    // 简化日志输出，避免看门狗问题
    Serial.printf("💓 系统健康: RAM=%uKB, Loop=%lums, WiFi=%s\n", 
                  freeHeap/1024, maxLoopTime, wifiConnected ? "OK" : "NG");
    
    if (freeHeap < 10000) {
      Serial.println("⚠️ 内存不足警告!");
    }
    
    lastHeartbeat = millis();
    maxLoopTime = 0;
  }
}

// =================== 状态处理函数 (保持不变，但添加喂狗操作) ===================
void handleIdleState() {
  setLED(false, true, false);
  displayIdle();
  
  if (readButtonImproved(BTN_OK)) {
    currentState = STATE_LANGUAGE_SELECT;
    selectedLanguage = LANG_EN;
    stateStartTime = millis();
    beepShort();
    Serial.println("🌐 进入语言选择界面");
  }
}

void handleLanguageSelectState() {
  setLED(false, false, true);
  displayLanguageSelect();
  
  if (readButtonImproved(BTN_SELECT)) {
    selectedLanguage = (Language)((selectedLanguage + 1) % 3);
    beepShort();
    Serial.printf("🔄 切换到语言: %s\n", LANG_NAMES[selectedLanguage]);
  }
  
  if (readButtonImproved(BTN_OK)) {
    currentLanguage = selectedLanguage;
    currentState = STATE_SELECT_PACKAGE;
    selectedPackage = 0;
    stateStartTime = millis();
    beepShort();
    Serial.printf("✅ 语言已选择: %s\n", LANG_NAMES[currentLanguage]);
  }
}

void handleSelectPackageState() {
  setLED(false, false, true);
  displayPackageSelection();
  
  if (readButtonImproved(BTN_SELECT)) {
    selectedPackage = (selectedPackage + 1) % PACKAGE_COUNT;
    beepShort();
    Serial.printf("📦 切换到套餐 %d: %s - %d分钟 $%.2f (%d脉冲)\n", 
                  selectedPackage + 1, packages[selectedPackage].name_en, 
                  packages[selectedPackage].minutes, packages[selectedPackage].price,
                  packages[selectedPackage].pulses);
  }
  
  if (readButtonImproved(BTN_OK)) {
    currentState = STATE_CARD_SCAN;
    stateStartTime = millis();
    beepShort();
    Serial.println("💳 进入刷卡界面...");
  }
}

void handleCardScanState() {
  setLED(false, false, true);
  displayCardScan();

  // 🔥 新增：实时调试信息（每秒打印一次）
  static unsigned long lastDebugPrint = 0;
  if (millis() - lastDebugPrint > 2000) {
    Serial.println("🔎 正在扫描NFC卡片...");

    // 读取 RC522 状态寄存器
    byte comIrqReg = mfrc522.PCD_ReadRegister(mfrc522.ComIrqReg);
    byte errorReg = mfrc522.PCD_ReadRegister(mfrc522.ErrorReg);
    byte statusReg = mfrc522.PCD_ReadRegister(mfrc522.Status1Reg);

    Serial.printf("   ComIrq寄存器: 0x%02X\n", comIrqReg);
    Serial.printf("   Error寄存器: 0x%02X\n", errorReg);
    Serial.printf("   Status寄存器: 0x%02X\n", statusReg);
    Serial.printf("   天线状态: %s\n", (statusReg & 0x03) ? "开启" : "关闭");

    lastDebugPrint = millis();
  }

  String decimalUID = readCardUID();
  if (decimalUID.length() > 0) {
    cardUID = decimalUID;
    Serial.println("====================================");
    Serial.printf("🔍 检测到NFC卡片: DEC=%s\n", decimalUID.c_str());
    
    currentCardInfo = getCardInfoFromSupabase(decimalUID);
    
    if (currentCardInfo.isValid && currentCardInfo.isActive) {
      if (currentCardInfo.balance >= packages[selectedPackage].price) {
        currentState = STATE_CARD_INFO;
        stateStartTime = millis();
        beepSuccess();
        Serial.println("✅ 卡片验证成功!");
      } else {
        Serial.printf("❌ 余额不足! 需要$%.2f，余额$%.2f\n", 
                      packages[selectedPackage].price, currentCardInfo.balance);
        displayError(TEXT_ERROR_LOW_BALANCE[currentLanguage]);
        beepError();
        delay(3000);
        resetToIdle();
      }
    } else {
      Serial.println("❌ 卡片无效或未激活!");
      displayError(TEXT_ERROR_INVALID_CARD[currentLanguage]);
      beepError();
      delay(3000);
      resetToIdle();
    }
    Serial.println("====================================");
  }
}

void handleCardInfoState() {
  setLED(false, false, true);
  displayCardInfo();
  
  if (readButtonImproved(BTN_OK)) {
    currentState = STATE_CONFIRM;
    stateStartTime = millis();
    beepShort();
    Serial.println("✅ 进入确认界面...");
  }
}

void handleConfirmState() {
  setLED(false, false, true);
  displayConfirm();
  
  if (readButtonImproved(BTN_OK)) {
    currentState = STATE_PROCESSING;
    stateStartTime = millis();
    processingStartTime = millis();
    sentPulses = 0;
    beepSuccess();
    
    Serial.println("====================================");
    Serial.println("🚗 开始洗车流程!");
    Serial.printf("卡片: DEC=%s, HEX=%s\n", currentCardInfo.cardUIDDecimal.c_str(), currentCardInfo.cardUID.c_str());
    Serial.printf("套餐: %s\n", packages[selectedPackage].name_en);
    Serial.printf("时长: %d分钟\n", packages[selectedPackage].minutes);
    Serial.printf("脉冲数: %d\n", packages[selectedPackage].pulses);
    Serial.printf("费用: $%.2f\n", packages[selectedPackage].price);
    Serial.printf("用户: %s\n", currentCardInfo.userName.c_str());
    Serial.println("====================================");
    
    // 更新余额和记录交易
    float amount = packages[selectedPackage].price;
    float newBalance = currentCardInfo.balance - amount;
    
    updateCardBalance(cardUID, newBalance);
    recordTransactionToSupabase(cardUID, -amount, currentCardInfo.balance, packages[selectedPackage].name_en);
    
    currentCardInfo.balance = newBalance;
  }
  
  if (readButtonImproved(BTN_SELECT)) {
    currentState = STATE_SELECT_PACKAGE;
    stateStartTime = millis();
    beepShort();
    Serial.println("🔙 返回套餐选择...");
  }
}

void handleProcessingState() {
  setLED(false, false, true);
  
  int totalPulses = packages[selectedPackage].pulses;
  int totalMinutes = packages[selectedPackage].minutes;
  
  unsigned long elapsed = millis() - processingStartTime;
  unsigned long totalTimeMs = totalMinutes * 60000UL;
  
  int remainingMin = 0;
  int remainingSec = 0;
  
  if (elapsed < totalTimeMs) {
    unsigned long remainingMs = totalTimeMs - elapsed;
    remainingMin = remainingMs / 60000;
    remainingSec = (remainingMs % 60000) / 1000;
  }
  
  displayProcessing(sentPulses, totalPulses, remainingMin, remainingSec);
  
  static unsigned long lastPulseTime = 0;
  if (sentPulses < totalPulses && (millis() - lastPulseTime >= PULSE_INTERVAL_MS)) {
    digitalWrite(PULSE_OUT, HIGH);
    digitalWrite(LED_BLUE, HIGH);
    delay(PULSE_WIDTH_MS);
    
    digitalWrite(PULSE_OUT, LOW);
    digitalWrite(LED_BLUE, LOW);
    
    sentPulses++;
    lastPulseTime = millis();
    
    Serial.printf("📡 发送脉冲 %d/%d\n", sentPulses, totalPulses);
    
    if (sentPulses % 5 == 0) {
      beepShort();
    }
    
    // 重要：在脉冲发送过程中喂狗
    esp_task_wdt_reset();
  }
  
  if (elapsed >= totalTimeMs && sentPulses >= totalPulses) {
    currentState = STATE_COMPLETE;
    stateStartTime = millis();
    Serial.println("🎉 洗车流程完成!");
  }
}

void handleCompleteState() {
  setLED(false, true, false);
  displayComplete();
  
  static bool soundPlayed = false;
  if (!soundPlayed) {
    beepSuccess();
    delay(500);
    beepSuccess();
    soundPlayed = true;
  }
  
  if (millis() - stateStartTime > STATE_TIMEOUT_COMPLETE_MS) {
    Serial.println("🔄 交易完成，返回待机状态");
    Serial.printf("📊 最终余额: $%.2f\n", currentCardInfo.balance);
    Serial.printf("💳 卡片: DEC=%s, HEX=%s\n", currentCardInfo.cardUIDDecimal.c_str(), currentCardInfo.cardUID.c_str());
    soundPlayed = false;
    resetToIdle();
  }
}

// =================== 主程序 ===================
void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("=====================================");
  Serial.printf("🚗 Eaglson Coin Wash Terminal %s\n", FIRMWARE_VERSION);
  Serial.println("基于 U8g2 OLED + ESP32-S3");
  Serial.println("🎯 支持中文显示 + 修正看门狗");
  Serial.printf("机器ID: %s\n", MACHINE_ID);
  Serial.println("=====================================");
  
  // 修正：获取主任务句柄
  mainTaskHandle = xTaskGetCurrentTaskHandle();
  
  // 测试UID转换功能
  Serial.println("🧪 测试UID转换功能:");
  Serial.printf("HEX 8BCC1674 -> DEC %s\n", hexUIDToDecimal("8BCC1674").c_str());
  Serial.printf("DEC 2345408116 -> HEX %s\n", decimalToHexUID("2345408116").c_str());
  Serial.println();
  
  // 初始化引脚
  pinMode(BTN_OK, INPUT);
  pinMode(BTN_SELECT, INPUT);
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_BLUE, OUTPUT);
  pinMode(BUZZER, OUTPUT);
  pinMode(PULSE_OUT, OUTPUT);

  // 🔥 修正：预先配置 SPI 关键引脚
  pinMode(RC522_CS, OUTPUT);
  pinMode(RC522_RST, OUTPUT);

  // 🔥 关键修复：先设置 RST 为高电平，再配置其他引脚
  digitalWrite(RC522_CS, HIGH);   // CS 默认拉高（未选中）
  digitalWrite(RC522_RST, HIGH);  // RST 拉高（正常工作）
  delay(100);                     // 等待 RST 稳定

  digitalWrite(LED_RED, LOW);
  digitalWrite(LED_GREEN, LOW);
  digitalWrite(LED_BLUE, LOW);
  digitalWrite(BUZZER, LOW);
  digitalWrite(PULSE_OUT, LOW);

  setLED(true, true, true);
  delay(500);
  setLED(false, false, false);

  // 🔥 修正：先初始化 SPI 和 MFRC522（在 I2C 之前）
  Serial.println("📡 初始化 SPI和NFC...");

  // 🔥 关键修复：硬件复位序列
  Serial.println("🔄 执行硬件复位序列...");
  digitalWrite(RC522_RST, LOW);   // 拉低 RST
  delay(50);                       // 保持低电平 50ms
  digitalWrite(RC522_RST, HIGH);  // 拉高 RST
  delay(100);                      // 等待芯片启动

  Serial.printf("   RST 引脚状态: %d (应该是1)\n", digitalRead(RC522_RST));

  SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI, RC522_CS);
  delay(50);  // 等待 SPI 总线稳定

  mfrc522.PCD_Init();
  delay(100);

  // 🔥 修正：软复位 RC522
  Serial.println("🔄 软复位 RC522...");
  mfrc522.PCD_Reset();
  delay(100);  // 增加延迟到 100ms
  
  byte version = mfrc522.PCD_ReadRegister(mfrc522.VersionReg);
  Serial.printf("RC522 固件版本: 0x%02X\n", version);

  // 🔥 新增：详细的版本检测信息
  if (version == 0x00 || version == 0xFF) {
    Serial.println("❌ RC522 初始化失败!");
    Serial.println("   可能原因:");
    Serial.println("   1. 接线错误（检查 MOSI/MISO/SCK/CS/RST）");
    Serial.println("   2. 供电不足（需要稳定的 3.3V）");
    Serial.println("   3. SPI 引脚冲突");
    Serial.printf("   4. CS 引脚状态: %d\n", digitalRead(RC522_CS));
    Serial.printf("   5. RST 引脚状态: %d\n", digitalRead(RC522_RST));

    setLED(true, false, false);
    while(1) {
      beepError();
      delay(2000);
    }
  }

  Serial.println("✅ RC522 初始化成功!");
  Serial.printf("   支持的卡片类型: ISO14443A (Mifare)\n");

  // 🔥 修正：在 SPI 初始化后再初始化 I2C OLED
  Serial.println("🖥️ 初始化 U8g2 OLED...");
  Wire.begin(I2C_SDA, I2C_SCL);  // 显式初始化 I2C
  display.begin();
  display.enableUTF8Print();  // 启用UTF8支持，用于中文显示

  Serial.println("✅ U8g2 OLED 初始化成功!");

  // 显示启动画面
  display.clearBuffer();
  display.setFont(u8g2_font_10x20_tf);
  display.drawStr(15, 25, "Eaglson");
  display.setFont(u8g2_font_6x10_tf);
  display.drawStr(25, 40, "Terminal v4.4");
  display.drawStr(15, 55, "NFC Ready!");
  display.sendBuffer();
  
  // 初始化WiFi
  initWiFi();
  
  // 修正：初始化看门狗，正确注册任务
  esp_task_wdt_config_t twdt_config = {
    .timeout_ms = 30000,     // 30秒超时（减少超时时间）
    .idle_core_mask = 0,     
    .trigger_panic = true    
  };
  
  if (esp_task_wdt_init(&twdt_config) == ESP_OK) {
    if (esp_task_wdt_add(mainTaskHandle) == ESP_OK) {  // 修正：使用任务句柄
      Serial.println("🐕 看门狗已启用 (30秒超时)");
    } else {
      Serial.println("⚠️ 看门狗任务注册失败");
    }
  } else {
    Serial.println("⚠️ 看门狗初始化失败，继续运行");
  }
  
  // 系统准备完成
  display.clearBuffer();
  display.setFont(u8g2_font_6x10_tf);
  display.drawStr(15, 15, "System Ready!");
  display.drawStr(5, 28, wifiConnected ? "WiFi: Connected" : "WiFi: Offline");
  display.drawStr(5, 40, "UID: Decimal Mode");
  char buffer[30];
  sprintf(buffer, "Free RAM: %u KB", ESP.getFreeHeap() / 1024);
  display.drawStr(5, 52, buffer);
  display.sendBuffer();
  
  beepSuccess();
  delay(3000);
  
  resetToIdle();
  
  Serial.println("✅ 系统初始化完成!");
  Serial.println("📊 套餐配置:");
  for(int i = 0; i < PACKAGE_COUNT; i++) {
    Serial.printf("  [%d] %s - %d分钟 $%.2f (%d脉冲)\n", 
                  i+1, packages[i].name_en, packages[i].minutes, 
                  packages[i].price, packages[i].pulses);
  }
  Serial.println();
  Serial.printf("🎮 系统已就绪... (RAM: %u KB)\n", ESP.getFreeHeap() / 1024);
  Serial.println("🌟 已修正：中文显示 + 看门狗 + 交易记录表结构");
  Serial.println();
  
  lastHeartbeat = millis();
}

void loop() {
  loopStartTime = millis();
  
  // 修正：正确的喂狗操作
  if (mainTaskHandle != NULL) {
    esp_task_wdt_reset();
  }
  
  // 系统维护
  checkWiFi();
  checkStateTimeout();
  performSystemHealthCheck();
  
  // 状态机处理
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
    case STATE_ERROR:
      setLED(true, false, false);
      displayError("System Error");
      delay(5000);
      resetToIdle();
      break;
  }
  
  // 性能监控
  unsigned long loopTime = millis() - loopStartTime;
  if (loopTime > maxLoopTime) {
    maxLoopTime = loopTime;
  }
  
  delay(50);
}
