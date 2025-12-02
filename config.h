/*
 * GoldSky_Lite 配置文件
 * 版本: v0.5
 *
 * 包含所有系统配置、引脚定义、数据结构和常量
 *
 * v0.5 更新 (2025-11-08):
 * - PendingTransaction 结构 (离线交易)
 * - MAX_OFFLINE_QUEUE 常量
 * - UI优化：NFC动画、滚动广告、齿轮动画
 * - Bug修复：完成页面显示、VIP INFO布局
 */

#ifndef CONFIG_H
#define CONFIG_H

// =================== 引脚配置 ===================
// OLED显示屏：2.42" SSD1309 128x64 I2C
// 硬件上电复位电路：VCC --[10kΩ]-- RST --[2.2μF]-- GND
#define OLED_WIDTH 128
#define OLED_HEIGHT 62
#define I2C_SDA 42  // 修改：原来是8
#define I2C_SCL 41  // 修改：原来是9

// =================== 显示区域配置 ===================
// 物理可视窗口: 55mm × 23mm
// 对应像素区域: 127 × 53 像素 (垂直居中，上下各留5-6像素)
#define DISPLAY_VISIBLE_WIDTH 127    // 可视宽度（像素）
#define DISPLAY_VISIBLE_HEIGHT 45    // 51 可视高度（像素）
#define DISPLAY_OFFSET_X 0           // 水平偏移（居中）
#define DISPLAY_OFFSET_Y 9           // 垂直偏移（向下移动1个单位：6→7）

// 边框样式 (0=无边框, 1=单线, 2=双线)
#define BORDER_STYLE 0               // 无边框（节省空间）
#define RC522_CS 10   //   CS=SAD
#define SPI_MOSI 11
#define SPI_SCK 12
#define SPI_MISO 13
#define RC522_RST 14
/*
模块引脚	ESP32-S3	说明
VCC	3.3V	电源（注意：必须是3.3V，不能用5V）
GND	GND	地线
RXD (MOSI)	GPIO 11	SPI数据输入
TXD (MISO)	GPIO 13	SPI数据输出
OUT (SCK)	GPIO 12	SPI时钟
KEY (CS/SDA)	GPIO 10	片选  
VT (RST)	GPIO 14	复位        


新模块标注	功能	ESP32-S3引脚	config.h定义
VCC	电源	3.3V	-
GND	地线	GND	-
RXD	MOSI	GPIO 11	SPI_MOSI
TXD	MISO	GPIO 13	SPI_MISO
OUT	SCK	GPIO 12	SPI_SCK
KEY	CS/SDA	GPIO 10	RC522_CS
VT	RST	GPIO 14	RC522_RST
*/


#define BTN_OK 1
#define BTN_SELECT 2
#define BUZZER 16

#define LED_POWER 4
#define LED_NETWORK 5
#define LED_PROGRESS 6
#define LED_STATUS 7

// LED极性配置
// true = 共阳极 (HIGH=熄灭, LOW=点亮)
// false = 共阴极 (HIGH=点亮, LOW=熄灭)
#define LED_COMMON_ANODE true

#define PULSE_OUT 3

// =================== WiFi和网络配置 ===================
// 注意：请在实际部署时修改为您的WiFi凭证
#define WIFI_SSID "YourWiFiSSID"
#define WIFI_PASSWORD "YourWiFiPassword"
#define WIFI_TIMEOUT_MS 20000

// =================== Supabase 配置 ===================
// 注意：请在实际部署时修改为您的Supabase配置
#define SUPABASE_URL "https://your-project.supabase.co"
#define SUPABASE_KEY "your-anon-key-here"

// =================== 日志配置 ===================
// 日志级别定义
#define LOG_LEVEL_NONE     0  // 完全关闭（不推荐）
#define LOG_LEVEL_ERROR    1  // 仅错误（稳定商用）
#define LOG_LEVEL_WARN     2  // 警告+错误
#define LOG_LEVEL_INFO     3  // 重要信息（初期商用推荐）⭐
#define LOG_LEVEL_DEBUG    4  // 调试信息（开发环境）
#define LOG_LEVEL_VERBOSE  5  // 详细信息（深度调试）

// 当前日志级别（商用建议：INFO初期 → WARN稳定后）
// 注意：这里不用 #define，而是用全局变量，以便运行时通过串口命令修改
// 在 GoldSky_Lite.ino 中定义: int CURRENT_LOG_LEVEL = LOG_LEVEL_INFO;

// 敏感信息脱敏（商用必须开启）
#define LOG_MASK_SENSITIVE false  // false = 显示完整卡号（调试用）

// =================== 系统配置 ===================
#define MACHINE_ID "VIP_TERMINAL_01"
#define FIRMWARE_VERSION "v2.4"
#define MAX_RETRY_COUNT 3

// =================== 脉冲和超时配置 ===================
// Nayax标准: active=500ms, inactive=500ms, 总周期=1000ms
#define PULSE_WIDTH_MS 500        // 脉冲高电平时间（匹配Nayax的50×10ms）
#define PULSE_INTERVAL_MS 1000    // 脉冲周期（高电平+低电平）

#define STATE_TIMEOUT_WELCOME_MS 60000
#define STATE_TIMEOUT_SELECT_MS 20000
#define STATE_TIMEOUT_CARD_SCAN_MS 15000
#define STATE_TIMEOUT_READY_MS 2000
#define STATE_TIMEOUT_PROCESSING_MS 1200000  // 20分钟（最长套餐15分钟+缓冲）
#define STATE_TIMEOUT_COMPLETE_MS 5000       // 5秒（缩短完成页显示时间）
#define STATE_TIMEOUT_VIP_QUERY_MS 15000
#define STATE_TIMEOUT_VIP_DISPLAY_MS 15000

// =================== 错误恢复配置（方案A优化1）===================
#define MAX_CONSECUTIVE_ERRORS 5
#define AUTO_RESTART_TIMEOUT_MS 300000  // 5分钟无操作自动重启

// =================== 健壮性增强配置 ===================
#define NFC_RETRY_INTERVAL_MS 30000     // NFC重试间隔：30秒
#define WIFI_RETRY_INTERVAL_MS 60000    // WiFi重试间隔：60秒
#define ALLOW_OFFLINE_MODE true         // 允许离线模式运行
#define FAULT_LED_BLINK_INTERVAL 300    // 故障LED闪烁间隔（ms）

// =================== LED状态枚举 ===================
enum LEDStatus {
  LED_OFF,
  LED_ON,
  LED_BLINK_SLOW,
  LED_BLINK_FAST
};

// =================== 系统状态枚举 ===================
enum SystemState {
  STATE_WELCOME,
  STATE_SELECT_PACKAGE,
  STATE_CARD_SCAN,
  STATE_SYSTEM_READY,
  STATE_PROCESSING,
  STATE_COMPLETE,
  STATE_VIP_QUERY,
  STATE_VIP_DISPLAY,
  STATE_ERROR
};

enum Language {
  LANG_EN,
  LANG_FR,
  LANG_CN
};

// =================== 显示区域结构 ===================
struct DisplayArea {
  int x;      // 起始X坐标
  int y;      // 起始Y坐标
  int width;  // 可用宽度
  int height; // 可用高度
};

// =================== 套餐结构 ===================
struct Package {
  const char* name_en;
  const char* name_fr;
  const char* name_cn;
  int minutes;
  float price;
  int pulses;
  bool isQuery;
};

// =================== 套餐配置 ===================
// 匹配Nayax配置: Pulse Increment = 4,5,8,15; Prices = $4,$5,$8,$15
#define PACKAGE_COUNT 5

// 套餐数组（可在此修改套餐详情和脉冲数）
static Package PACKAGES[PACKAGE_COUNT] = {
  {"4 Min Wash", "Lavage 4 min", "4分钟洗车", 4, 4.00, 4, false},   // $4/4min
  {"7 Min Wash", "Lavage 7 min", "7分钟洗车", 7, 6.00, 7, false},   // $6/7min
  {"9 Min Wash", "Lavage 9 min", "9分钟洗车", 9, 8.00, 9, false},   // $8/9min
  {"14 Min Wash", "Lavage 14 min", "14分钟洗车", 14, 12.00, 14, false}, // $12/14min
  {"VIP Info", "Info VIP", "VIP查询", 0, 0.00, 0, true}              // VIP查询选项
};

// 会员类型
static const char* MEMBER_TYPES[] = {
  "Basic", "Silver", "Gold", "Platinum", "Diamond", "VIP", "Premium", "Elite"
};

// 多语言文本
static const char* TEXT_WELCOME[] = {"Welcome", "Bienvenue", "欢迎"};
static const char* TEXT_SELECT_SERVICE[] = {"Select Service", "Choisir service", "选择服务"};
static const char* TEXT_TAP_CARD[] = {"Tap Your Card", "Appuyez carte", "请刷卡"};
static const char* TEXT_PROCESSING[] = {"Starting Wash", "Démarrage", "启动洗车"};
static const char* TEXT_COMPLETE[] = {"Complete!", "Termine!", "完成!"};
static const char* TEXT_THANK_YOU[] = {"Thank You!", "Merci!", "谢谢!"};
static const char* TEXT_ERROR_LOW_BALANCE[] = {"Low Balance", "Solde insuffisant", "余额不足"};
static const char* TEXT_ERROR_INVALID_CARD[] = {"Invalid Card", "Carte invalide", "卡片无效"};

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

// =================== 卡片信息结构 ===================
struct CardInfo {
  String cardNumber;
  String displayCardNumber;
  String cardUID;
  String cardUIDDecimal;
  float balance;
  bool isActive;
  bool isValid;
  String userName;
  String cardType;
  String lastTransactionDate;

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

// =================== 离线交易结构 ===================
struct PendingTransaction {
  String cardUID;
  float amount;
  float balanceBefore;
  String packageName;
  unsigned long timestamp;

  void clear() {
    cardUID = "";
    amount = 0.0;
    balanceBefore = 0.0;
    packageName = "";
    timestamp = 0;
  }
};

#define MAX_OFFLINE_QUEUE 50  // 最多缓存50笔交易

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

#endif // CONFIG_H
