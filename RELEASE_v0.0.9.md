# GoldSky_Lite v0.0.9 - 系统健康度监测版本

**发布日期**: 2025-12-04
**版本号**: v0.0.9

## 🎯 本次更新目的

解决设备运行2-3天后出现的刷卡无响应问题，通过健康度监测系统自动采集运行数据，帮助诊断根本原因。

## ✨ 新增功能

### 1. 系统健康度监测系统 ⭐ 核心功能

#### 自动数据采集
- **启动基线日志**: 系统启动时立即上传初始化状态，作为对比基准
- **定时运行日志**: 每30分钟自动上传系统健康度数据
- **实时事件记录**: NFC读卡、交易、错误等事件实时记录

#### 监测指标
**系统运行**
- 运行时长（秒/小时）
- 主循环执行时间
- 看门狗重置次数

**内存管理**
- 当前剩余堆内存
- 最小剩余堆内存
- 堆内存使用率

**WiFi 状态**
- 连接状态
- 信号强度（RSSI）
- 重连次数

**NFC 模块状态** 🔍 重点诊断
- 初始化状态
- 固件版本
- 读卡成功/失败次数
- 读卡成功率
- 空闲时长（分钟）
- 最后活跃时间

**OLED/I2C 状态**
- OLED工作状态
- I2C错误次数

**业务统计**
- 总交易数
- 最近1小时交易数
- 最后交易时间

**错误统计**
- 最后错误信息
- 最近30分钟错误次数

#### 串口调试命令
```
health          - 查看详细健康度报告
health upload   - 立即上传健康度日志
help            - 查看所有可用命令
```

### 2. 数据库支持

#### Supabase 表结构
新增 `system_health_logs` 表，包含所有健康度指标字段。

#### RLS 策略
支持禁用RLS或配置细粒度策略。

## 🔧 技术实现

### 新增文件

1. **HealthMonitor.h** (2.5KB)
   - 健康度监测核心模块
   - 自动采集和上传逻辑
   - JSON数据构建
   - 错误处理机制

2. **HEALTH_MONITOR_README.md** (17KB)
   - 完整使用文档
   - 数据库建表脚本
   - SQL查询示例
   - 问题诊断指南

3. **健康度监测系统_快速开始.md** (4KB)
   - 快速部署指南
   - 关键指标说明
   - 典型问题模式

### 修改文件

1. **config.h**
   - 添加 `HealthMetrics` 结构体
   - 完整的健康度指标定义
   - 计算方法（成功率、使用率等）
   - I2C引脚修正为 GPIO 42, 41

2. **GoldSky_Lite.ino**
   - 集成 `HealthMonitor.h`
   - setup() 中初始化健康度监测
   - setup() 中上传基线日志
   - loop() 中定时上传健康度数据
   - performHealthCheck() 增强NFC检测
   - 串口命令新增 `health` 和 `health upload`
   - 交易成功时记录到健康度系统

3. **GoldSky_Utils.ino**
   - readCardUID() 记录NFC读卡成功
   - readCardUID() 记录NFC读卡失败
   - 新增 getStateString() 状态转换函数

## 📊 诊断刷卡无响应问题

### 典型问题模式

#### 模式1: NFC模块挂死
- **症状**: `nfc_initialized` 从 true → false
- **表现**: `nfc_idle_minutes` > 120分钟
- **结果**: 刷卡完全无响应

#### 模式2: 内存泄漏
- **症状**: `free_heap` 持续下降到 < 50KB
- **表现**: 系统逐渐变慢
- **结果**: 最终卡死需要重启

#### 模式3: I2C总线冲突
- **症状**: `i2c_error_count` 持续增加
- **表现**: `nfc_success_rate` 波动，时好时坏
- **结果**: 刷卡响应不稳定

#### 模式4: WiFi重连阻塞
- **症状**: `wifi_reconnect_count` 频繁增加
- **表现**: 主循环被阻塞
- **结果**: 刷卡响应延迟

### SQL诊断查询

#### 对比初始状态和当前状态
```sql
WITH baseline AS (
  SELECT * FROM system_health_logs
  WHERE device_id = '34:85:18:62:44:84'
  ORDER BY timestamp ASC
  LIMIT 1
),
latest AS (
  SELECT * FROM system_health_logs
  WHERE device_id = '34:85:18:62:44:84'
  ORDER BY timestamp DESC
  LIMIT 1
)
SELECT
  '初始状态' as status,
  baseline.uptime_hours,
  baseline.free_heap / 1024 as free_kb,
  baseline.nfc_success_rate,
  baseline.nfc_idle_minutes
FROM baseline
UNION ALL
SELECT
  '当前状态' as status,
  latest.uptime_hours,
  latest.free_heap / 1024 as free_kb,
  latest.nfc_success_rate,
  latest.nfc_idle_minutes
FROM latest;
```

#### 查找NFC异常记录
```sql
SELECT
  timestamp,
  uptime_hours,
  nfc_initialized,
  nfc_success_rate,
  nfc_idle_minutes,
  free_heap / 1024 as free_kb,
  last_error
FROM system_health_logs
WHERE device_id = '34:85:18:62:44:84'
  AND (
    nfc_initialized = false
    OR nfc_success_rate < 80
    OR nfc_idle_minutes > 120
    OR free_heap < 100000
  )
ORDER BY timestamp DESC;
```

## 🐛 Bug修复

1. **I2C引脚配置错误**
   - 修正：GPIO 8, 9 → GPIO 42, 41
   - 位置：config.h

2. **ArduinoJson警告**
   - 修复：StaticJsonDocument → JsonDocument
   - 位置：HealthMonitor.h

3. **Supabase API调用**
   - 修复：使用 ConfigManager 获取密钥
   - 修复：添加 .c_str() 转换
   - 位置：HealthMonitor.h

## 📦 部署指南

### 1. 创建数据库表

在Supabase SQL Editor中执行：

```sql
CREATE TABLE system_health_logs (
  id BIGSERIAL PRIMARY KEY,
  device_id VARCHAR(50) NOT NULL,
  timestamp TIMESTAMP WITH TIME ZONE DEFAULT NOW(),

  uptime_seconds BIGINT,
  uptime_hours DECIMAL(10,2),

  free_heap BIGINT,
  min_free_heap BIGINT,
  heap_usage_percent DECIMAL(5,2),

  wifi_connected BOOLEAN,
  wifi_rssi INTEGER,
  wifi_reconnect_count INTEGER,

  nfc_initialized BOOLEAN,
  nfc_firmware_version VARCHAR(10),
  nfc_last_read_success BOOLEAN,
  nfc_read_success_count INTEGER,
  nfc_read_fail_count INTEGER,
  nfc_success_rate DECIMAL(5,2),
  nfc_idle_minutes INTEGER,

  oled_working BOOLEAN,
  i2c_error_count INTEGER,

  total_transactions INTEGER,
  transactions_last_hour INTEGER,

  current_state VARCHAR(30),
  loop_execution_time_ms INTEGER,
  watchdog_reset_count INTEGER,

  last_error TEXT,
  error_count_last_30min INTEGER
);

CREATE INDEX idx_health_device_time ON system_health_logs(device_id, timestamp DESC);
CREATE INDEX idx_health_timestamp ON system_health_logs(timestamp DESC);
CREATE INDEX idx_health_nfc_status ON system_health_logs(nfc_initialized, nfc_success_rate);

ALTER TABLE system_health_logs DISABLE ROW LEVEL SECURITY;
```

### 2. 编译上传代码

直接编译 GoldSky_Lite.ino 上传到ESP32-S3。

### 3. 验证运行

串口监视器应显示：
```
✅ 初始化日志上传成功
```

在Supabase中查询验证：
```sql
SELECT * FROM system_health_logs
ORDER BY timestamp DESC
LIMIT 1;
```

## 🔄 版本兼容性

- **向后兼容**: ✅ 完全兼容v0.0.8
- **数据库迁移**: 仅需创建新表，不影响现有表
- **配置文件**: 无需修改WiFi和Supabase配置

## 📝 已知问题

无

## 🚀 下一步计划

根据健康度日志分析结果，可能的改进方向：

1. **自动恢复机制**
   - NFC成功率< 50%时自动重置NFC模块
   - 内存< 100KB时自动清理缓存
   - WiFi重连失败时降级为离线模式

2. **预警系统**
   - Supabase Edge Function发送邮件/短信告警
   - 异常指标自动推送通知

3. **可视化仪表板**
   - Grafana/Superset接入Supabase
   - 实时监控设备健康度

## 👥 贡献者

- GoldSky Development Team

## 📄 许可证

[项目许可证]

---

**安装说明**: 参考 [HEALTH_MONITOR_README.md](./HEALTH_MONITOR_README.md)
**快速开始**: 参考 [健康度监测系统_快速开始.md](./健康度监测系统_快速开始.md)
