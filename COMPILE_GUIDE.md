# GoldSky_Lite v2.5 编译指南

版本: v2.5
目标板: ESP32-S3

---

## 🔧 Arduino IDE 配置

### 1. 安装ESP32开发板支持

**Arduino IDE 1.8.x**:
1. 打开 `文件` → `首选项`
2. 在 "附加开发板管理器网址" 添加:
   ```
   https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
   ```
3. 打开 `工具` → `开发板` → `开发板管理器`
4. 搜索 "esp32"
5. 安装 "esp32 by Espressif Systems" (建议版本 2.0.11+)

**Arduino IDE 2.x**:
1. 左侧 "BOARD MANAGER"
2. 搜索 "esp32"
3. 安装 "esp32 by Espressif Systems"

---

### 2. 选择正确的开发板

`工具` → `开发板` → `ESP32 Arduino` → 选择以下之一:
- **推荐**: `ESP32S3 Dev Module`
- 或: `ESP32-S3-DevKitC-1`

---

### 3. 配置开发板参数

重要配置项:

| 参数 | 值 | 说明 |
|------|-----|------|
| USB CDC On Boot | Enabled | 允许USB串口 |
| CPU Frequency | 240MHz | 最高性能 |
| Flash Mode | QIO | 快速模式 |
| Flash Size | 4MB (32Mb) | 默认 |
| Partition Scheme | **Default 4MB with spiffs** | 必须! |
| PSRAM | Disabled | 除非你的板有PSRAM |
| Upload Speed | 921600 | 快速上传 |
| Arduino Runs On | Core 1 | 默认 |
| Events Run On | Core 1 | 默认 |

**关键**: 必须选择 "Default 4MB with spiffs" 分区方案，否则NVS可能无法工作。

---

### 4. 安装必需库

`工具` → `管理库` → 搜索并安装:

1. **ArduinoJson** (by Benoit Blanchon)
   - 版本: 6.21.0 或更高
   - 用于: JSON解析

2. **U8g2** (by oliver)
   - 版本: 2.35.0 或更高
   - 用于: OLED显示

3. **MFRC522** (by GithubCommunity)
   - 版本: 1.4.10 或更高
   - 用于: NFC读卡

**注意**: WiFi, HTTPClient, SPI, Wire, Preferences, esp_task_wdt 是ESP32核心库，无需额外安装。

---

## ⚠️ 常见编译错误

### 错误1: `SPI.h: No such file or directory`

**原因**: 未选择ESP32开发板

**解决方案**:
1. 确认已安装ESP32开发板支持
2. `工具` → `开发板` → 选择 `ESP32S3 Dev Module`
3. 重启Arduino IDE

---

### 错误2: `WiFi.h: No such file or directory`

**原因**: 同上，未选择ESP32开发板

**解决方案**: 同错误1

---

### 错误3: `U8g2lib.h: No such file or directory`

**原因**: 未安装U8g2库

**解决方案**:
1. `工具` → `管理库`
2. 搜索 "U8g2"
3. 安装 "U8g2 by oliver"

---

### 错误4: `MFRC522.h: No such file or directory`

**原因**: 未安装MFRC522库

**解决方案**:
1. `工具` → `管理库`
2. 搜索 "MFRC522"
3. 安装 "MFRC522 by GithubCommunity"

---

### 错误5: `ArduinoJson.h: No such file or directory`

**原因**: 未安装ArduinoJson库

**解决方案**:
1. `工具` → `管理库`
2. 搜索 "ArduinoJson"
3. 安装 "ArduinoJson by Benoit Blanchon" (版本6.x)

---

### 错误6: `'JsonDocument' was not declared in this scope`

**原因**: ArduinoJson版本过低 (v5.x)

**解决方案**:
1. 卸载旧版本
2. 安装 ArduinoJson v6.21.0+

---

### 错误7: 内存不足 / Sketch too large

**原因**: 分区方案不正确

**解决方案**:
1. `工具` → `Partition Scheme`
2. 选择 "Default 4MB with spiffs" 或 "Huge APP (3MB No OTA)"

---

## 📦 编译前检查清单

在点击 "上传" 之前，请确认:

- [ ] 开发板选择: `ESP32S3 Dev Module`
- [ ] 分区方案: `Default 4MB with spiffs`
- [ ] USB CDC On Boot: `Enabled`
- [ ] 已安装 ArduinoJson 6.x
- [ ] 已安装 U8g2
- [ ] 已安装 MFRC522
- [ ] 串口端口已选择正确

---

## 🚀 编译和上传

### 方法1: Arduino IDE (推荐)

1. 打开 `GoldSky_Lite.ino`
2. Arduino IDE会自动加载所有 `.ino` 文件
3. 点击 `验证` (✓) 检查编译
4. 点击 `上传` (→) 上传到设备

### 方法2: 命令行 (高级)

```bash
# 使用arduino-cli
arduino-cli compile --fqbn esp32:esp32:esp32s3 GoldSky_Lite.ino
arduino-cli upload -p COM3 --fqbn esp32:esp32:esp32s3 GoldSky_Lite.ino
```

---

## 📊 预期编译输出

**成功编译**:
```
Sketch uses 946280 bytes (48%) of program storage space.
Global variables use 53624 bytes (16%) of dynamic memory.
```

如果超过80%，考虑:
- 优化代码
- 减少日志输出
- 使用更大的分区方案

---

## 🔍 串口监视器配置

上传后打开串口监视器:
1. `工具` → `串口监视器`
2. 波特率: **115200**
3. 换行符: `Both NL & CR` 或 `Newline`

**预期输出**:
```
🚗 Eaglson Coin Wash Terminal v2.5
Fixed Edition - 模块化重构版 + 安全配置
🎯 5步流程 + 4LED + VIP查询 + 离线队列
=====================================
💾 初始化NVS存储...
⚙️ 首次启动,保存默认配置...
✅ 配置已加载:
   WiFi SSID: xxx
```

---

## 💾 NVS分区说明

v2.5使用NVS (Non-Volatile Storage) 存储:
- WiFi配置
- API密钥
- 离线交易队列

**默认分区大小**: ~15KB (足够)

**查看NVS内容** (可选):
```bash
# 使用ESP-IDF工具
esptool.py --port COM3 read_flash 0x9000 0x6000 nvs.bin
python nvs_partition_parser.py nvs.bin
```

---

## 🐛 调试技巧

### 1. 启用详细编译输出
`文件` → `首选项` → 勾选 "编译时显示详细输出"

### 2. 查看完整错误信息
编译失败时，滚动到输出窗口顶部查看第一个错误

### 3. 清理缓存
```
Windows: C:\Users\<用户名>\AppData\Local\Temp\arduino_build_*
macOS: /tmp/arduino_build_*
Linux: /tmp/arduino_build_*
```
删除后重新编译

### 4. 重装库
如果库损坏:
1. 关闭Arduino IDE
2. 删除库文件夹 (Documents/Arduino/libraries/<库名>)
3. 重新打开IDE
4. 重新安装库

---

## 📞 技术支持

如果遇到无法解决的编译问题:

1. **检查Arduino IDE版本**
   - 推荐: 1.8.19 或 2.2.1+

2. **检查ESP32板包版本**
   - 推荐: 2.0.11 - 2.0.14
   - 避免: 3.x (可能不兼容)

3. **提供完整错误信息**
   - 编译输出的前20行
   - IDE版本
   - 开发板选择截图

---

**文档版本**: v1.0
**最后更新**: 2025-11-07
**适用版本**: GoldSky_Lite v2.5
