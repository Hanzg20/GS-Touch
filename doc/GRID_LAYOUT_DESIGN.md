# 网格布局套餐选择设计

设计日期：2025-11-05
设计目标：采用网格布局显示所有套餐，选中状态用反色显示

---

## 🎨 网格布局设计

### 布局结构：2行 × 3列

```
┌────────────────────────────────────────┐
│  ┌────┐  ┌────┐  ┌────┐              │
│  │ $4 │  │ $5 │  │ $8 │   第1行(3个) │
│  │ 4m │  │ 5m │  │ 8m │              │
│  └────┘  └────┘  └────┘              │
│                                        │
│     ┌────┐  ┌────┐       第2行(2个)  │
│     │$15 │  │VIP │                    │
│     │14m │  │Info│                    │
│     └────┘  └────┘                    │
└────────────────────────────────────────┘
```

### 选中状态（反色显示）

```
未选中: 空心边框 + 黑色文字
┌────┐
│ $4 │
│ 4m │
└────┘

选中: 实心背景 + 白色文字
█████
█ $4 █
█ 4m █
█████
```

---

## 📐 精确布局参数

### 网格尺寸
```cpp
cellWidth = 40;    // 每个格子宽度
cellHeight = 24;   // 每个格子高度
gapX = 3;          // 水平间距
gapY = 3;          // 垂直间距
```

### 计算公式

#### 第1行（3个格子）
```cpp
totalWidth = cellWidth × 3 + gapX × 2 = 40×3 + 3×2 = 126像素
row1StartX = (127 - 126) / 2 = 0.5 ≈ 1像素  (居中)
row1Y = area.y + 2 = 8

格子0: X = 1,  Y = 8
格子1: X = 44, Y = 8  (1 + 40 + 3)
格子2: X = 87, Y = 8  (44 + 40 + 3)
```

#### 第2行（2个格子）
```cpp
totalWidth = cellWidth × 2 + gapX × 1 = 40×2 + 3×1 = 83像素
row2StartX = (127 - 83) / 2 = 22像素  (居中)
row2Y = row1Y + cellHeight + gapY = 8 + 24 + 3 = 35

格子3: X = 22, Y = 35
格子4: X = 65, Y = 35  (22 + 40 + 3)
```

---

## 🎯 5个套餐的显示效果

### 格子0: $4套餐（第1行第1列）
```
位置: X=1, Y=8
尺寸: 40×24
内容:
  $4   (Y=18, Bold 8pt)
  4m   (Y=28, Bold 8pt)
```

### 格子1: $5套餐（第1行第2列）
```
位置: X=44, Y=8
尺寸: 40×24
内容:
  $5   (Y=18, Bold 8pt)
  5m   (Y=28, Bold 8pt)
```

### 格子2: $8套餐（第1行第3列）
```
位置: X=87, Y=8
尺寸: 40×24
内容:
  $8   (Y=18, Bold 8pt)
  8m   (Y=28, Bold 8pt)
```

### 格子3: $15套餐（第2行第1列）
```
位置: X=22, Y=35
尺寸: 40×24
内容:
  $15  (Y=45, Bold 8pt)
  15m  (Y=55, Bold 8pt)
```

### 格子4: VIP查询（第2行第2列）
```
位置: X=65, Y=35
尺寸: 40×24
内容:
  VIP  (Y=45, Bold 8pt)
  Info (Y=55, Bold 8pt)
```

---

## 💡 选中状态显示逻辑

### 绘制代码
```cpp
if (i == selectedPackage) {
  // 选中：实心背景 + 白色文字
  display.drawBox(x, y, cellWidth, cellHeight);
  display.setDrawColor(0);  // 白色文字（反色）
} else {
  // 未选中：空心边框 + 黑色文字
  display.drawFrame(x, y, cellWidth, cellHeight);
  display.setDrawColor(1);  // 黑色文字
}

// 绘制文字...
display.drawStr(...);

display.setDrawColor(1);  // 恢复黑色
```

### 视觉效果对比

**选择$4时**：
```
█████  ┌────┐  ┌────┐
█ $4 █  │ $5 │  │ $8 │
█ 4m █  │ 5m │  │ 8m │
█████  └────┘  └────┘

   ┌────┐  ┌────┐
   │$15 │  │VIP │
   │15m │  │Info│
   └────┘  └────┘
```

**选择VIP Info时**：
```
┌────┐  ┌────┐  ┌────┐
│ $4 │  │ $5 │  │ $8 │
│ 4m │  │ 5m │  │ 8m │
└────┘  └────┘  └────┘

   ┌────┐  █████
   │$15 │  █VIP █
   │15m │  █Info█
   └────┘  █████
```

---

## 🔄 屏幕180度旋转

### 旋转配置
```cpp
display.setDisplayRotation(U8G2_R2);  // 180度旋转
```

### 旋转选项说明
- `U8G2_R0`: 0度（默认）
- `U8G2_R1`: 90度顺时针
- `U8G2_R2`: 180度
- `U8G2_R3`: 270度顺时针

### 初始化代码
```cpp
if (display.begin()) {
  display.enableUTF8Print();
  display.setDisplayRotation(U8G2_R2);  // 180度旋转
  sysStatus.displayWorking = true;
  logInfo("✅ OLED初始化成功（SSD1309 + 硬件复位 + 180°旋转）");
}
```

---

## 🎮 按键操作

### $ 键（SELECT键）
**功能**：切换选择下一个套餐
**循环逻辑**：
```cpp
selectedPackage = (selectedPackage + 1) % PACKAGE_COUNT;
```
**顺序**：0 → 1 → 2 → 3 → 4 → 0 → ...

**重置超时**：
```cpp
stateStartTime = millis();  // 重置超时计时器
```

### OK 键
**功能**：确认选择当前套餐
**行为**：
- 普通套餐 → 进入刷卡界面
- VIP Info → 进入VIP查询界面

---

## ⏱️ 超时重置机制

### 问题
用户在选择套餐时按按钮，如果不重置超时计时器，很容易超时返回欢迎界面。

### 解决方案
```cpp
void handleSelectPackageState() {
  if (readButtonImproved(BTN_SELECT)) {
    selectedPackage = (selectedPackage + 1) % PACKAGE_COUNT;
    stateStartTime = millis();  // ✅ 重置超时计时器
    beepShort();
  }

  if (readButtonImproved(BTN_OK)) {
    // 确认选择，进入下一个状态
    stateStartTime = millis();  // ✅ 重置超时计时器
    // ...
  }
}
```

### 超时配置
```cpp
#define STATE_TIMEOUT_SELECT_MS 20000  // 20秒无操作超时
```

---

## 📊 布局优势

### 与横向滑动对比

| 特性 | 横向滑动 | 网格布局 |
|-----|---------|---------|
| 一次显示套餐数 | 1个 | 5个全部 |
| 需要按键次数 | 多次（最多4次） | 1次（直接选中） |
| 用户认知负担 | 需要记忆有多少选项 | 一目了然 |
| 视觉对比 | 无法对比 | 可以对比所有选项 |
| 现代感 | ⭐⭐⭐⭐⭐ | ⭐⭐⭐⭐ |
| 效率 | ⭐⭐⭐ | ⭐⭐⭐⭐⭐ |

### 主要优势

1. **一目了然** - 5个选项同时显示，无需滚动
2. **快速对比** - 用户可以立即看到所有价格和时长
3. **高效选择** - 最多按4次$键即可选到任意套餐
4. **清晰反馈** - 反色显示让选中状态非常明显
5. **节省时间** - 用户无需多次按键探索有哪些选项

---

## 🎨 样式细节

### 字体
```cpp
display.setFont(u8g2_font_helvB08_tf);  // Bold 8pt
```
- 清晰易读
- 适合小格子

### 颜色方案（黑白屏）
**未选中**：
- 边框：黑色（1像素线）
- 背景：白色
- 文字：黑色

**选中**：
- 边框：无
- 背景：黑色（实心）
- 文字：白色（反色）

### 文字对齐
```cpp
int textWidth = display.getStrWidth(line1);
int centerX = x + (cellWidth - textWidth) / 2;
display.drawStr(centerX, y + 10, line1);
```
- 水平居中
- 第1行文字：Y偏移+10
- 第2行文字：Y偏移+20

---

## 🔧 技术实现

### 核心代码
```cpp
void displayPackageSelection() {
  // 定义网格参数
  int cellWidth = 40, cellHeight = 24;
  int gapX = 3, gapY = 3;

  // 计算起始位置
  int row1StartX = (area.width - (cellWidth*3 + gapX*2)) / 2;
  int row2StartX = (area.width - (cellWidth*2 + gapX)) / 2;
  int row1Y = area.y + 2;
  int row2Y = row1Y + cellHeight + gapY;

  // 遍历所有套餐
  for (int i = 0; i < PACKAGE_COUNT; i++) {
    // 计算位置
    int x = (i < 3) ? row1StartX + i * (cellWidth + gapX)
                    : row2StartX + (i-3) * (cellWidth + gapX);
    int y = (i < 3) ? row1Y : row2Y;

    // 绘制格子和文字
    if (i == selectedPackage) {
      display.drawBox(x, y, cellWidth, cellHeight);  // 实心
      display.setDrawColor(0);  // 白色文字
    } else {
      display.drawFrame(x, y, cellWidth, cellHeight);  // 空心
    }

    // 绘制文字（居中）
    // ...
  }
}
```

---

## 📚 相关文件

1. **[GoldSky_Display.ino](../GoldSky_Display.ino)** - Lines 83-154
   - `displayPackageSelection()` 函数实现

2. **[GoldSky_Lite.ino](../GoldSky_Lite.ino)**
   - Line 783: 180度旋转设置
   - Lines 365-388: `handleSelectPackageState()` 按键处理+超时重置

3. **[config.h](../config.h)** - Lines 158-167
   - 套餐配置定义

---

## 🎯 总结

### 设计原则
1. **直观性** - 所有选项一次性展示
2. **对比性** - 用户可以比较所有套餐
3. **响应性** - 反色显示提供即时反馈
4. **效率性** - 减少按键次数，快速选择

### 实现成果
✅ 2行3列网格布局
✅ 5个套餐全部显示
✅ 反色选中状态
✅ 180度屏幕旋转
✅ 按键超时重置

---

**文档版本**: v1.0
**最后更新**: 2025-11-05
**状态**: ✅ 已实现网格布局 + 180度旋转
