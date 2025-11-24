# Eaglson Coin Wash 洗车终端系统 - 业务流程需求文档

**项目名称**: Eaglson Coin Wash - Intelligent Car Wash Terminal System
**硬件平台**: ESP32-S3
**版本**: v1.0
**文档日期**: 2024-01-28
**状态**: 需求确认中

---

## 📋 文档目录

1. [系统概述](#系统概述)
2. [核心业务流程](#核心业务流程)
3. [功能详细需求](#功能详细需求)
4. [技术规格](#技术规格)
5. [界面设计](#界面设计)
6. [异常处理](#异常处理)
7. [数据流](#数据流)
8. [测试需求](#测试需求)

---

## 1. 系统概述

### 1.1 项目背景

Eaglson Coin Wash 是一款基于ESP32的智能洗车终端系统，旨在提供全自动化、用户友好的自助洗车服务。系统支持多语言界面、NFC卡片支付、Supabase云数据库集成，实现从用户下单到洗车执行的全流程自动化。

### 1.2 系统特性

- ✅ **多语言支持**: 英文、Français、中文
- ✅ **NFC支付**: RC522/PKa532 读卡器
- ✅ **云端集成**: Supabase 数据库
- ✅ **套餐管理**: 灵活的套餐配置
- ✅ **实时反馈**: OLED 显示 + LED 指示 + 蜂鸣器
- ✅ **脉冲控制**: 精确控制洗车计时器
- ✅ **交易记录**: 完整的交易历史

### 1.3 硬件配置


| 组件   | 型号         | 连接            |
| ------ | ------------ | --------------- |
| 主控   | ESP32-S3     | -               |
| 显示   | OLED 128×64 | I2C (GPIO8/9)   |
| 读卡器 | RC522        | SPI (GPIO10-14) |
| 按钮   | OK + SELECT  | GPIO1, GPIO2    |
| LED    | RGB          | GPIO5/6/7       |
| 蜂鸣器 | -            | GPIO16          |
| 继电器 | -            | GPIO4           |

### 1.4 软件架构

- **开发环境**: Arduino IDE 2.x
- **核心框架**: 状态机驱动
- **数据库**: Supabase (PostgreSQL)
- **通信协议**: WiFi + HTTPS
- **支付系统**: NFC + 云端验证

---

## 2. 核心业务流程

### 2.1 业务流程总览

```
┌─────────────────────────────────────────────────────────┐
│                Eaglson Coin Wash                         │
│              完整业务流程流程图                          │
└─────────────────────────────────────────────────────────┘

  开始
    ↓
[1. 系统初始化]
    ↓
[2. 语言选择] ──→ EN / FR / CN
    ↓
[3. 套餐选择] ──→ 5min/$5 / 10min/$15 / 15min/$20 / 30min/$30
    ↓
[4. 刷卡检测] ──→ NFC 卡片
    ↓
[5. 卡片验证] ──→ Supabase API
    ├─ 有效 → [6. 显示卡片信息]
    └─ 无效 → [错误提示] → 返回步骤3
    ↓
[7. 支付确认] ──→ 显示消费金额和余额
    ↓
[8. 执行洗车] ──→ 发送脉冲信号
    ├─ 扣除余额
    ├─ 记录交易
    └─ 控制洗车设备
    ↓
[9. 完成提示] ──→ 显示完成信息
    ↓
返回待机状态
```

### 2.2 业务流程详细描述

#### 🔵 步骤1: 系统初始化与语言选择

**功能描述**:

- 系统启动后进入待机状态
- 显示多语言欢迎界面
- 用户选择界面语言

**技术实现**:

```cpp
// 系统状态
enum SystemState {
  STATE_IDLE,            // 待机
  STATE_LANGUAGE_SELECT,  // 语言选择
  STATE_SELECT_PACKAGE,  // 套餐选择
  STATE_CARD_SCAN,       // 刷卡检测
  STATE_CARD_INFO,       // 卡片信息
  STATE_CONFIRM,         // 支付确认
  STATE_PROCESSING,      // 洗车执行
  STATE_COMPLETE          // 完成
};

// 语言枚举
enum Language {
  LANG_EN,  // English (默认)
  LANG_FR,  // Français
  LANG_CN   // 中文
};
```

**用户界面**:


| 语言 | 欢迎标语                       |
| ---- | ------------------------------ |
| 英文 | Welcome to Eaglson Coin Wash   |
| 法文 | Bienvenue au Lave-Auto Eaglson |
| 中文 | 欢迎使用壹狗剩自助洗车服务     |

**操作指引**:


| 语言 | 操作提示                                  |
| ---- | ----------------------------------------- |
| 英文 | Press $ to switch, OK to confirm          |
| 法文 | Appuyez $ pour changer, OK pour confirmer |
| 中文 | 按$切换，OK确认                           |

**超时设置**:

- 20秒无操作自动返回待机
- 超时提示: "Timeout, returning to welcome screen"

**交互反馈**:

- SELECT 按钮: 切换语言，LED 闪烁，蜂鸣器短响
- OK 按钮: 确认选择，LED 常亮，蜂鸣器长响
- LED 状态: 🟢 绿灯 → 🔵 蓝灯

#### 🟢 步骤2: 洗车套餐选择

**功能描述**:

- 显示洗车套餐列表
- 用户选择需要的套餐
- 显示价格和时长信息

**套餐配置** (参考项目配置):


| 序号 | 套餐名称   | 时长   | 价格   | 脉冲数 |
| ---- | ---------- | ------ | ------ | ------ |
| 1    | Quick Wash | 5分钟  | $5.00  | 5个    |
| 2    | Standard   | 10分钟 | $15.00 | 10个   |
| 3    | Deluxe     | 15分钟 | $20.00 | 15个   |
| 4    | Premium    | 30分钟 | $30.00 | 30个   |

**技术实现**:

```cpp
struct Package {
  const char* name;
  int minutes;      // 时长（分钟）
  int price;        // 价格（美元）
  int pulses;       // 脉冲数量
};

Package packages[] = {
  {"Quick Wash",   5,   5,  5},
  {"Standard",    10,  15, 10},
  {"Deluxe",      15,  20, 15},
  {"Premium",     30,  30, 30}
};
```

**用户界面设计** (128×64 OLED):

```
┌─────────────────────────┐
│  Select Package         │  <- 标题
├─────────────────────────┤
│ [1] 5min   $5   ←高亮  │  <- 套餐1 (选中)
│ [2] 10min  $15         │  <- 套餐2
│ [3] 15min  $20         │  <- 套餐3
│ [4] 30min  $30         │  <- 套餐4
└─────────────────────────┘
```

**操作指引**:


| 语言 | 操作提示                                        |
| ---- | ----------------------------------------------- |
| 英文 | Press $ to select, OK to confirm                |
| 法文 | Appuyez $ pour sélectionner, OK pour confirmer |
| 中文 | 按$选择，OK确认                                 |

**超时设置**:

- 30秒无操作自动返回语言选择
- 超时提示: "Selection timeout, please start again"

**交互反馈**:

- SELECT 按钮: 切换套餐，LED 闪烁，蜂鸣器短响
- OK 按钮: 确认套餐，进入刷卡界面
- LED 状态: 🔵 蓝灯闪烁

#### 🟡 步骤3: 卡片验证与支付确认

##### 3.1 刷卡检测

**功能描述**:

- 显示刷卡提示界面
- 等待用户将NFC卡片贴近读卡器
- 读取卡片UID

**用户界面**:

```
┌─────────────────────────┐
│  Please tap card        │  <- 刷卡提示
├─────────────────────────┤
│                         │
│       ◉ ◯ ◉            │  <- NFC图标
│        NFC              │
│                         │
│      Scanning...        │  <- 扫描中
└─────────────────────────┘
```

**操作指引**:


| 语言 | 提示文字                     |
| ---- | ---------------------------- |
| 英文 | Please tap your card         |
| 法文 | Veuillez appuyer votre carte |
| 中文 | 请刷卡                       |

**超时设置**:

- 15秒未检测到卡片自动返回套餐选择
- 超时提示: "No card detected, please try again"

##### 3.2 卡片验证 (Supabase API)

**功能描述**:

- 调用Supabase API查询卡片信息
- 验证卡片有效性
- 检查余额是否充足

**API接口设计**:

```javascript
// Supabase 卡片查询接口
GET /rest/v1/jc_vip_cards?card_uid=eq.{uid}

// 响应数据
{
  "card_uid": "2345408116",
  "card_number": "CARD-1234",
  "card_credit": 76.00,
  "is_active": true,
  "user_name": "John Doe",
  "user_email": "john@example.com",
  "card_type": "VIP"
}
```

**验证规则**:

1. **卡片存在性检查**: 卡片UID必须在数据库中
2. **激活状态检查**: `is_active` 必须为 `true`
3. **余额充足检查**: `card_credit >= package_price`

**技术实现**:

```cpp
struct CardInfo {
  String cardNumber;   // 卡号（脱敏）
  float balance;       // 余额
  bool isActive;       // 激活状态
  bool isValid;        // 有效性
  String userName;     // 用户名
  String userEmail;    // 用户邮箱
};

CardInfo validateCard(const String& uid) {
  // 1. 调用Supabase API
  // 2. 获取卡片信息
  // 3. 验证状态
  // 4. 返回验证结果
}
```

##### 3.3 显示卡片信息

**信息展示**:

```
┌─────────────────────────┐
│  Card Detected          │  <- 标题
├─────────────────────────┤
│ Card: ****1234          │  <- 脱敏卡号
│ Type: VIP               │  <- 卡片类型
│ Balance: $76.00         │  <- 当前余额
│                         │
│ 本次消费: $5.00          │  <- 本次金额
│ 洗车后余额: $71.00      │  <- 剩余余额
├─────────────────────────┤
│ Press OK to confirm     │  <- 确认提示
└─────────────────────────┘
```

**操作指引**:


| 语言 | 确认提示                              |
| ---- | ------------------------------------- |
| 英文 | Press OK to confirm payment           |
| 法文 | Appuyez OK pour confirmer le paiement |
| 中文 | 按OK确认支付                          |

**错误处理**:

```
❌ 卡片无效: "Invalid card, please try another card"
❌ 余额不足: "Insufficient balance, please select another package"
❌ 卡片未激活: "Card not activated, please contact support"
```

##### 3.4 支付确认

**功能描述**:

- 显示确认界面
- 支持取消操作
- 执行支付扣款

**用户界面**:

```
┌─────────────────────────┐
│  Confirm Order          │  <- 标题
├─────────────────────────┤
│ Package: Quick Wash    │  <- 套餐名称
│ Time: 5 min             │  <- 时长
│ Price: $5.00            │  <- 价格
│                         │
│ Balance Before: $76.00  │  <- 扣款前余额
│ Balance After:  $71.00  │  <- 扣款后余额
├─────────────────────────┤
│ OK:Confirm  SEL:Cancel  │  <- 操作提示
└─────────────────────────┘
```

**支付流程**:

1. 用户按 **OK** → 执行扣款
2. 调用 Supabase API 更新余额
3. 记录交易历史
4. 进入洗车执行

**取消流程**:

1. 用户按 **SELECT** → 返回套餐选择
2. 不扣除余额
3. 显示取消提示

#### 🔴 步骤4: 洗车执行与系统复位

##### 4.1 执行洗车 (脉冲控制)

**功能描述**:

- 扣除卡片余额
- 发送脉冲信号控制洗车计时器
- 显示洗车进度

**扣款API**:

```javascript
// Supabase 余额更新接口
PATCH /rest/v1/jc_vip_cards?card_uid=eq.{uid}

// 请求体
{
  "card_credit": new_balance
}

// 事务记录
POST /rest/v1/jc_transactions
{
  "machine_id": "TERMINAL_001",
  "card_uid": "2345408116",
  "package_id": 1,
  "amount": 5.00,
  "balance_before": 76.00,
  "balance_after": 71.00,
  "timestamp": "2024-01-28T12:00:00Z"
}
```

**脉冲控制**:

```cpp
// 脉冲参数
#define PULSE_WIDTH_MS      100    // 脉冲宽度: 100ms
#define PULSE_INTERVAL_MS   1000   // 脉冲间隔: 1秒

// 发送脉冲函数
void sendPulses(int count) {
  for (int i = 0; i < count; i++) {
    digitalWrite(PULSE_OUT, HIGH);
    delay(PULSE_WIDTH_MS);
    digitalWrite(PULSE_OUT, LOW);
    delay(PULSE_INTERVAL_MS);
  
    // 每5个脉冲响一声
    if (i % 5 == 0) {
      beepShort();
    }
  }
}
```

**用户界面**:

```
┌─────────────────────────┐
│  Wash in Progress       │  <- 标题
├─────────────────────────┤
│                         │
│       ⏱️ 5:00           │  <- 倒计时
│      3/15 pulses        │  <- 进度 (3/15)
│        20% ████░░░░      │  <- 进度条
│                         │
│   Enjoy Your Wash!      │  <- 祝福语
└─────────────────────────┘
```

**倒计时显示**:

- 根据套餐时长显示倒计时
- 实时更新剩余时间
- 格式: MM:SS

**倒计时文案**:


| 语言 | 进行中提示                           |
| ---- | ------------------------------------ |
| 英文 | Wash in Progress... Enjoy Your Wash! |
| 法文 | Lavage en cours... Bon lavage!       |
| 中文 | 洗车进行中...洗车愉快！              |

##### 4.2 完成提示

**功能描述**:

- 显示洗车完成信息
- 播放成功提示音
- 自动返回待机

**用户界面**:

```
┌─────────────────────────┐
│   Complete!             │  <- 标题
├─────────────────────────┤
│                         │
│         ✓              │  <- 完成图标
│                         │
│   5 min completed       │  <- 完成信息
│                         │
│   Thank you!            │  <- 感谢语
└─────────────────────────┘
```

**完成文案**:


| 语言 | 完成信息                               |
| ---- | -------------------------------------- |
| 英文 | Thank you for using Eaglson Coin Wash! |
| 法文 | Merci d'utiliser Eaglson Coin Wash!    |
| 中文 | 感谢使用壹狗剩自助洗车！               |

**交互反馈**:

- LED 状态: 🟢 绿灯常亮
- 蜂鸣器: 成功提示音（两声短响）
- 持续时间: 5秒

##### 4.3 系统复位

**功能描述**:

- 清空临时数据
- 重置状态机
- 返回待机状态

**复位流程**:

```cpp
void resetSystem() {
  // 清空临时变量
  cardUID = "";
  selectedPackage = 0;
  sentPulses = 0;
  
  // 重置状态机
  currentState = STATE_IDLE;
  stateStartTime = millis();
  
  // 显示待机界面
  displayIdle();
}
```

---

## 3. 功能详细需求

### 3.1 多语言支持

**支持语言**:

1. **English (英文)** - 默认语言
2. **Français (法语)** - 完整翻译
3. **中文** - 简体中文

**文字资源** (示例):


| 场景     | 英文                         | 法文                           | 中文                       |
| -------- | ---------------------------- | ------------------------------ | -------------------------- |
| 欢迎界面 | Welcome to Eaglson Coin Wash | Bienvenue au Lave-Auto Eaglson | 欢迎使用壹狗剩自助洗车服务 |
| 选择语言 | Select Language              | Choisir la langue              | 选择语言                   |
| 选择套餐 | Select Package               | Choisir le forfait             | 选择套餐                   |
| 刷卡提示 | Please tap your card         | Veuillez appuyer votre carte   | 请刷卡                     |
| 处理中   | Processing...                | Traitement en cours...         | 处理中...                  |
| 已完成   | Complete!                    | Terminé!                      | 完成！                     |

### 3.2 NFC 卡片验证

**支持的卡片类型**:

- Mifare Classic (4字节UID)
- Mifare Ultralight (7字节UID)
- ISO14443 Type A/B

**卡片信息结构**:

```cpp
struct CardInfo {
  String cardUID;        // 卡片UID (数据库主键)
  String cardNumber;     // 卡号 (脱敏显示)
  float balance;          // 余额
  bool isActive;          // 激活状态
  bool isValid;           // 有效性
  String userName;        // 用户名
  String userEmail;       // 用户邮箱
  String cardType;        // 卡片类型 (VIP/Standard)
};
```

**脱敏规则**:

- 原始卡号: `CARD-1234567890`
- 脱敏显示: `****7890`

### 3.3 Supabase 集成

**数据库表结构**:

#### jc_vip_cards (会员卡表)

```sql
CREATE TABLE jc_vip_cards (
  card_uid BIGINT PRIMARY KEY,      -- 卡片UID (整数)
  card_number VARCHAR(20),          -- 卡号
  card_credit DECIMAL(10,2),        -- 余额
  is_active BOOLEAN,                -- 激活状态
  user_name VARCHAR(100),           -- 用户名
  user_email VARCHAR(100),         -- 用户邮箱
  card_type VARCHAR(20),            -- 卡片类型
  created_at TIMESTAMP,             -- 创建时间
  updated_at TIMESTAMP              -- 更新时间
);
```

#### jc_transactions (交易记录表)

```sql
CREATE TABLE jc_transactions (
  id SERIAL PRIMARY KEY,
  machine_id VARCHAR(20),            -- 机器ID
  card_uid BIGINT,                  -- 卡片UID
  package_id INTEGER,                -- 套餐ID
  amount DECIMAL(10,2),              -- 金额
  balance_before DECIMAL(10,2),      -- 扣款前余额
  balance_after DECIMAL(10,2),      -- 扣款后余额
  timestamp TIMESTAMP DEFAULT NOW()  -- 交易时间
);
```

**API端点**:

- 查询卡片: `GET /rest/v1/jc_vip_cards?card_uid=eq.{uid}`
- 更新余额: `PATCH /rest/v1/jc_vip_cards?card_uid=eq.{uid}`
- 记录交易: `POST /rest/v1/jc_transactions`

### 3.4 脉冲控制

**脉冲参数**:

- **宽度**: 100ms
- **间隔**: 1000ms (1秒)
- **电压**: 5V (高电平)
- **波形**: 方波

**脉冲与时间对应关系**:

- 1个脉冲 = 1分钟洗车时间
- 5个脉冲 = 5分钟
- 10个脉冲 = 10分钟
- 30个脉冲 = 30分钟

**控制逻辑**:

```cpp
// 发送指定数量的脉冲
void sendPulseSequence(int pulseCount) {
  for (int i = 0; i < pulseCount; i++) {
    // 发送脉冲
    digitalWrite(PULSE_OUT, HIGH);
    delay(100);  // 脉冲宽度
  
    digitalWrite(PULSE_OUT, LOW);
    delay(1000); // 脉冲间隔
  
    // 每5个脉冲反馈
    if (i % 5 == 0) {
      beepShort();
    }
  }
}
```

---

## 4. 技术规格

### 4.1 硬件规格


| 项目     | 规格                   |
| -------- | ---------------------- |
| 主控芯片 | ESP32-S3 (双核240MHz)  |
| Flash    | 8MB                    |
| SRAM     | 512KB                  |
| 显示     | OLED 128×64 (SSD1306) |
| 读卡器   | RC522 NFC (SPI)        |
| 按钮     | 2个 (OK + SELECT)      |
| LED      | RGB 三色               |
| 蜂鸣器   | 有源                   |
| 继电器   | 5V 光耦隔离            |

### 4.2 软件规格


| 项目     | 规格                  |
| -------- | --------------------- |
| 开发环境 | Arduino IDE 2.x       |
| 核心框架 | FreeRTOS              |
| 通信协议 | WiFi + HTTPS          |
| 数据库   | Supabase (PostgreSQL) |
| 编程语言 | C/C++                 |
| 代码行数 | ~800行                |

### 4.3 性能指标


| 指标         | 目标值  |
| ------------ | ------- |
| 系统启动时间 | < 3秒   |
| NFC读取时间  | < 500ms |
| API响应时间  | < 2秒   |
| 状态切换时间 | < 100ms |
| 脉冲发送精度 | ±10ms  |
| 系统稳定性   | > 99%   |

### 4.4 可靠性要求

- **看门狗**: 10秒超时自动重启
- **错误恢复**: 累计5次错误自动重启
- **超时保护**: 所有状态都有超时机制
- **健康检查**: 每10秒检查内存和硬件状态
- **交易记录**: 本地缓存50笔交易

---

## 5. 界面设计

### 5.1 界面布局规范

**OLED 128×64 显示区域**:

- **顶部标题栏**: 0-15像素 (高度14px)
- **内容区域**: 15-64像素 (高度49px)
- **字体大小**: 1号字体 (6×8像素)
- **按钮文字**: 1号字体

### 5.2 状态指示


| 状态   | LED颜色 | 模式 | 说明     |
| ------ | ------- | ---- | -------- |
| 待机   | 🟢 绿色 | 常亮 | 系统就绪 |
| 操作中 | 🔵 蓝色 | 常亮 | 用户操作 |
| 处理中 | 🔵 蓝色 | 闪烁 | 后台处理 |
| 成功   | 🟢 绿色 | 闪烁 | 操作成功 |
| 错误   | 🔴 红色 | 闪烁 | 系统错误 |

### 5.3 声音反馈


| 操作     | 提示音 | 时长  |
| -------- | ------ | ----- |
| 按钮按下 | 短响   | 50ms  |
| 操作成功 | 两短响 | 100ms |
| 操作错误 | 三短响 | 300ms |
| 系统启动 | 两短响 | 200ms |

### 5.4 按钮映射


| 按钮   | GPIO | 功能 | 说明         |
| ------ | ---- | ---- | ------------ |
| OK     | 1    | 确认 | 确认当前选择 |
| SELECT | 2    | 选择 | 切换选项     |

---

## 6. 异常处理

### 6.1 错误类型

#### 网络错误

- WiFi 连接失败
- Supabase API 超时
- 数据库查询失败

**处理方式**:

- 显示错误提示
- 自动重试 (最多3次)
- 降级运行 (离线模式)

#### 卡片错误

- 卡片读取失败
- 卡片不在数据库中
- 卡片未激活
- 余额不足

**处理方式**:

- 显示错误信息
- 返回上一状态
- 记录错误日志

#### 硬件错误

- OLED 初始化失败
- NFC 读卡器故障
- 脉冲发送失败

**处理方式**:

- 进入错误状态
- 闪烁红色LED
- 触发系统重启

### 6.2 错误提示


| 错误类型 | 提示信息             | 操作           |
| -------- | -------------------- | -------------- |
| 余额不足 | Insufficient balance | 返回套餐选择   |
| 卡片无效 | Invalid card         | 返回刷卡界面   |
| 网络错误 | Network error        | 重试或降级运行 |
| 系统错误 | System error         | 自动重启       |

---

## 7. 数据流

### 7.1 数据流总览

```
用户操作
  ↓
本地处理 (ESP32)
  ↓
NFC读取 (RC522)
  ↓
API调用 (Supabase)
  ↓
数据库更新 (PostgreSQL)
  ↓
脉冲发送 (GPIO)
  ↓
洗车设备控制 (继电器)
```

### 7.2 交易数据流

```javascript
// 1. 读取卡片
POST /api/nfc/read
Response: { uid: "2345408116" }

// 2. 查询卡片信息
GET /rest/v1/jc_vip_cards?card_uid=eq.2345408116
Response: {
  card_credit: 76.00,
  is_active: true
}

// 3. 验证余额
if (balance >= price) {
  // 扣款
  PATCH /rest/v1/jc_vip_cards?card_uid=eq.2345408116
  Body: { card_credit: 71.00 }
  
  // 记录交易
  POST /rest/v1/jc_transactions
  Body: {
    machine_id: "TERMINAL_001",
    card_uid: "2345408116",
    package_id: 1,
    amount: 5.00
  }
}
```

---

## 8. 测试需求

### 8.1 功能测试

- [X]  系统启动测试
- [X]  语言选择测试
- [X]  套餐选择测试
- [X]  NFC 卡片读取测试
- [X]  Supabase API 测试
- [X]  支付流程测试
- [X]  脉冲发送测试
- [X]  交易记录测试

### 8.2 压力测试

- [ ]  连续100笔交易
- [ ]  24小时运行测试
- [ ]  内存泄漏测试
- [ ]  并发处理测试

### 8.3 用户体验测试

- [ ]  操作流程顺畅度
- [ ]  界面响应速度
- [ ]  错误提示友好度
- [ ]  多语言准确性

### 8.4 安全性测试

- [ ]  卡片信息加密
- [ ]  网络传输安全
- [ ]  余额计算准确性
- [ ]  防止重复扣款

---

## 9. 附录

### 9.1 术语表

- **NFC**: Near Field Communication (近场通信)
- **RC522**: NFC 读卡器模块
- **ESP32-S3**: ESP32 第三代芯片
- **Supabase**: 开源Firebase替代品
- **OLED**: Organic Light Emitting Diode (有机发光二极管)

### 9.2 参考资料

- ESP32-S3 Datasheet
- RC522 NFC Reader Guide
- Supabase API Documentation
- Arduino IDE Documentation

### 9.3 项目文件结构

```
Eaglson_Coin_Wash/
├── Eaglson_Lite_v2.ino          # 主程序
├── Eaglson_业务需求文档.md       # 本文档
├── docs/
│   ├── 新业务流程说明.md
│   └── API文档.md
└── config/
    ├── 套餐配置.json
    └── Supabase配置.json
```

---

**文档状态**: ✅ 需求确认
**更新日期**: 2024-01-28
**版本**: v1.0
**作者**: System Architect Team

---

## 📞 联系方式

- **项目经理**: Eaglson Team
- **技术支持**: support@eaglson.com
- **文档更新**: 每次重大变更更新此文档
