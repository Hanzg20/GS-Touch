# GoldSky_Lite v0.0.8

智能洗车支付终端系统 - ESP32-S3 + NFC + OLED

---

## 项目概述

GoldSky_Lite 是基于 ESP32-S3 的智能洗车终端系统，支持NFC卡片支付、实时余额查询、离线交易缓存等功能。

### 主要特性

- **NFC卡片支付** - MFRC522读卡器，支持MIFARE卡片
- **OLED显示** - 2.42英寸SSD1309 128x64分辨率
- **WiFi连接** - 自动重连，支持断网缓存
- **离线交易** - 最多缓存50笔交易，恢复后自动同步
- **VIP卡系统** - 充值优惠、余额查询
- **商用日志** - 5级日志系统，敏感数据脱敏
- **Supabase集成** - 云端数据库存储

---

## 套餐配置

| 套餐 | 价格 | 时长 | 脉冲数 |
|------|------|------|--------|
| 套餐1 | $4.00 | 4分钟 | 4 |
| 套餐2 | $6.00 | 7分钟 | 7 |
| 套餐3 | $8.00 | 9分钟 | 9 |
| 套餐4 | $12.00 | 14分钟 | 14 |

---

## 硬件配置

### 核心组件
- **主控**: ESP32-S3 Dev Module
- **显示**: 2.42" SSD1309 OLED (128x64, I2C)
- **读卡器**: MFRC522 NFC模块 (SPI)
- **LED**: 4个状态指示灯
- **蜂鸣器**: 有源蜂鸣器

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
| 脉冲输出 | GPIO 3 | 继电器控制 |
| 蜂鸣器 | GPIO 4 | 音频反馈 |

---

## 快速开始

### 1. 配置WiFi和API

编辑 `config.h`:
```cpp
#define WIFI_SSID "YourWiFiSSID"
#define WIFI_PASSWORD "YourPassword"
#define SUPABASE_URL "https://your-project.supabase.co"
#define SUPABASE_KEY "your-anon-key"
```

### 2. 编译上传

1. Arduino IDE 打开 `GoldSky_Lite.ino`
2. 选择开发板: ESP32S3 Dev Module
3. 选择端口并上传

### 3. 串口监视器

波特率: 115200

---

## 日志系统

### 日志级别

| 级别 | 说明 | 商用建议 |
|------|------|----------|
| ERROR | 仅错误 | 稳定后使用 |
| WARN | 警告+错误 | 稳定后使用 |
| INFO | 重要信息 | 初期推荐 |
| DEBUG | 调试信息 | 开发环境 |
| VERBOSE | 详细信息 | 深度调试 |

### 串口命令

```
log info     - 设置为INFO级别（商用推荐）
log debug    - 设置为DEBUG级别（调试）
log status   - 查看当前状态
cache        - 查看离线缓存
help         - 显示帮助
```

---

## 文件结构

```
GoldSky_Lite/
├── GoldSky_Lite.ino      # 主程序
├── GoldSky_Display.ino   # 显示函数
├── GoldSky_Utils.ino     # 工具函数
├── config.h              # 配置文件
├── ConfigManager.h       # 配置管理类
├── README.md             # 本文档
├── CHANGELOG.md          # 版本历史
└── docs/                 # 技术文档
```

---

## 相关文档

- [doc/CHANGELOG.md](doc/CHANGELOG.md) - 版本更新日志
- [doc/CONFIGMANAGER_GUIDE.md](doc/CONFIGMANAGER_GUIDE.md) - 配置管理指南
- [doc/OFFLINE_CACHE_GUIDE.md](doc/OFFLINE_CACHE_GUIDE.md) - 离线缓存指南
- [doc/PRODUCTION_CONFIG_GUIDE.md](doc/PRODUCTION_CONFIG_GUIDE.md) - 生产环境配置

---

## 版本历史

- **v0.0.7** (2025-11-24) - 商用日志系统 + UI优化 + 套餐更新 + 文档整理
- **v0.0.6** (2025-11-12) - NFC动画改进 + 完成页优化
- **v0.0.5** (2025-11-08) - UI优化 + Bug修复 + 离线交易系统

---

## 许可证

MIT License

---

## 维护者

- **Hanzg20** - GitHub: [@Hanzg20](https://github.com/Hanzg20)
- 仓库: https://github.com/Hanzg20/GS-Touch
