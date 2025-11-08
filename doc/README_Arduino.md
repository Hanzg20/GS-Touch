# Arduino IDE 使用指南

## 📁 文件结构（符合 Arduino 规范）

```
GoldSky_Lite/              ← 文件夹名称
├── GoldSky_Lite.ino       ← 主文件（必须与文件夹同名）
└── 文档文件（可选）
```

**重要**: Arduino IDE 要求 `文件夹名称` = `主文件名（不含.ino）`

## ✅ 当前正确配置

- ✅ 文件夹名称: `GoldSky_Lite`
- ✅ 主文件名称: `GoldSky_Lite.ino`
- ✅ 完全符合 Arduino IDE 规范

## 🚀 快速开始

### 1. 打开项目

1. 启动 Arduino IDE 2.x
2. 菜单: `文件 → 打开`
3. 选择: `GoldSky_Lite/GoldSky_Lite.ino`

### 2. 选择开发板

1. 工具 → 开发板 → ESP32-S3 Dev Module
2. 端口 → 选择对应的 COM 端口

### 3. 安装依赖库

需要安装以下库（通过库管理器）:
- Adafruit SSD1306
- Adafruit GFX Library
- MFRC522

### 4. 编译和上传

1. 点击 "验证" 检查代码
2. 点击 "上传" 烧录固件
3. 打开串口监视器 (115200 baud)

## 📋 功能特性

- ✅ 完整的业务流程
- ✅ 多语言支持（英/法/中）
- ✅ NFC 刷卡
- ✅ 实时倒计时
- ✅ 超时保护
- ✅ 看门狗定时器
- ✅ 交易记录

## 🔧 硬件连接

参考 `Eaglson_业务需求文档.md` 硬件配置部分

## ⚠️ 注意事项

### 文件命名规则

Arduino IDE 有严格的命名规则：

```cpp
// ❌ 错误：文件夹和文件名称不一致
Eaglson_Lite/
└── GoldSky_Lite.ino  // Arduino IDE 无法识别

// ✅ 正确：文件夹和文件名称一致
GoldSky_Lite/
└── GoldSky_Lite.ino  // Arduino IDE 正常识别
```

### 其他文件说明

项目包含以下文件（不影响编译）：

- `GoldSky_Lite.ino` - **主程序文件（必须）**
- `GoldSky_Lite_v2.ino` - 历史版本（可删除）
- `Eaglson_Lite_Final.ino` - 备份版本（可删除）
- `*.md` - 文档文件（可选）

## 🐛 常见问题

### Q: 为什么 Arduino IDE 找不到主文件？

A: 确保：
1. 文件夹名称 = 文件名称（不含扩展名）
2. `.ino` 文件在文件夹根目录
3. 主文件名为 `GoldSky_Lite.ino`

### Q: 编译错误怎么办？

A: 检查：
1. 开发板选择正确（ESP32S3 Dev Module）
2. 所有依赖库已安装
3. 串口未被其他程序占用

### Q: 上传失败？

A: 尝试：
1. 按住 BOOT 按钮再连接 USB
2. 检查 USB 线是否支持数据传输
3. 重新安装 ESP32 开发板支持

---

**最后更新**: 2024-01-28  
**兼容性**: Arduino IDE 2.x + ESP32-S3 Core
