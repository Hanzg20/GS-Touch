# 📋 GoldSky_Lite 配置指南

本文档说明如何通过修改 `config.h` 文件来调整系统参数。

---

## 🎯 套餐配置（最常用）

### 修改套餐详情和脉冲数

打开 [config.h](config.h)，找到第 68-74 行的 `PACKAGES` 数组：

```cpp
// 套餐数组（可在此修改套餐详情和脉冲数）
static Package PACKAGES[PACKAGE_COUNT] = {
  {"Quick Wash", "Lavage Rapide", "快速洗车", 4, 4.00, 4, false},
  {"Standard", "Standard", "标准洗车", 6, 6.00, 8, false},
  {"Deluxe", "Deluxe", "豪华洗车", 8, 9.00, 9, false},
  {"Premium", "Premium", "至尊洗车", 10, 12.00, 12, false},
  {"VIP Info", "Info VIP", "VIP查询", 0, 0.00, 0, true}
};
```

### 套餐结构说明

每个套餐包含7个字段：
```cpp
{"英文名", "法语名", "中文名", 时长(分钟), 价格, 脉冲数, 是否查询模式}
```

### 示例：修改脉冲数

**场景1：将Standard套餐的脉冲数从8改为10**
```cpp
{"Standard", "Standard", "标准洗车", 6, 6.00, 10, false},  // 原来是8
```

**场景2：增加套餐价格**
```cpp
{"Premium", "Premium", "至尊洗车", 10, 15.00, 15, false},  // 原价12，改为15
```

**场景3：修改洗车时长**
```cpp
{"Deluxe", "Deluxe", "豪华洗车", 10, 9.00, 9, false},  // 原8分钟，改为10分钟
```

---

## ⚙️ 脉冲配置

### 修改脉冲时间参数（第 48-49 行）

```cpp
#define PULSE_WIDTH_MS 100        // 脉冲宽度（毫秒）- 每个脉冲持续时间
#define PULSE_INTERVAL_MS 1000    // 脉冲间隔（毫秒）- 两个脉冲之间的时间
```

**示例：**
- 加快脉冲速度：`#define PULSE_INTERVAL_MS 500` （0.5秒发送一次）
- 延长脉冲宽度：`#define PULSE_WIDTH_MS 200` （200毫秒）

---

## 🔧 超时配置

### 各状态超时时间（第 51-58 行）

```cpp
#define STATE_TIMEOUT_WELCOME_MS 60000      // 欢迎界面：60秒
#define STATE_TIMEOUT_SELECT_MS 20000       // 套餐选择：20秒
#define STATE_TIMEOUT_CARD_SCAN_MS 15000    // 刷卡等待：15秒
#define STATE_TIMEOUT_READY_MS 2000         // 准备状态：2秒
#define STATE_TIMEOUT_PROCESSING_MS 120000  // 洗车进行：120秒
#define STATE_TIMEOUT_COMPLETE_MS 8000      // 完成显示：8秒
#define STATE_TIMEOUT_VIP_QUERY_MS 15000    // VIP查询：15秒
#define STATE_TIMEOUT_VIP_DISPLAY_MS 15000  // VIP显示：15秒
```

**示例：延长刷卡等待时间**
```cpp
#define STATE_TIMEOUT_CARD_SCAN_MS 30000  // 从15秒改为30秒
```

---

## 🛡️ 错误恢复配置（方案A优化）

### 自动恢复参数（第 61-62 行）

```cpp
#define MAX_CONSECUTIVE_ERRORS 5           // 连续错误阈值：5次
#define AUTO_RESTART_TIMEOUT_MS 300000     // 无操作重启：5分钟
```

**示例：**
- 更严格的错误恢复：`#define MAX_CONSECUTIVE_ERRORS 3` （3次错误就重置）
- 延长空闲时间：`#define AUTO_RESTART_TIMEOUT_MS 600000` （10分钟）

---

## 🌐 WiFi和网络配置

### WiFi凭证（第 34-36 行）

```cpp
#define WIFI_SSID "hanzg_hanyh"           // WiFi名称
#define WIFI_PASSWORD "han1314521"        // WiFi密码
#define WIFI_TIMEOUT_MS 20000             // 连接超时：20秒
```

**⚠️ 安全提示：** 生产环境建议使用环境变量或加密配置文件存储凭证。

---

## 🗄️ Supabase后端配置

### API配置（第 39-40 行）

```cpp
#define SUPABASE_URL "https://ttbtxxpnvkcbyugzdqfw.supabase.co"
#define SUPABASE_KEY "eyJhbGci..."  // 完整密钥
```

---

## 📌 引脚配置

### GPIO引脚定义（第 12-31 行）

```cpp
// OLED显示
#define I2C_SDA 8
#define I2C_SCL 9

// NFC读卡器
#define RC522_CS 10
#define SPI_MOSI 11
#define SPI_SCK 12
#define SPI_MISO 13
#define RC522_RST 14

// 按钮
#define BTN_OK 1
#define BTN_SELECT 2

// LED指示灯
#define LED_POWER 3
#define LED_NETWORK 4
#define LED_PROGRESS 5
#define LED_STATUS 6

// 蜂鸣器
#define BUZZER 16

// 脉冲输出
#define PULSE_OUT 7
```

**⚠️ 注意：** 修改引脚需要同步修改硬件连接！

---

## 🌍 多语言文本配置

### 文本数组（第 82-89 行）

```cpp
static const char* TEXT_WELCOME[] = {"Welcome", "Bienvenue", "欢迎"};
static const char* TEXT_SELECT_SERVICE[] = {"Select Service", "Choisir service", "选择服务"};
static const char* TEXT_TAP_CARD[] = {"Tap Your Card", "Appuyez carte", "请刷卡"};
// ... 更多文本
```

**格式：** `{英文, 法语, 中文}`

**示例：修改欢迎文本**
```cpp
static const char* TEXT_WELCOME[] = {"Welcome!", "Bienvenue!", "欢迎光临"};
```

---

## 🔧 系统标识配置

### 机器ID和版本（第 43-45 行）

```cpp
#define MACHINE_ID "EAGLESON_TERMINAL_01"  // 机器唯一标识
#define FIRMWARE_VERSION "v2.4"            // 固件版本
#define MAX_RETRY_COUNT 3                  // 最大重试次数
```

---

## 📝 配置修改流程

### 标准步骤：

1. **编辑配置**
   - 打开 `config.h`
   - 修改相应参数
   - 保存文件

2. **编译验证**
   - Arduino IDE → Verify/Compile (Ctrl+R)
   - 确认无编译错误

3. **上传固件**
   - 连接ESP32-S3开发板
   - Arduino IDE → Upload (Ctrl+U)

4. **功能测试**
   - 串口监视器查看启动日志
   - 测试修改的功能
   - 验证脉冲输出（如有修改）

---

## ⚠️ 常见注意事项

### 1. 套餐数量修改

如果增加或减少套餐数量，必须同步修改：
```cpp
#define PACKAGE_COUNT 5  // 改为实际套餐数量
```

### 2. 脉冲参数范围

建议范围：
- **脉冲宽度：** 50-500ms（太短可能无法触发，太长浪费时间）
- **脉冲间隔：** 500-2000ms（根据设备响应速度调整）
- **脉冲数量：** 1-20个（根据实际洗车设备需求）

### 3. 超时时间合理性

- 刷卡等待不宜超过60秒（用户体验）
- 欢迎界面超时可以较长（吸引注意）
- 完成界面超时不宜太短（用户需要看到结果）

### 4. 内存保护阈值

```cpp
// 在 performHealthCheck() 中（主文件第1135行和1143行）
if (freeHeap < 20000) {  // 严重不足：立即重启
if (freeHeap < 40000) {  // 预警：尝试释放
```

建议保持默认值，除非了解ESP32内存管理。

---

## 🧪 测试检查清单

修改配置后，建议测试：

**套餐修改后：**
- [ ] OLED显示正确的套餐信息（名称、价格、时长）
- [ ] 扣费金额正确
- [ ] 脉冲数量符合设置
- [ ] 洗车时长符合设置

**脉冲修改后：**
- [ ] 脉冲间隔正确（用秒表验证）
- [ ] 脉冲宽度足以触发洗车设备
- [ ] 总洗车时长 = 脉冲数 × 脉冲间隔

**超时修改后：**
- [ ] 超时后正确返回欢迎界面
- [ ] 不会过早超时导致用户操作中断

**错误恢复修改后：**
- [ ] 空闲指定时间后自动重启
- [ ] 连续错误后正确重置

---

## 📚 参考文档

- **主程序：** [GoldSky_Lite.ino](GoldSky_Lite.ino)
- **工具函数：** [GoldSky_Utils.ino](GoldSky_Utils.ino)
- **显示函数：** [GoldSky_Display.ino](GoldSky_Display.ino)
- **优化文档：** [V24_OPTIMIZATION_COMPLETED.md](V24_OPTIMIZATION_COMPLETED.md)

---

## 💡 配置示例

### 示例1：快速洗车场景（低价高周转）

```cpp
static Package PACKAGES[PACKAGE_COUNT] = {
  {"Quick Wash", "Lavage Rapide", "快速洗车", 3, 3.00, 3, false},   // 3分钟3刀
  {"Standard", "Standard", "标准洗车", 5, 5.00, 5, false},         // 5分钟5刀
  {"Deluxe", "Deluxe", "豪华洗车", 7, 7.00, 7, false},             // 7分钟7刀
  {"Premium", "Premium", "至尊洗车", 10, 10.00, 10, false},        // 10分钟10刀
  {"VIP Info", "Info VIP", "VIP查询", 0, 0.00, 0, true}
};

#define PULSE_INTERVAL_MS 600   // 更快的脉冲间隔
```

### 示例2：高端洗车场景（精细服务）

```cpp
static Package PACKAGES[PACKAGE_COUNT] = {
  {"Basic", "Basique", "基础洗车", 8, 10.00, 8, false},      // 8分钟10刀
  {"Premium", "Premium", "高级洗车", 12, 18.00, 12, false},   // 12分钟18刀
  {"Luxury", "Luxe", "豪华洗车", 15, 25.00, 15, false},       // 15分钟25刀
  {"Ultimate", "Ultime", "至尊洗车", 20, 35.00, 20, false},   // 20分钟35刀
  {"VIP Info", "Info VIP", "VIP查询", 0, 0.00, 0, true}
};

#define PULSE_INTERVAL_MS 1500  // 更慢的脉冲间隔（精细控制）
```

---

**最后更新：** 2025-10-30
**版本：** v2.4 Refactored Edition
