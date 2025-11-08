# U8g2 字体最佳实践 - 2.42" OLED (128x64)

GoldSky_Lite 字体优化指南

---

## 📐 显示区域限制

### 物理屏幕
- 全屏：128px × 64px

### 80% 显示区域（推荐配置）
- 可用宽度：**102px**
- 可用高度：**51px**
- 边距：上下左右各 13px

---

## 🎨 U8g2 字体分类

### 1. **字体命名规则**

```
u8g2_font_<name>_<size>_<type>

name: 字体名称 (helvB=Helvetica Bold, helvR=Helvetica Regular, etc.)
size: 字体大小 (6x10, 7x13, 8x13, 10x20, etc.)
type: 字体类型
  - tf: Transparent font (透明背景)
  - tn: Transparent numbers only (仅数字)
  - tr: Transparent restricted (部分字符)
```

### 2. **常用字体系列对比**

| 字体系列 | 特点 | 可读性 | 空间占用 | 推荐场景 |
|---------|------|--------|---------|---------|
| **6x10, 6x12, 6x13** | 小号等宽 | ★★★ | 小 | 列表、详细信息 |
| **7x13, 7x14** | 中号等宽 | ★★★★ | 中 | 通用文本 |
| **helvR08** | Helvetica 8pt | ★★★★ | 小 | ✅ 推荐：通用 |
| **helvB08** | Helvetica Bold 8pt | ★★★★★ | 中 | ✅ 推荐：标题 |
| **helvB10** | Helvetica Bold 10pt | ★★★★ | 大 | 大标题 |
| **logisoso16_tn** | 数字专用 16pt | ★★★★★ | 大 | ⚠️ 仅数字！ |
| **unifont_t_symbols** | 符号字体 | N/A | 中 | ⚠️ 仅符号！ |

---

## ⚠️ 常见字体陷阱

### 陷阱 1：数字专用字体

```cpp
// ❌ 错误：用数字字体显示文字
display.setFont(u8g2_font_logisoso16_tn);
display.drawStr(x, y, "VIP Info");  // 不会显示！

// ✅ 正确：只用于显示数字
display.setFont(u8g2_font_logisoso16_tn);
display.drawStr(x, y, "12:34");  // 正常显示
```

**数字专用字体**：
- `u8g2_font_logisoso16_tn`
- `u8g2_font_logisoso20_tn`
- `u8g2_font_logisoso24_tn`
- 所有带 `_tn` 后缀的字体

### 陷阱 2：符号字体

```cpp
// ❌ 错误：用符号字体显示文字
display.setFont(u8g2_font_unifont_t_symbols);
display.drawStr(x, y, "Select");  // 不会显示！

// ✅ 正确：只用于显示符号
display.setFont(u8g2_font_unifont_t_symbols);
display.drawGlyph(x, y, 0x25b6);  // ▶ 显示箭头
```

### 陷阱 3：字体太大超出显示区域

```cpp
// ❌ 在 80% 显示区域 (102px) 使用大字体
display.setFont(u8g2_font_helvB10_tf);
display.drawStr(x, y, "Select Service");  // 可能显示不全！

// ✅ 使用合适大小的字体
display.setFont(u8g2_font_helvB08_tf);  // 8pt 足够
display.drawStr(x, y, "Select Service");  // 完整显示
```

---

## 📏 字体大小与显示区域

### 80% 显示区域 (102px 宽)

| 字体 | 字符宽度 | 最多字符数 | 示例文本长度 |
|------|---------|-----------|------------|
| `6x10_tf` | ~6px | **17 字符** | "Press OK to Star" |
| `6x12_tf` | ~6px | **17 字符** | "Press OK to Star" |
| `helvR08_tf` | ~5-6px | **15-17 字符** | "24h Self-Service" ✅ |
| `helvB08_tf` | ~6-7px | **14-17 字符** | "Select Service" ✅ |
| `helvB10_tf` | ~8-10px | **10-13 字符** | "EAGLSON WASH" ✅ |
| `logisoso16_tn` | ~12px | **8 数字** | "12:34:56" ✅ |

---

## ✅ 推荐字体组合

### 方案 1：简洁风格（推荐）

```cpp
// 大标题
u8g2_font_helvB10_tf         // "EAGLSON WASH"

// 标题
u8g2_font_helvB08_tf         // "Select Service"

// 正文
u8g2_font_helvR08_tf         // "24h Self-Service"

// 小字/状态
u8g2_font_6x12_tf            // "Active | Premium"

// 数字倒计时
u8g2_font_logisoso16_tn      // "05:30"
```

### 方案 2：紧凑风格

```cpp
// 大标题
u8g2_font_helvB08_tf         // 标题统一用 8pt

// 标题
u8g2_font_7x13_tf            // 等宽字体

// 正文
u8g2_font_6x12_tf            // 小号等宽

// 小字
u8g2_font_6x10_tf            // 更小

// 数字
u8g2_font_10x20_tf           // 等宽数字字体
```

---

## 📐 行间距最佳实践

### 计算公式

```cpp
行高 = 字体高度 + 间距

推荐间距：
- 6x10 字体 → 行高 10-11px
- 6x12 字体 → 行高 12-13px
- helvR08 → 行高 10-12px
- helvB08 → 行高 11-13px
- helvB10 → 行高 13-15px
```

### 示例：80% 显示区域 (51px 高)

```cpp
标题区: 11px (helvB10 + 分隔线)
内容区: 36px (3行文本，每行12px)
底部区: 4px (小字提示)
总计: 51px ✅
```

---

## 🎯 各界面优化建议

### 1. 欢迎界面

**80% 区域限制**：102px × 51px

```cpp
// ✅ 优化后布局
标题 (helvB08): "EAGLSON WASH"     // Y=10, 适中大小
分隔线:           ─────────────      // Y=12
正文1 (helvR08):  "24h Self-Service" // Y=24, 间距12px
正文2 (helvR08):  "Quick & Safe"     // Y=34, 间距10px
正文3 (helvR08):  "Member Discount"  // Y=44, 间距10px
提示 (6x10):      "Press OK"         // Y=51, 底部
```

**关键点**：
- ❌ 不要用 helvB10（太大）
- ✅ 用 helvB08 作为标题
- ✅ 行间距 10-12px

### 2. 套餐选择

```cpp
// ✅ 优化后布局
标题 (helvB08): "Select Service"  // Y=10
分隔线:          ─────────────     // Y=12
列表项 (6x12):                     // 起始 Y=20
  ▶ Quick Wash      $4 4min       // Y=20, 间距10px
    Standard        $6 6min       // Y=30
    Deluxe          $9 8min       // Y=40
    Premium        $12 10min      // Y=50
```

**关键点**：
- ✅ 用 6x12（紧凑清晰）
- ✅ 箭头用 drawGlyph(0x25b6)
- ✅ 价格右对齐

### 3. 洗车进度

```cpp
// ✅ 优化后布局
标题 (helvB08):   "Processing"    // Y=10
分隔线:            ─────────        // Y=12
倒计时 (logisoso16): "05:30"      // Y=28, 大号数字
脉冲 (helvR08):    "Pulse 3/12"   // Y=38
进度条:            [████░░░░]      // Y=44-51
```

**关键点**：
- ✅ logisoso16_tn 仅用于数字
- ✅ 倒计时居中突出
- ✅ 进度条在底部

### 4. VIP 信息

```cpp
// ❌ 问题：文字太长 + 字体太大
标题 (helvB08): "VIP Card Info"  // 可能显示不全

// ✅ 优化方案1：缩短文字
标题 (helvB08): "VIP Info"       // ✅ 9个字符

// ✅ 优化方案2：用小字体
标题 (helvR08): "VIP Card Info"  // ✅ 完整显示

// ✅ 优化方案3：分两行
行1 (helvB08): "VIP"
行2 (helvR08): "Card Information"
```

---

## 🧪 字体测试工具

### 测量文字宽度

```cpp
display.setFont(u8g2_font_helvB08_tf);
int width = display.getStrWidth("VIP Card Info");
Serial.printf("文字宽度: %d px\n", width);

// 判断是否超出显示区域
DisplayArea area = getDisplayArea();
if (width > area.width - 4) {
  Serial.println("⚠️ 文字太长！");
}
```

### 测试字体高度

```cpp
int height = display.getMaxCharHeight();
Serial.printf("字体高度: %d px\n", height);
```

---

## 📋 字体选择决策树

```
需要显示什么？
├─ 纯数字 (倒计时)
│  └─ logisoso16_tn (大号清晰)
│
├─ 符号 (箭头、图标)
│  └─ unifont_t_symbols + drawGlyph()
│
├─ 英文标题 (粗体)
│  ├─ 长度 < 13字符 → helvB10_tf
│  └─ 长度 > 13字符 → helvB08_tf
│
├─ 英文正文
│  └─ helvR08_tf
│
└─ 列表/详细信息
   └─ 6x12_tf (紧凑清晰)
```

---

## ⚙️ 最佳实践总结

### ✅ DO

1. **测量文字宽度** - 确保不超出显示区域
2. **使用专用字体** - 数字用数字字体，文字用文字字体
3. **合理行间距** - 至少字体高度 + 2px
4. **短标题** - 在 80% 区域下，标题 < 13 字符
5. **渐进测试** - 从小字体开始，逐步调大

### ❌ DON'T

1. ❌ 数字字体显示文字
2. ❌ 字体太大超出区域
3. ❌ 行间距太小导致重叠
4. ❌ 不测量直接使用
5. ❌ 混用多种字体风格

---

## 🔗 参考资源

- [U8g2 字体列表](https://github.com/olikraus/u8g2/wiki/fntlistall)
- [U8g2 API 文档](https://github.com/olikraus/u8g2/wiki/u8g2reference)

---

**最后更新**: 2025-11-01
