# 🚗 GoldSky_Lite - 智能NFC洗车终端系统

**版本：** v5.1 Final Stable
**状态：** ✅ 生产就绪
**最后更新：** 2025-10-30

---

## 📖 项目概述

GoldSky_Lite 是一款基于 ESP32-S3 和 MFRC522 的**智能NFC洗车支付终端**，专为自助洗车行业设计。

### 核心功能
- ✅ NFC刷卡支付（支持 Mifare 卡片）
- ✅ 多语言界面（英语/法语/中文）
- ✅ 4种洗车套餐（4/6/8/10分钟）
- ✅ 脉冲控制外部计时器
- ✅ WiFi云端同步（Supabase）
- ✅ 离线模式支持
- ✅ 实时倒计时显示
- ✅ 完整的错误处理

### 技术特点
- **硬件平台：** ESP32-S3 NXRX
- **NFC模块：** MFRC522（SPI接口）
- **显示屏：** SSD1306 OLED（128×64，I2C）
- **开发语言：** Arduino C/C++
- **代码规模：** 1134 行
- **架构：** 8状态有限状态机

---

## 📚 文档导航

### 🚀 快速开始
- **[快速部署指南](QUICK_START.md)** - 5分钟完成设备部署
- **[优化指南](OPTIMIZATION_GUIDE.md)** - 5项实用优化方案
- **[优化示例代码](APPLY_OPTIMIZATION_EXAMPLE.ino)** - 优化代码片段

### 🔧 技术文档
- **[硬件测试程序](RC522_Test.ino)** - RC522 诊断工具
- **[接线说明](#硬件接线)** - 完整引脚定义
- **[API接口文档](#api接口)** - Supabase 集成说明

### 📝 其他资源
- **[doc/](doc/)** - 历史文档和备份
  - 业务需求文档
  - 项目状态报告
  - Phase 1 开发总结
  - Arduino IDE 使用指南

---

## ⚡ 快速开始

### 1. 硬件准备

| 组件 | 型号 | 数量 |
|------|------|------|
| 主控 | ESP32-S3 Dev Module | 1 |
| NFC | MFRC522 模块 | 1 |
| 显示 | SSD1306 OLED (128×64) | 1 |
| 按钮 | 6×6mm 轻触开关 | 2 |
| LED | 5mm 红/绿/蓝 | 3 |
| 蜂鸣器 | 5V 有源蜂鸣器 | 1 |

### 2. 软件安装

```bash
# Arduino IDE 库依赖
- U8g2 (>= 2.34)
- MFRC522 (>= 1.4.10)
- ArduinoJson (>= 6.21)
- WiFi (ESP32内置)
- HTTPClient (ESP32内置)
```

### 3. 配置修改

打开 `GoldSky_Lite.ino`，修改以下配置：

```cpp
// WiFi配置
#define WIFI_SSID "YOUR_WIFI_NAME"
#define WIFI_PASSWORD "YOUR_PASSWORD"

// 机器ID（每台设备唯一）
#define MACHINE_ID "EAGLESON_TERMINAL_01"

// 调试模式（生产环境改为 false）
#define DEBUG_MODE true
```

### 4. 上传固件

1. 选择开发板：**ESP32S3 Dev Module**
2. 选择串口
3. 点击**上传**
4. 等待完成

详细步骤请参考 **[快速部署指南](QUICK_START.md)**

---

## 🔌 硬件接线

### NFC 模块 (MFRC522)
```
RC522    →  ESP32-S3
────────────────────
SDA (CS) →  GPIO 10
SCK      →  GPIO 12
MOSI     →  GPIO 11
MISO     →  GPIO 13
RST      →  GPIO 14
3.3V     →  3.3V
GND      →  GND
```

### OLED 显示屏 (SSD1306)
```
OLED  →  ESP32-S3
─────────────────
SDA   →  GPIO 8
SCL   →  GPIO 9
VCC   →  3.3V
GND   →  GND
```

### 按钮和 LED
```
组件       →  ESP32-S3
───────────────────────
OK 按钮    →  GPIO 1
SELECT按钮 →  GPIO 2
绿色 LED   →  GPIO 5
蓝色 LED   →  GPIO 6
红色 LED   →  GPIO 7
蜂鸣器     →  GPIO 16
脉冲输出   →  GPIO 4
```

完整接线图请参考 **doc/** 目录。

---

## 📊 套餐配置

| 套餐 | 时长 | 价格 | 脉冲数 |
|------|------|------|--------|
| Quick Wash | 4分钟 | $4.00 | 4 |
| Standard | 6分钟 | $6.00 | 8 |
| Deluxe | 8分钟 | $9.00 | 9 |
| Premium | 10分钟 | $12.00 | 12 |

**修改套餐：** 编辑第81-86行的 `packages[]` 数组。

---

## 🔄 业务流程

```
待机（IDLE）
    ↓ [按OK键]
选择语言（LANGUAGE_SELECT）
    ↓ [SELECT切换 + OK确认]
选择套餐（SELECT_PACKAGE）
    ↓ [SELECT切换 + OK确认]
刷卡（CARD_SCAN）
    ↓ [检测到NFC卡片]
显示卡片信息（CARD_INFO）
    ↓ [按OK]
确认支付（CONFIRM）
    ↓ [OK=支付 / SELECT=返回]
洗车进行中（PROCESSING）
    ↓ [脉冲发送完成]
完成（COMPLETE）
    ↓ [10秒超时]
回到待机
```

---

## 🌐 API 接口

### Supabase 集成

**数据库表结构：**

```sql
-- VIP 卡片表
CREATE TABLE jc_vip_cards (
  id BIGSERIAL PRIMARY KEY,
  card_uid BIGINT UNIQUE NOT NULL,
  card_credit DECIMAL(10,2) DEFAULT 0,
  is_active BOOLEAN DEFAULT true,
  cardholder_name TEXT,
  member_type TEXT,
  created_at TIMESTAMPTZ DEFAULT NOW()
);

-- 交易历史表
CREATE TABLE jc_transaction_history (
  id BIGSERIAL PRIMARY KEY,
  machine_id TEXT NOT NULL,
  card_uid BIGINT NOT NULL,
  transaction_type TEXT,
  transaction_amount DECIMAL(10,2),
  balance_before DECIMAL(10,2),
  third_party_reference TEXT,
  created_at TIMESTAMPTZ DEFAULT NOW()
);
```

**API 端点：**

1. **查询卡片信息**
   ```
   GET /rest/v1/jc_vip_cards?card_uid=eq.{UID}
   ```

2. **更新余额**
   ```
   PATCH /rest/v1/jc_vip_cards?card_uid=eq.{UID}
   Body: {"card_credit": 新余额}
   ```

3. **记录交易**
   ```
   POST /rest/v1/jc_transaction_history
   Body: {交易数据}
   ```

---

## 🛠️ 优化建议

已为你准备了**5项实用优化方案**，按需实施：

| 优化项 | 难度 | 收益 | 实施时间 | 优先级 |
|--------|------|------|----------|--------|
| 1. 错误自动恢复 | ⭐ | ⭐⭐⭐⭐⭐ | 15分钟 | 🔴 高 |
| 2. 内存泄漏防护 | ⭐ | ⭐⭐⭐⭐ | 10分钟 | 🔴 高 |
| 3. NFC读卡增强 | ⭐⭐ | ⭐⭐⭐⭐ | 20分钟 | 🟡 中 |
| 4. 调试开关 | ⭐ | ⭐⭐⭐ | 10分钟 | 🟡 中 |
| 5. 配置文档化 | ⭐ | ⭐⭐⭐⭐ | 30分钟 | 🟢 低 |

**详细说明：** 参考 **[优化指南](OPTIMIZATION_GUIDE.md)**

**代码示例：** 参考 **[优化示例代码](APPLY_OPTIMIZATION_EXAMPLE.ino)**

---

## 🧪 测试与验证

### 功能测试清单

- [ ] 硬件初始化
  - [ ] RC522 固件版本 = 0x91/0x92
  - [ ] OLED 显示正常
  - [ ] LED 自检通过
  - [ ] 蜂鸣器正常

- [ ] WiFi 连接
  - [ ] 连接成功并获取IP
  - [ ] 断线自动重连
  - [ ] 离线模式切换

- [ ] NFC 读卡
  - [ ] UID 读取正确
  - [ ] 卡片类型识别
  - [ ] 防重复读取

- [ ] 业务流程
  - [ ] 语言切换
  - [ ] 套餐选择
  - [ ] 支付确认
  - [ ] 脉冲输出
  - [ ] 倒计时显示

### 性能基准

**正常指标：**
- 启动时间：~10秒
- 内存占用：60-80KB（剩余200KB+）
- WiFi连接：5-10秒
- 刷卡响应：<500ms
- 完整交易：10-15秒

**异常阈值：**
- ⚠️ 内存 < 40KB
- ⚠️ Loop时间 > 500ms
- ⚠️ WiFi重连 > 3次/小时

---

## 🐛 故障排查

### 常见问题

**1. RC522 初始化失败 (0x00)**
- 检查MOSI/MISO是否接反
- 测量供电电压（应为3.2-3.4V）
- 使用 [RC522_Test.ino](RC522_Test.ino) 诊断

**2. OLED 无显示**
- 检查I2C地址（默认0x3C）
- 确认SDA/SCL接线

**3. WiFi 连接失败**
- 确认SSID和密码
- 检查是否为2.4GHz网络（不支持5GHz）
- 尝试靠近路由器

**4. 系统频繁重启**
- 检查供电稳定性（建议5V 2A）
- 查看内存警告
- 调整看门狗超时

更多问题请参考 **[快速部署指南](QUICK_START.md)** 的故障排查章节。

---

## 📈 版本历史

### v5.1 (2025-10-30) - 当前版本
- ✅ 修正RC522初始化顺序
- ✅ 优化硬件复位序列
- ✅ 修复UID转换溢出
- ✅ 增强调试日志
- ✅ 完善错误提示

### v4.4 (2025-10-26)
- ✅ 使用U8g2库支持中文显示
- ✅ 修正看门狗任务注册
- ✅ 更新交易记录表结构

### v4.0 (2025-10-20)
- ✅ 完整业务流程实现
- ✅ WiFi和Supabase集成
- ✅ 多语言支持
- ✅ 脉冲控制逻辑

---

## 📞 技术支持

### 获取帮助

- **文档：** 查看 doc/ 目录
- **测试工具：** 使用 RC522_Test.ino
- **优化指南：** OPTIMIZATION_GUIDE.md
- **快速部署：** QUICK_START.md

### 贡献代码

欢迎提交优化建议和Bug报告。

---

## 📄 许可证

Copyright © 2025 Eaglson Development Team
All Rights Reserved

本项目为商业项目，未经授权不得复制、修改或分发。

---

## 🎉 致谢

感谢以下开源项目：
- **U8g2** - OLED显示库
- **MFRC522** - NFC读卡库
- **ArduinoJson** - JSON解析库
- **ESP32** - 硬件平台

---

**最后更新：** 2025-10-30
**维护者：** Eaglson Development Team
**项目状态：** ✅ 生产就绪
