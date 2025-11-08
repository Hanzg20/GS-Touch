# LED 共阳极配置 - 修改总结

修改日期：2025-11-02
硬件要求：共阳极LED（VCC接LED阳极，GPIO通过电阻接LED阴极）

---

## ✅ 修改内容

### 1. 配置文件添加极性选项

**[config.h:41-44](config.h#L41-L44)**

```cpp
// LED极性配置
// true = 共阳极 (HIGH=熄灭, LOW=点亮)
// false = 共阴极 (HIGH=点亮, LOW=熄灭)
#define LED_COMMON_ANODE true    // ← 共阳极模式
```

---

### 2. LED控制函数自动极性转换

**[GoldSky_Utils.ino:43-50](GoldSky_Utils.ino#L43-L50)**

新增辅助函数：
```cpp
inline void ledWrite(int pin, bool state) {
  #if LED_COMMON_ANODE
    digitalWrite(pin, !state);  // 共阳极: 反相输出
  #else
    digitalWrite(pin, state);   // 共阴极: 直接输出
  #endif
}
```

**修改的函数**：
- `updateSingleLED()` - 所有 `digitalWrite()` 改为 `ledWrite()`
- `updateLEDIndicators()` - LED_POWER控制改为 `ledWrite()`

---

## 🔌 硬件接线（共阳极）

```
ESP32-S3              LED                电阻
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
                       ┌── VCC (+3.3V)
                       │
GPIO 4 (POWER) ────────┤
                       │
                      LED
                       │
                    [220Ω]
                       │
                      GND

其他LED (GPIO 5, 6, 7) 同样接法
```

**关键**:
- LED阳极 → VCC
- LED阴极 → 电阻 → GPIO
- GPIO输出 **LOW** 时LED **亮**
- GPIO输出 **HIGH** 时LED **灭**

---

## 🎯 工作原理

### 极性自动转换

| 代码逻辑 | 共阴极 | 共阳极（反相） | LED状态 |
|---------|-------|--------------|--------|
| `LED_ON` → `ledWrite(pin, HIGH)` | HIGH | **LOW** | 亮 |
| `LED_OFF` → `ledWrite(pin, LOW)` | LOW | **HIGH** | 灭 |

**优势**:
- ✅ 上层代码逻辑完全不变
- ✅ 只需修改一个配置项即可切换极性
- ✅ 编译时自动优化（使用`#if`预处理）

---

## 🧪 测试验证

### 快速测试

上传代码后，观察LED：

| LED | 预期状态 | GPIO电压（共阳极） |
|-----|---------|------------------|
| **POWER** | 常亮 | 0V (LOW) |
| **NETWORK** | WiFi连接后常亮 | 0V (LOW) |
| **PROGRESS** | 选择套餐后常亮 | 0V (LOW) |
| **STATUS** | 刷卡时快闪（100ms） | 0V/3.3V 交替 |

### 万用表验证

测量GPIO引脚：
```
LED亮时: 0V   (共阳极特征)
LED灭时: 3.3V (共阳极特征)

如果相反，说明配置错误或硬件是共阴极
```

---

## ⚠️ 常见问题

### Q: 所有LED常亮，无法控制

**A**: LED_COMMON_ANODE配置错误，硬件是共阳极但配置成了共阴极
```cpp
// 改为
#define LED_COMMON_ANODE true
```

### Q: 所有LED常灭，无法点亮

**A**: LED_COMMON_ANODE配置错误，硬件是共阴极但配置成了共阳极
```cpp
// 改为
#define LED_COMMON_ANODE false
```

### Q: LED闪烁反相（应该亮时灭）

**A**: 硬件接线与配置不匹配，检查：
1. LED阳极是否真的接VCC
2. LED阴极是否通过电阻接GPIO
3. 配置是否正确

---

## 🔄 极性切换

### 如果硬件改为共阴极

只需修改一行配置：

```cpp
// config.h 第44行
#define LED_COMMON_ANODE false  // 改为false
```

重新编译上传，其他代码无需任何修改！

---

## 📚 详细文档

完整的技术细节、硬件接线图、故障排查流程，请参考：

**[LED_COMMON_ANODE_CONFIG.md](doc/LED_COMMON_ANODE_CONFIG.md)**

---

## 📋 修改清单

- [x] config.h - 添加 `LED_COMMON_ANODE` 配置项
- [x] GoldSky_Utils.ino - 新增 `ledWrite()` 函数
- [x] GoldSky_Utils.ino - 修改 `updateSingleLED()`
- [x] GoldSky_Utils.ino - 修改 `updateLEDIndicators()`
- [x] 创建配置文档 LED_COMMON_ANODE_CONFIG.md

---

**状态**: ✅ 已完成，待硬件测试
**配置**: LED_COMMON_ANODE = **true** (共阳极)
**修改日期**: 2025-11-02
