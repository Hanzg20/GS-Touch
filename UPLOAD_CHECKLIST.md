# GoldSky_Lite 上传检查清单 - 字体优化版本

上传前请确认：

---

## ✅ 代码修改完成

### 修改的文件：
1. ✅ [config.h](config.h) - 显示区域配置
   - `DISPLAY_AREA_PERCENT = 80`
   - `BORDER_STYLE = 0`（无边框）
   - DisplayArea 结构体定义

2. ✅ [GoldSky_Display.ino](GoldSky_Display.ino) - 全部9个显示函数优化
   - displayWelcome()
   - displayPackageSelection()
   - displayCardScan()
   - displayVIPQueryScan()
   - displayVIPInfo()
   - displayProcessing()
   - displayWashProgress()
   - displayComplete()
   - displayError()

---

## 📋 上传步骤

### 1. Arduino IDE 编译检查

```
1. 打开 Arduino IDE
2. 打开 GoldSky_Lite.ino
3. 点击 "验证/编译"（✓ 图标）
4. 确认无错误
```

### 2. 上传到 ESP32-S3

```
1. 连接 ESP32-S3 到电脑
2. 选择端口
3. 点击 "上传"（→ 图标）
4. 等待上传完成
```

### 3. 测试所有界面

打开串口监视器（115200波特率），测试以下界面：

#### 测试清单

- [ ] **欢迎界面** - 标题清晰，3行文字无重叠，底部提示完整显示
- [ ] **套餐选择** - 箭头正常，价格右对齐，文字清晰
- [ ] **刷卡界面** - 标题完整，动画圆圈正常，套餐信息完整
- [ ] **VIP查询** - "VIP Info"标题完整显示
- [ ] **VIP信息** - "VIP Info"标题完整，用户名、卡号、余额、状态都清晰无重叠
- [ ] **处理中** - 进度条正常，百分比居中
- [ ] **洗车进度** - 倒计时数字大而清晰，脉冲计数清晰，进度条正常
- [ ] **完成界面** - 对勾图标居中，感谢文字完整
- [ ] **错误界面** - X图标居中，错误消息完整

---

## 🎯 重点检查项

### 关键优化点验证

1. **字体大小** - 所有标题使用 8pt（helvB08_tf），不再使用 10pt
2. **行间距** - 文字之间至少10px间距，无重叠
3. **"VIP Info"** - 标题完整显示，不截断
4. **数字倒计时** - 洗车进度界面的倒计时大而清晰
5. **80%显示区域** - 所有文字和图形都在可见区域内

---

## 📸 测试反馈

如果测试中发现问题，请提供：

1. **问题描述** - 具体哪个界面，哪个文字有问题
2. **照片** - 拍摄显示屏照片（请拍清楚）
3. **串口日志** - 如果有错误信息，提供串口输出

---

## 🛠️ 如果需要微调

### 调整显示区域比例

如果遮挡罩子覆盖的面积不是80%：

编辑 [config.h](config.h)：
```cpp
#define DISPLAY_AREA_PERCENT 75  // 改为实际百分比（70-95推荐）
```

### 启用边框（可选）

如果需要边框辅助调试：

编辑 [config.h](config.h)：
```cpp
#define BORDER_STYLE 1  // 1=单线, 2=双线, 3=圆角
```

---

## 📚 参考文档

优化过程中创建的文档：

- [FONT_BEST_PRACTICES.md](doc/FONT_BEST_PRACTICES.md) - U8g2字体最佳实践
- [FONT_OPTIMIZATION_SUMMARY.md](doc/FONT_OPTIMIZATION_SUMMARY.md) - 本次优化总结
- [DISPLAY_CONFIG_GUIDE.md](doc/DISPLAY_CONFIG_GUIDE.md) - 显示区域配置指南

---

## ✅ 预期效果

上传成功后，您应该看到：

1. ✅ 所有文字清晰可读，无重叠
2. ✅ "VIP Info"标题完整显示
3. ✅ 文字间距充足，布局舒适
4. ✅ 倒计时数字大而清晰
5. ✅ 所有界面居中对齐
6. ✅ 所有内容都在80%显示区域内（不会被遮挡罩子覆盖）

---

**准备好了吗？开始上传测试吧！** 🚀

---

**创建时间**：2025-11-01
