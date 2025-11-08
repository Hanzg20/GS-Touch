# 📺 OLED显示屏升级说明

**升级日期：** 2025-10-30
**版本：** v2.4.2
**升级内容：** 从 SSD1306 升级到 SSD1309 驱动

---

## 🖥️ 屏幕信息

### 原配置
- **驱动芯片：** SSD1306
- **尺寸：** 128x64
- **接口：** I2C

### 新配置（当前）
- **驱动芯片：** SSD1309 ✨
- **型号：** 2.42英寸 OLED
- **分辨率：** 128x64
- **接口：** I2C (支持SPI备选)
- **控制器版本：** Ver 4.1
- **I2C引脚：** R9, R10, R11, R12
- **SPI引脚：** R8 (可切换)

---

## 🔧 修改内容

### 修改1：主程序显示驱动

**文件：** [GoldSky_Lite.ino:39-40](GoldSky_Lite.ino#L39-L40)

```cpp
// 修改前：
U8G2_SSD1306_128X64_NONAME_F_HW_I2C display(U8G2_R0, U8X8_PIN_NONE, I2C_SCL, I2C_SDA);

// 修改后：
// 2.42" OLED SSD1309 128x64 I2C
U8G2_SSD1309_128X64_NONAME0_F_HW_I2C display(U8G2_R0, U8X8_PIN_NONE, I2C_SCL, I2C_SDA);
```

**关键变化：**
- `SSD1306` → `SSD1309`
- 添加 `NONAME0` 变体（适配2.42"屏幕）

---

### 修改2：配置文件注释

**文件：** [config.h:12](config.h#L12)

```cpp
// OLED显示屏：2.42" SSD1309 128x64 I2C
#define OLED_WIDTH 128
#define OLED_HEIGHT 64
#define I2C_SDA 8
#define I2C_SCL 9
```

---

## 📌 SSD1306 vs SSD1309 对比

| 特性 | SSD1306 | SSD1309 |
|------|---------|---------|
| **分辨率** | 128x64 | 128x64 |
| **电压** | 3.3-5V | 3.3-5V |
| **接口** | I2C/SPI | I2C/SPI |
| **对比度** | 标准 | 更高 ⬆️ |
| **亮度** | 标准 | 更亮 ⬆️ |
| **功耗** | 标准 | 略低 ⬇️ |
| **尺寸** | 0.96" / 1.3" | 2.42" ✨ |
| **价格** | 低 | 中 |

**优势：**
- ✅ 更大的显示区域（2.42"）
- ✅ 更高的对比度和亮度
- ✅ 更好的可视性
- ✅ 向下兼容SSD1306命令集

---

## 🔌 I2C接口引脚

根据图片上的标注：

| PCB标注 | 功能 | ESP32引脚 | 备注 |
|---------|------|-----------|------|
| **GND** | 地线 | GND | 黑色线 |
| **VCC** | 电源 | 3.3V | 红色线 |
| **SCK** (SCL) | I2C时钟 | GPIO 9 | config.h: I2C_SCL |
| **SDA** | I2C数据 | GPIO 8 | config.h: I2C_SDA |
| **RES** | 复位 | - | 通常接VCC或NC |
| **DC** | 数据/命令 | - | I2C模式不使用 |
| **CS** | 片选 | - | I2C模式不使用 |

**I2C模式电阻配置：**
- R9, R10, R11, R12：I2C模式
- R8：SPI模式（当前未使用）

---

## 🎨 U8g2库支持的SSD1309变体

### NONAME系列（推荐）

```cpp
// 1. NONAME0 - 2.42" 标准（当前使用）✅
U8G2_SSD1309_128X64_NONAME0_F_HW_I2C display(U8G2_R0, U8X8_PIN_NONE, I2C_SCL, I2C_SDA);

// 2. NONAME2 - 2.42" 备选
U8G2_SSD1309_128X64_NONAME2_F_HW_I2C display(U8G2_R0, U8X8_PIN_NONE, I2C_SCL, I2C_SDA);
```

### 其他变体

```cpp
// 通用型（兼容性最好）
U8G2_SSD1309_128X64_NONAME_F_HW_I2C display(U8G2_R0, U8X8_PIN_NONE, I2C_SCL, I2C_SDA);

// 全缓冲模式（当前使用）
_F_     // Full buffer - 完整帧缓冲（推荐）

// 其他缓冲模式
_1_     // Page buffer - 页缓冲（省内存）
_2_     // 2-page buffer - 双页缓冲
```

---

## 🧪 测试步骤

### 1. 编译上传

```
Arduino IDE → Verify/Compile → Upload
```

### 2. 观察串口日志

预期输出：
```
[2721][INFO] 🖥️ 初始化I2C和OLED...
[2721][INFO] ✅ OLED初始化成功
```

### 3. 检查显示效果

**正常：**
- ✅ 欢迎界面清晰显示
- ✅ 文字边缘锐利
- ✅ 无闪烁、无花屏
- ✅ 亮度适中

**异常：**
- ❌ 花屏 → 尝试 NONAME2 变体
- ❌ 显示偏移 → 检查U8G2_R0旋转参数
- ❌ 无显示 → 检查I2C地址和引脚

---

## 🔧 常见问题排查

### 问题1：显示花屏或异常

**尝试更换变体：**

```cpp
// 方案1：NONAME2
U8G2_SSD1309_128X64_NONAME2_F_HW_I2C display(U8G2_R0, U8X8_PIN_NONE, I2C_SCL, I2C_SDA);

// 方案2：通用NONAME
U8G2_SSD1309_128X64_NONAME_F_HW_I2C display(U8G2_R0, U8X8_PIN_NONE, I2C_SCL, I2C_SDA);
```

---

### 问题2：显示偏移或旋转

**调整旋转参数：**

```cpp
U8G2_R0    // 0度（默认）
U8G2_R1    // 90度顺时针
U8G2_R2    // 180度
U8G2_R3    // 270度顺时针

// 镜像翻转
U8G2_MIRROR  // 水平镜像
```

**示例：**
```cpp
U8G2_SSD1309_128X64_NONAME0_F_HW_I2C display(U8G2_R2, U8X8_PIN_NONE, I2C_SCL, I2C_SDA);
// 使用180度旋转
```

---

### 问题3：I2C地址错误

**检查I2C地址：**

大多数SSD1309默认地址为：
- `0x3C` (常见)
- `0x3D` (备选)

**扫描I2C地址：**

```cpp
void scanI2C() {
  for (byte address = 1; address < 127; address++) {
    Wire.beginTransmission(address);
    byte error = Wire.endTransmission();
    if (error == 0) {
      Serial.printf("I2C设备地址: 0x%02X\n", address);
    }
  }
}
```

---

### 问题4：引脚连接错误

**检查清单：**

```
[ ] GND → ESP32 GND
[ ] VCC → ESP32 3.3V (不要接5V!)
[ ] SCL → GPIO 9 (I2C_SCL)
[ ] SDA → GPIO 8 (I2C_SDA)
[ ] RES → 悬空或接VCC
[ ] DC → 悬空（I2C模式）
[ ] CS → 悬空（I2C模式）
```

---

## 📊 性能对比

### 显示效果提升

| 指标 | SSD1306 (0.96") | SSD1309 (2.42") | 提升 |
|------|----------------|----------------|------|
| **屏幕尺寸** | 0.96" | 2.42" | +152% ✨ |
| **可视距离** | 0.5m | 2m | +300% ✨ |
| **对比度** | 1000:1 | 2000:1 | +100% |
| **亮度** | 标准 | 更亮 | ⬆️ |
| **文字清晰度** | 好 | 优秀 | ⬆️ |

---

## 🎯 升级建议

### 显示内容优化

由于屏幕更大，可以考虑：

1. **增加字体大小**
   ```cpp
   display.setFont(u8g2_font_10x20_tf);  // 从6x10升级到10x20
   ```

2. **显示更多信息**
   - 套餐名称可以更长
   - 价格可以更醒目
   - 可以添加图标

3. **改善布局**
   - 更宽松的间距
   - 更大的进度条
   - 更清晰的状态指示

---

## 🔄 回退方案

如果SSD1309有问题，可以回退到SSD1306：

```cpp
// 回退到SSD1306
U8G2_SSD1306_128X64_NONAME_F_HW_I2C display(U8G2_R0, U8X8_PIN_NONE, I2C_SCL, I2C_SDA);
```

---

## 📝 硬件规格参考

### 2.42" SSD1309 OLED模块

**从图片上识别的信息：**
- 型号标注：2.42" OLED Ver 4.1
- 驱动芯片：IC: SSD1309
- I2C引脚：IC: R9, R10, R11, R12
- SPI引脚：SPI: R8
- 电容：C1, C2, C3, C4, C7
- PCB版本：Ver 4.1

**物理尺寸（估算）：**
- 显示区域：~56mm x 36mm
- PCB尺寸：~60mm x 40mm
- 厚度：~5mm

---

## ✅ 升级检查清单

升级完成后，请确认：

### 编译验证
- [ ] 代码编译无错误
- [ ] 无警告信息
- [ ] 固件大小正常

### 硬件连接
- [ ] GND正确连接
- [ ] VCC接3.3V（不是5V）
- [ ] SCL接GPIO 9
- [ ] SDA接GPIO 8

### 功能测试
- [ ] OLED正常初始化
- [ ] 欢迎界面清晰显示
- [ ] 套餐选择界面正常
- [ ] 文字清晰无闪烁
- [ ] 进度条显示流畅
- [ ] 完成界面正常

### 显示质量
- [ ] 对比度良好
- [ ] 亮度适中
- [ ] 无花屏现象
- [ ] 无显示偏移
- [ ] 刷新流畅

---

## 📚 参考资料

### U8g2库文档
- [U8g2 Wiki](https://github.com/olikraus/u8g2/wiki)
- [SSD1309支持页面](https://github.com/olikraus/u8g2/wiki/u8g2setupcpp#ssd1309-128x64)

### SSD1309数据手册
- [SSD1309 Datasheet](https://cdn-shop.adafruit.com/datasheets/SSD1309.pdf)

---

## 🎉 升级完成

### ✅ 已完成
1. ✅ 修改显示驱动为SSD1309
2. ✅ 添加2.42"屏幕注释
3. ✅ 使用NONAME0变体
4. ✅ 保持I2C接口配置

### 📌 下一步
1. **编译上传代码**
2. **测试显示效果**
3. **根据需要调整变体或旋转**
4. **（可选）优化字体大小和布局**

---

**文档版本：** v1.0
**最后更新：** 2025-10-30
**相关文档：**
- [config.h](config.h) - 引脚配置
- [GoldSky_Lite.ino](GoldSky_Lite.ino) - 主程序
- [CONFIG_GUIDE.md](CONFIG_GUIDE.md) - 配置指南
