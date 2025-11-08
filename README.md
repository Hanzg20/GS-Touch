# GoldSky_Lite v0.5

智能洗车支付终端系统 - ESP32-S3 + NFC + OLED

---

## 📋 项目概述

GoldSky_Lite 是一个基于 ESP32-S3 的智能洗车终端系统，支持NFC卡片支付、实时余额查询、离线交易缓存等功能。

### 主要特性

- ✅ **NFC卡片支付** - MFRC522读卡器，支持MIFARE卡片
- ✅ **OLED显示** - 2.42英寸SSD1309 128x64分辨率
- ✅ **WiFi连接** - 自动重连，支持断网缓存
- ✅ **离线交易** - 最多缓存50笔交易，恢复后自动同步
- ✅ **VIP卡系统** - 充值优惠、余额查询
- ✅ **LED状态指示** - 4色LED实时反馈
- ✅ **Supabase集成** - 云端数据库存储

---

## 🎨 界面预览

### 欢迎页
```
┌─────────────────────────────┐
│     EAGLSON WASH            │
│                             │
│  Recharge NOW! Get BONUS... │ ← 滚动广告
│  A:$60  B:$125  C:$240      │
│                   Press OK  │
└─────────────────────────────┘
```

### NFC刷卡页
```
┌─────────────────────────────┐
│       TAP CARD              │
│                             │
│     ╭─╮  )))  ✋           │ ← NFC动画
│    │)))│                   │
│     ╰─╯                    │
│      $5.00 - 5min          │
└─────────────────────────────┘
```

### 洗车进度页
```
┌─────────────────────────────┐
│    Wash in Progress         │
│        04:57                │ ← 倒计时
│                             │
│  1 0 1 0  →  ⚙             │ ← 齿轮动画
│  1/5                        │
└─────────────────────────────┘
```

### 完成页
```
┌─────────────────────────────┐
│      COMPLETE               │
│         ✓                   │ ← 对勾
│    Remain $40.50            │ ← 剩余余额
│                             │
│      Thank You!             │
└─────────────────────────────┘
```

---

## 🛠️ 硬件要求

### 核心组件
- **主控**: ESP32-S3 Dev Module
- **显示**: 2.42" SSD1309 OLED (128×64, I2C)
- **读卡器**: MFRC522 NFC模块 (SPI)
- **LED**: 4×5mm LED (绿/黄/红/蓝)
- **蜂鸣器**: 有源蜂鸣器 5V

### 引脚配置

| 功能 | 引脚 | 说明 |
|------|------|------|
| OLED SDA | GPIO 42 | I2C数据线 |
| OLED SCL | GPIO 41 | I2C时钟线 |
| NFC MISO | GPIO 13 | SPI数据输入 |
| NFC MOSI | GPIO 11 | SPI数据输出 |
| NFC SCK | GPIO 12 | SPI时钟 |
| NFC SS | GPIO 10 | SPI片选 |
| NFC RST | GPIO 14 | 复位引脚 |
| LED绿 | GPIO 5 | 系统正常 |
| LED黄 | GPIO 6 | 处理中 |
| LED红 | GPIO 7 | 错误状态 |
| LED蓝 | GPIO 15 | VIP模式 |
| 蜂鸣器 | GPIO 4 | 音频反馈 |
| 脉冲输出 | GPIO 1 | 继电器控制 |

---

## 💻 软件环境

### 开发工具
- **Arduino IDE**: 2.x 或更高版本
- **ESP32 Board**: 2.0.x

### 依赖库
```
ArduinoJson >= 7.0.0
U8g2 >= 2.35.0
MFRC522 >= 1.4.0
WiFi (ESP32内置)
HTTPClient (ESP32内置)
Preferences (ESP32内置)
```

### 安装步骤

1. **安装ESP32开发板支持**
   ```
   文件 → 首选项 → 附加开发板管理器网址
   https://espressif.github.io/arduino-esp32/package_esp32_index.json
   ```

2. **安装依赖库**
   ```
   工具 → 管理库 → 搜索并安装:
   - ArduinoJson (by Benoit Blanchon)
   - U8g2 (by olikraus)
   - MFRC522 (by GithubCommunity)
   ```

3. **配置开发板**
   ```
   工具 → 开发板 → ESP32S3 Dev Module
   工具 → Partition Scheme → Default 4MB with spiffs
   工具 → USB CDC On Boot → Enabled
   ```

详细编译指南: [COMPILE_GUIDE.md](COMPILE_GUIDE.md)

---

## 🚀 快速开始

### 1. 克隆项目
```bash
git clone https://github.com/Hanzg20/GS-Touch.git
cd GS-Touch/misc_projects/GoldSky_Lite
```

### 2. 配置WiFi和API
编辑 `config.h`:
```cpp
// WiFi配置
#define DEFAULT_WIFI_SSID "YourWiFiSSID"
#define DEFAULT_WIFI_PASSWORD "YourPassword"

// Supabase配置
#define DEFAULT_SUPABASE_URL "https://your-project.supabase.co"
#define DEFAULT_SUPABASE_KEY "your-anon-key"
```

### 3. 编译上传
1. 打开 `GoldSky_Lite.ino`
2. 选择正确的开发板和端口
3. 点击上传按钮

### 4. 测试
参考: [TEST_CHECKLIST_V2.5.md](docs/TEST_CHECKLIST_V2.5.md)

---

## 📚 文档

### 核心文档
- [CHANGELOG.md](CHANGELOG.md) - 版本更新日志
- [COMPILE_GUIDE.md](COMPILE_GUIDE.md) - 编译指南
- [CONFIG_GUIDE.md](CONFIG_GUIDE.md) - 配置说明
- [UPLOAD_CHECKLIST.md](UPLOAD_CHECKLIST.md) - 上传检查清单

### 技术文档 (docs/)
- [OPTIMIZATION_SUMMARY_V2.5.md](docs/OPTIMIZATION_SUMMARY_V2.5.md) - 优化总结
- [NFC_ANIMATION.md](docs/NFC_ANIMATION.md) - NFC动画设计
- [GEAR_ANIMATION.md](docs/GEAR_ANIMATION.md) - 齿轮动画设计
- [UI_ENHANCEMENT_V2.5.md](docs/UI_ENHANCEMENT_V2.5.md) - UI增强说明
- [BUGFIX_V2.6.md](docs/BUGFIX_V2.6.md) - Bug修复报告

---

## 🔧 核心功能

### 离线交易队列
```cpp
// 离线时自动缓存交易
void addToOfflineQueue(String uid, float amount, float balance, String pkg);

// WiFi恢复后自动同步
bool syncOfflineQueue();

// 最多缓存50笔交易
#define MAX_OFFLINE_QUEUE 50
```

### 配置管理
```cpp
// 安全的配置存储
ConfigManager config;
config.init(ssid, password, apiUrl, apiKey);

// 运行时更新
config.setWiFiCredentials(newSSID, newPassword);
```

### API验证
```cpp
// 完整的输入验证
bool validateCardInfoResponse(const JsonDocument& doc);

// 防止异常数据
- 卡号长度检查
- 余额范围验证 (0-10000)
- JSON格式验证
```

---

## 🎯 系统状态机

```
WELCOME → SELECT_PACKAGE → SCAN_CARD → VIP_INFO →
CONFIRM → PROCESSING → COMPLETE → WELCOME
         ↓                          ↑
    VIP_QUERY ←→ VIP_QUERY_SCAN ←─┘
         ↓
       ERROR
```

---

## 📊 v0.5 更新亮点

### 新功能
- ✅ 离线交易队列系统 (最多50笔)
- ✅ 配置管理系统 (NVS存储)
- ✅ API响应验证

### UI优化
- ✅ NFC感应动画 (专业图标)
- ✅ 滚动广告优化 (+25%速度)
- ✅ 齿轮动画简化
- ✅ VIP INFO横向布局

### Bug修复
- ✅ 完成页面显示问题 (可靠性25%→100%)
- ✅ ArduinoJson兼容性
- ✅ 状态管理优化

详细更新: [CHANGELOG.md](CHANGELOG.md)

---

## 🧪 测试

### 基础功能测试
- [ ] WiFi连接
- [ ] NFC读卡
- [ ] OLED显示
- [ ] LED指示
- [ ] 蜂鸣器反馈

### 业务流程测试
- [ ] 套餐选择
- [ ] 余额查询
- [ ] 交易扣款
- [ ] 洗车计时
- [ ] 完成显示

### 异常测试
- [ ] WiFi断开恢复
- [ ] 离线交易同步
- [ ] 无效卡片处理
- [ ] 余额不足提示

完整测试清单: [docs/TEST_CHECKLIST_V2.5.md](docs/TEST_CHECKLIST_V2.5.md)

---

## 🤝 贡献

欢迎提交Issue和Pull Request！

1. Fork项目
2. 创建功能分支 (`git checkout -b feature/AmazingFeature`)
3. 提交更改 (`git commit -m 'Add some AmazingFeature'`)
4. 推送到分支 (`git push origin feature/AmazingFeature`)
5. 开启Pull Request

---

## 📄 许可证

本项目采用 MIT 许可证 - 详见 LICENSE 文件

---

## 👥 维护者

- **Hanzg20** - 主要开发者
- GitHub: [@Hanzg20](https://github.com/Hanzg20)

---

## 🔗 相关链接

- 仓库地址: https://github.com/Hanzg20/GS-Touch.git
- 问题反馈: https://github.com/Hanzg20/GS-Touch/issues
- Supabase: https://supabase.com

---

## 📝 版本历史

- **v0.5** (2025-11-08) - UI优化 + Bug修复 + 离线交易
- **v2.4** (2025-11-07) - 基础功能完成

完整更新日志: [CHANGELOG.md](CHANGELOG.md)
