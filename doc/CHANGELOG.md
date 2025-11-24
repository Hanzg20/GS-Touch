# GoldSky_Lite 更新日志

## [v0.0.7] - 2025-11-24

### 商用日志系统
- **5级日志**: NONE/ERROR/WARN/INFO/DEBUG/VERBOSE
- **敏感数据脱敏**: 卡号显示为 `23****16`
- **串口命令**: 运行时切换日志级别，无需重新编译
- **日志精简**: 启动日志减少59%，交易日志减少81%

### 套餐配置更新
- 套餐2: $5/5min → $6/7min
- 套餐3: $8/8min → $8/9min
- 套餐4: $15/15min → $12/14min

### UI优化
- **齿轮页标题**: "Wash in Progress" → "Starting Wash"
- **支付页文本**: 精简42字符
- **刷卡页提示**: "TAP CARD" → "Tap to Pay"
- **完成页标题**: "COMPLETE" → "Enjoy Wash!"
- **NFC动画**: 横向椭圆 + 水平波纹线

### 技术改进
- 日志级别从 #define 改为全局变量，支持运行时修改
- 串口命令: log info/debug/warn, log status, cache, help

---

## [v0.5] - 2025-11-08

### 新功能
- **离线交易队列**: 支持缓存50笔交易，WiFi恢复后自动同步
- **配置管理系统**: WiFi凭证和API密钥存储到NVS

### UI优化
- NFC感应动画（专业图标设计）
- 欢迎页滚动广告（+25%速度）
- 齿轮动画简化
- VIP INFO横向布局
- 完成页显示剩余余额

### Bug修复
- 完成页面显示可靠性从25%提升到100%
- ArduinoJson v7兼容性修复
- API响应验证

---

## [v2.4] - 2025-11-07

### 基础功能
- 洗车控制系统
- NFC卡片读取
- Supabase数据库集成
- OLED显示界面
- LED状态指示
- 蜂鸣器反馈

---

## 维护者

- 开发者: Hanzg20
- 仓库: https://github.com/Hanzg20/GS-Touch
