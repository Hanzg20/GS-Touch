# GoldSky_Lite 系统健康度监测文档

## 📋 概述

系统健康度监测模块用于自动采集和上传设备运行数据，帮助诊断和分析系统问题，特别是**刷卡无响应**等稳定性问题。

## 🎯 解决的问题

您遇到的问题：
- ✅ 设备运行几天后刷卡无响应
- ✅ 设备显示正常但功能失效
- ✅ 需要断电重启才能恢复
- ✅ 无法追踪问题发生的时间和原因

## 🔧 功能特性

### 1. 自动采集数据

#### 启动时上传基线日志 ⭐ 新增
系统启动完成后，会立即上传一条**初始化基线日志**，用于与后续日志对比：
- 记录系统刚启动时的健康状态
- 运行时间为0，内存为最大值
- NFC刚初始化的状态
- **用途**：与运行几天后的日志对比，可以看出哪些指标发生了变化

#### 定时上传运行日志
每30分钟自动采集并上传以下数据到Supabase数据库：

#### 系统运行时间
- 运行时长（秒/小时）
- 帮助判断是否与运行时间相关

#### 内存状态
- 当前剩余堆内存
- 最小剩余堆内存
- 内存使用率
- **诊断目的**：内存泄漏会导致系统异常

#### WiFi 状态
- 连接状态
- 信号强度（RSSI）
- 重连次数
- **诊断目的**：WiFi问题可能影响系统稳定性

#### NFC 模块状态 ⭐ 重点
- 初始化状态
- 固件版本
- 读卡成功次数
- 读卡失败次数
- 读卡成功率
- 最后活跃时间
- 空闲时长（分钟）
- **诊断目的**：直接关联刷卡无响应问题

#### OLED/I2C 状态
- OLED工作状态
- I2C错误次数
- **诊断目的**：I2C总线冲突可能影响NFC

#### 业务统计
- 总交易数
- 最近1小时交易数
- 最后交易时间
- **诊断目的**：交易频率与问题的关联

#### 系统状态
- 当前运行状态
- 主循环执行时间
- 看门狗重置次数
- **诊断目的**：死循环或阻塞问题

#### 错误统计
- 最后错误信息
- 最近30分钟错误次数
- **诊断目的**：错误累积可能导致崩溃

### 2. 实时监测
- NFC读卡成功/失败自动记录
- 交易完成自动记录
- 错误发生自动记录

### 3. 串口命令支持
方便现场调试和问题诊断

## 📊 数据库表结构

```sql
CREATE TABLE system_health_logs (
  id BIGSERIAL PRIMARY KEY,
  device_id VARCHAR(50) NOT NULL,
  timestamp TIMESTAMP WITH TIME ZONE DEFAULT NOW(),

  -- 系统运行时间
  uptime_seconds BIGINT,
  uptime_hours DECIMAL(10,2),

  -- 内存状态
  free_heap BIGINT,
  min_free_heap BIGINT,
  heap_usage_percent DECIMAL(5,2),

  -- WiFi 状态
  wifi_connected BOOLEAN,
  wifi_rssi INTEGER,
  wifi_reconnect_count INTEGER,

  -- NFC 模块状态
  nfc_initialized BOOLEAN,
  nfc_firmware_version VARCHAR(10),
  nfc_last_read_success BOOLEAN,
  nfc_read_success_count INTEGER,
  nfc_read_fail_count INTEGER,
  nfc_success_rate DECIMAL(5,2),
  nfc_last_active_time TIMESTAMP,
  nfc_idle_minutes INTEGER,

  -- OLED/I2C 状态
  oled_working BOOLEAN,
  i2c_error_count INTEGER,

  -- 业务统计
  total_transactions INTEGER,
  transactions_last_hour INTEGER,
  last_transaction_time TIMESTAMP,

  -- 系统状态
  current_state VARCHAR(30),
  loop_execution_time_ms INTEGER,
  watchdog_reset_count INTEGER,

  -- 错误/异常
  last_error TEXT,
  error_count_last_30min INTEGER,

  CONSTRAINT fk_device
    FOREIGN KEY (device_id)
    REFERENCES devices(device_id)
    ON DELETE CASCADE
);

-- 索引
CREATE INDEX idx_health_device_time ON system_health_logs(device_id, timestamp DESC);
CREATE INDEX idx_health_timestamp ON system_health_logs(timestamp DESC);
CREATE INDEX idx_health_nfc_status ON system_health_logs(nfc_initialized, nfc_success_rate);
```

## 🚀 使用方法

### 1. 在Supabase创建数据库表

复制上面的SQL，在Supabase SQL Editor中执行：
1. 登录 https://supabase.com
2. 进入您的项目
3. 点击左侧 "SQL Editor"
4. 粘贴SQL并执行

### 2. 编译上传代码

代码已集成到 GoldSky_Lite.ino，直接编译上传即可。

**启动后会自动上传初始化基线日志** ⭐

串口监视器会显示：
```
🏥 初始化健康度监测系统...
   设备ID: A4:CF:12:34:56:78
   上传间隔: 30 分钟
📊 上传系统初始化基线日志...
   设备ID: A4:CF:12:34:56:78
   运行时长: 0.0 小时
   剩余内存: 298 KB
   NFC成功率: 0.0%
✅ 初始化日志上传成功
```

在Supabase中立即可以查到第一条基线记录：

```sql
SELECT * FROM system_health_logs
WHERE device_id = '您的设备MAC地址'
ORDER BY timestamp ASC
LIMIT 1;
```

这条基线记录用于与后续日志对比，查看系统状态变化。

### 3. 串口命令

连接串口监视器（115200波特率），可用以下命令：

```
health          - 查看当前健康度状态
health upload   - 立即上传健康度日志
help            - 查看所有可用命令
```

#### 示例输出：

```
╔════════════════════════════════════════════════╗
║           系统健康度状态报告                   ║
╚════════════════════════════════════════════════╝
🔹 设备ID: A4:CF:12:34:56:78
🔹 运行时长: 48.32 小时

📱 内存状态:
   剩余堆内存: 256 KB
   最小堆内存: 245 KB
   堆使用率: 15.2%

📡 WiFi 状态:
   连接状态: 已连接
   信号强度: -45 dBm
   重连次数: 2

💳 NFC 状态:
   初始化状态: 正常
   固件版本: 0x92
   成功读卡: 156 次
   失败读卡: 3 次
   成功率: 98.1%
   空闲时长: 125 分钟  ⚠️ 注意：长时间空闲

📺 OLED 状态:
   工作状态: 正常
   I2C错误: 0 次

💰 业务统计:
   总交易数: 45
   最近1小时: 0 笔

⚙️ 系统状态:
   当前状态: WELCOME
   循环耗时: 12 ms

⚠️ 错误统计:
   最后错误: 无
   最近30分钟: 0 次错误
════════════════════════════════════════════════
```

## 🔍 如何分析问题

### 问题场景：刷卡无响应

#### 步骤1：查看历史数据
在Supabase中查询问题发生前后的健康度日志：

```sql
SELECT
  timestamp,
  uptime_hours,
  nfc_initialized,
  nfc_success_rate,
  nfc_idle_minutes,
  free_heap / 1024 as free_heap_kb,
  current_state,
  last_error
FROM system_health_logs
WHERE device_id = '您的设备ID'
ORDER BY timestamp DESC
LIMIT 100;
```

#### 步骤2：关注关键指标

##### 1. NFC空闲时间（nfc_idle_minutes）
- **正常值**：< 60分钟
- **异常值**：> 120分钟（2小时）
- **分析**：如果刷卡无响应前，NFC空闲时间异常长，可能是NFC模块进入低功耗或挂死状态

##### 2. NFC成功率（nfc_success_rate）
- **正常值**：> 95%
- **异常值**：< 80%
- **分析**：成功率持续下降表明NFC模块通信异常

##### 3. 内存使用（free_heap）
- **正常值**：> 200KB
- **警告值**：< 100KB
- **危险值**：< 50KB
- **分析**：内存持续下降可能导致系统崩溃

##### 4. NFC初始化状态（nfc_initialized）
- **正常值**：true
- **异常值**：false
- **分析**：从true变为false表示NFC模块失联

##### 5. I2C错误计数（i2c_error_count）
- **正常值**：0
- **异常值**：> 10
- **分析**：I2C错误可能影响NFC（如果NFC和OLED共享I2C总线）

#### 步骤3：时间关联分析

```sql
-- 查看运行时间与问题的关联
SELECT
  ROUND(uptime_hours::numeric, 0) as runtime_hours,
  AVG(nfc_success_rate) as avg_nfc_success_rate,
  AVG(free_heap / 1024) as avg_free_heap_kb,
  COUNT(*) as sample_count
FROM system_health_logs
WHERE device_id = '您的设备ID'
GROUP BY ROUND(uptime_hours::numeric, 0)
ORDER BY runtime_hours;
```

**分析结果示例**：
- 如果运行24小时后，NFC成功率从98%降到60%，则问题与长时间运行相关
- 如果内存在48小时后降到50KB以下，则存在内存泄漏

### 常见问题模式

#### 模式1：内存泄漏型
- **症状**：free_heap持续下降，最终< 50KB
- **表现**：运行2-3天后系统缓慢或卡死
- **解决**：检查代码中String拼接、动态内存分配

#### 模式2：NFC模块挂死型
- **症状**：nfc_initialized突然变false，nfc_success_rate=0%
- **表现**：刷卡完全无响应
- **解决**：添加NFC模块周期性重置，检查硬件连接

#### 模式3：I2C总线冲突型
- **症状**：i2c_error_count持续增加，NFC间歇性失败
- **表现**：刷卡时好时坏
- **解决**：调整I2C时钟，增加延时，检查上拉电阻

#### 模式4：WiFi断连影响型
- **症状**：wifi_reconnect_count频繁增加，系统卡在重连
- **表现**：刷卡响应延迟或无响应
- **解决**：优化WiFi重连逻辑，降低重连优先级

## 📈 查询示例

### 1. 对比初始化基线与当前状态 ⭐ 推荐

这个查询可以看出系统从启动到现在发生了什么变化：

```sql
WITH baseline AS (
  SELECT * FROM system_health_logs
  WHERE device_id = 'YOUR_DEVICE_ID'
  ORDER BY timestamp ASC
  LIMIT 1
),
latest AS (
  SELECT * FROM system_health_logs
  WHERE device_id = 'YOUR_DEVICE_ID'
  ORDER BY timestamp DESC
  LIMIT 1
)
SELECT
  '初始状态' as status,
  baseline.timestamp,
  baseline.uptime_hours,
  baseline.free_heap / 1024 as free_kb,
  baseline.nfc_success_rate,
  baseline.total_transactions
FROM baseline
UNION ALL
SELECT
  '当前状态' as status,
  latest.timestamp,
  latest.uptime_hours,
  latest.free_heap / 1024 as free_kb,
  latest.nfc_success_rate,
  latest.total_transactions
FROM latest;
```

**示例输出**：
| status | timestamp | uptime_hours | free_kb | nfc_success_rate | total_transactions |
|--------|-----------|--------------|---------|------------------|-------------------|
| 初始状态 | 2025-12-04 10:00 | 0.00 | 298 | 0.0 | 0 |
| 当前状态 | 2025-12-06 18:30 | 56.50 | 156 | 94.5 | 127 |

**分析**：
- 内存从 298KB 降到 156KB（下降142KB）- ⚠️ 可能存在内存泄漏
- NFC成功率 94.5% - ✅ 正常
- 运行56.5小时，127笔交易 - ✅ 正常工作

### 2. 查看最近24小时趋势
```sql
SELECT
  timestamp,
  uptime_hours,
  free_heap / 1024 as free_kb,
  nfc_success_rate,
  nfc_idle_minutes,
  total_transactions
FROM system_health_logs
WHERE device_id = 'YOUR_DEVICE_ID'
  AND timestamp > NOW() - INTERVAL '24 hours'
ORDER BY timestamp DESC;
```

### 2. 查找NFC异常时刻
```sql
SELECT
  timestamp,
  uptime_hours,
  nfc_initialized,
  nfc_success_rate,
  nfc_read_fail_count,
  last_error
FROM system_health_logs
WHERE device_id = 'YOUR_DEVICE_ID'
  AND (nfc_initialized = false OR nfc_success_rate < 80)
ORDER BY timestamp DESC;
```

### 3. 内存告警
```sql
SELECT
  timestamp,
  uptime_hours,
  free_heap / 1024 as free_kb,
  min_free_heap / 1024 as min_free_kb,
  current_state
FROM system_health_logs
WHERE device_id = 'YOUR_DEVICE_ID'
  AND free_heap < 100000
ORDER BY timestamp DESC;
```

## ⚙️ 配置选项

在 `HealthMonitor.h` 中可调整上传间隔：

```cpp
// 默认30分钟上传一次
#define HEALTH_LOG_INTERVAL 1800000  // 30分钟 (毫秒)

// 测试时可改为5分钟
// #define HEALTH_LOG_INTERVAL 300000   // 5分钟 (测试用)
```

## 🎯 下一步改进建议

基于健康度日志分析，可以添加：

1. **自动恢复机制**
   - NFC成功率< 50%时自动重置NFC模块
   - 内存< 100KB时自动清理缓存

2. **预警推送**
   - Supabase Edge Function发送邮件/短信告警
   - 异常指标自动通知

3. **可视化仪表板**
   - Grafana/Superset接入Supabase
   - 实时监控设备健康度

4. **智能诊断**
   - AI分析历史数据，预测故障
   - 自动生成诊断报告

## 📝 版本历史

- **v1.0** (2025-12-04)
  - 初始版本
  - 支持30分钟自动上传
  - 串口命令支持
  - NFC/WiFi/内存监测

---

**作者**: GoldSky Development Team
**日期**: 2025-12-04
**用途**: 诊断刷卡无响应问题
