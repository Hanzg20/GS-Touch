/*
 * GoldSky_Display.ino
 * 显示函数集合 - 优化2.42"长方形屏幕
 *
 * 包含所有OLED屏幕显示函数（居中+横向布局）
 */

// 外部变量声明（来自 GoldSky_Utils.ino）
extern int nfcReadFailCount;  // NFC连续读卡失败次数

// =================== 辅助函数 ===================
// 计算实际显示区域（考虑遮挡罩子）
// DisplayArea 结构体定义在 config.h 中

DisplayArea getDisplayArea() {
  DisplayArea area;

  // 使用物理可视窗口尺寸（55mm × 23mm）
  area.x = DISPLAY_OFFSET_X;
  area.y = DISPLAY_OFFSET_Y;
  area.width = DISPLAY_VISIBLE_WIDTH;
  area.height = DISPLAY_VISIBLE_HEIGHT;

  return area;
}

// 计算文字居中的X坐标（考虑显示区域）
int getCenterX(const char* text) {
  DisplayArea area = getDisplayArea();
  int textWidth = display.getStrWidth(text);
  return area.x + (area.width - textWidth) / 2;
}

// 绘制边框
void drawBorder() {
  if (BORDER_STYLE == 0) return;  // 0 = 无边框

  DisplayArea area = getDisplayArea();

  if (BORDER_STYLE == 1) {
    // 单线边框
    display.drawFrame(area.x, area.y, area.width, area.height);
  }
  else if (BORDER_STYLE == 2) {
    // 双线边框
    display.drawFrame(area.x, area.y, area.width, area.height);
    display.drawFrame(area.x + 1, area.y + 1, area.width - 2, area.height - 2);
  }
  else if (BORDER_STYLE == 3) {
    // 圆角边框
    display.drawRFrame(area.x, area.y, area.width, area.height, 3);
  }
}

// =================== 显示函数 ===================
void displayWelcome() {
  if (!sysStatus.displayWorking) return;

  DisplayArea area = getDisplayArea();
  display.clearBuffer();

  // 顶部标题
  display.setFont(u8g2_font_helvB10_tf);  // Bold 10pt
  const char* title = "EAGLSON WASH";
  int titleY = area.y + 11;
  display.drawStr(getCenterX(title), titleY, title);

  // 滚动广告文本 (在屏幕中间)
  static unsigned long lastScroll = 0;
  static int scrollPos = 0;

  // VIP推广滚动广告 (吸引人的文案)
  const char* vipAd = "More Cash, More Savings! VIP Top-Up Bonus: $50=$60 | $100=$125 | $200=$240 PLUS Free Tires Change! ";
  display.setFont(u8g2_font_helvB08_tf);  // Bold 8pt
  int adWidth = display.getStrWidth(vipAd);

  // 滚动速度控制
  if (millis() - lastScroll > 60) {  // 更快的滚动
    scrollPos++;
    lastScroll = millis();
  }

  // 循环滚动
  if (scrollPos > adWidth) {
    scrollPos = 0;
  }

  // 绘制滚动文字 (屏幕中间)
  int scrollY = area.y + area.height / 2 + 4;

  // 绘制主滚动文字
  int textX = area.x + area.width - scrollPos;
  display.drawStr(textX, scrollY, vipAd);

  // 如果文字即将离开屏幕，在右侧补充一次显示（无缝循环）
  if (scrollPos > area.width) {
    display.drawStr(textX + adWidth, scrollY, vipAd);
  }

  // 欢迎文字 (底部)
  display.setFont(u8g2_font_6x10_tf);
  const char* welcome = "Welcome    ";
  display.drawStr(getCenterX(welcome), scrollY + 12, welcome);

  // 右下角提示
  const char* prompt = "Press OK";
  int promptX = area.x + area.width - display.getStrWidth(prompt) - 2;
  int promptY = area.y + area.height - 2;
  display.drawStr(promptX, promptY, prompt);

  display.sendBuffer();
}

void displayPackageSelection() {
  if (!sysStatus.displayWorking) return;

  DisplayArea area = getDisplayArea();
  display.clearBuffer();

  // 网格布局: 2行3列
  // 第1行: 0, 1, 2 (3个套餐)
  // 第2行: 3, 4    (2个套餐，居中)

  int cellWidth = 40;   // 每个格子宽度
  int cellHeight = 20;  // 每个格子高度（减小从24→20）
  int gapX = 3;         // 水平间距
  int gapY = 2;         // 垂直间距（减小从3→2）

  // 第一行起始位置（3个格子居中）
  int row1StartX = area.x + (area.width - (cellWidth * 3 + gapX * 2)) / 2;
  int row1Y = area.y + 1;  // 从2→1，向上移动1像素

  // 第二行起始位置（2个格子居中）
  int row2StartX = area.x + (area.width - (cellWidth * 2 + gapX)) / 2;
  int row2Y = row1Y + cellHeight + gapY;

  display.setFont(u8g2_font_helvB08_tf);  // Bold 8pt

  for (int i = 0; i < PACKAGE_COUNT; i++) {
    int x, y;

    // 计算格子位置
    if (i < 3) {
      // 第一行
      x = row1StartX + i * (cellWidth + gapX);
      y = row1Y;
    } else {
      // 第二行
      x = row2StartX + (i - 3) * (cellWidth + gapX);
      y = row2Y;
    }

    // 绘制格子
    if (i == selectedPackage) {
      // 选中：实心背景 + 白色文字（反色）
      display.drawBox(x, y, cellWidth, cellHeight);
      display.setDrawColor(0);  // 白色文字
    } else {
      // 未选中：空心边框 + 黑色文字
      display.drawFrame(x, y, cellWidth, cellHeight);
      display.setDrawColor(1);  // 黑色文字
    }

    // 套餐文字
    char line1[16], line2[16];
    if (packages[i].isQuery) {
      sprintf(line1, "VIP");
      sprintf(line2, "Info");
    } else {
      sprintf(line1, "$%.0f", packages[i].price);
      sprintf(line2, "%dmin", packages[i].minutes);  // 改为 "数值+min"
    }

    // 居中显示文字（调整垂直位置适应20px高度）
    int text1Width = display.getStrWidth(line1);
    int text2Width = display.getStrWidth(line2);

    display.drawStr(x + (cellWidth - text1Width) / 2, y + 9, line1);   // 从10→9
    display.drawStr(x + (cellWidth - text2Width) / 2, y + 18, line2);  // 从20→18

    display.setDrawColor(1);  // 恢复黑色
  }

  display.sendBuffer();
}

void displayCardScan() {
  if (!sysStatus.displayWorking) return;

  // 限制刷新率：只在动画需要更新时刷新（400ms一次）
  static unsigned long lastRefresh = 0;
  static int lastAnimFrame = -1;

  unsigned long now = millis();
  int animFrame = (now / 400) % 3;  // 0,1,2 循环

  // 只在动画帧变化时刷新显示
  if (animFrame == lastAnimFrame && (now - lastRefresh) < 400) {
    return;  // 跳过本次刷新
  }

  lastRefresh = now;
  lastAnimFrame = animFrame;

  DisplayArea area = getDisplayArea();
  display.clearBuffer();

  // 套餐信息（顶部居中）
  const Package& pkg = packages[selectedPackage];
  char buffer[32];
  sprintf(buffer, "$%.0f-%dmin", pkg.price, pkg.minutes);
  display.setFont(u8g2_font_helvB08_tf);
  int titleY = area.y + 9;
  display.drawStr(getCenterX(buffer), titleY, buffer);

  // NFC感应图标动画（屏幕中心）
  int centerX = area.x + area.width / 2;
  int centerY = area.y + area.height / 2;

  // 1. 绘制NFC标签椭圆 (横向，左侧)
  int tagCenterX = centerX - 20;
  int tagCenterY = centerY;
  display.drawEllipse(tagCenterX, tagCenterY, 18, 12, U8G2_DRAW_ALL);  // 横向：rx=18, ry=12

  // 2. 绘制椭圆内的三条波纹线 (横向)
  display.drawLine(tagCenterX - 6, tagCenterY - 6, tagCenterX + 6, tagCenterY - 6);
  display.drawLine(tagCenterX - 8, tagCenterY - 2, tagCenterX + 8, tagCenterY - 2);
  display.drawLine(tagCenterX - 6, tagCenterY + 2, tagCenterX + 6, tagCenterY + 2);

  // 3. 绘制无线信号波纹 (从NFC标签向右扩散)
  for(int i = 0; i < 3; i++) {
    if(i <= animFrame) {
      int waveRadius = 6 + (i * 2);

      // 绘制弧形波纹 (右侧半圆)
      display.drawCircle(tagCenterX + 15, tagCenterY, waveRadius, U8G2_DRAW_UPPER_RIGHT | U8G2_DRAW_LOWER_RIGHT);
    }
  }

  // 4. 绘制手的图标 (右侧)
  int handX = centerX + 25;
  int handY = centerY;

  // 手掌 (矩形)
  display.drawFrame(handX, handY - 8, 10, 16);

  // 手指 (三条短线)
  display.drawLine(handX + 2, handY - 8, handX + 2, handY - 12);
  display.drawLine(handX + 5, handY - 8, handX + 5, handY - 13);
  display.drawLine(handX + 8, handY - 8, handX + 8, handY - 12);

  // 提示文字（右下角，向中间偏移避免遮挡）
  display.setFont(u8g2_font_helvB08_tf);

  // ✅ 优化：根据NFC失败次数显示不同提示
  const char* prompt;
  if (nfcReadFailCount >= 10) {
    // 连续失败10次以上，提示用户调整卡片
    prompt = "Adjust Card";
    // 闪烁提示（每500ms切换）
    if ((millis() / 500) % 2 == 0) {
      display.setFont(u8g2_font_helvB10_tf);  // 加粗字体
    }
  } else {
    prompt = "Tap to Pay";  // ✅ 更友好的提示
  }

  int promptX = area.x + area.width - 60;  // 向左移动10像素（50→60）
  int promptY = area.y + area.height - 3;  // 底部Y坐标
  display.drawStr(promptX, promptY, prompt);

  display.sendBuffer();
}

void displayVIPQueryScan() {
  if (!sysStatus.displayWorking) return;

  DisplayArea area = getDisplayArea();
  display.clearBuffer();

  // 标题
  display.setFont(u8g2_font_helvB10_tf);
  const char* title = "VIP INFO";
  int titleY = area.y + 11;
  display.drawStr(getCenterX(title), titleY, title);

  // NFC感应图标动画
  unsigned long now = millis();
  int animFrame = (now / 400) % 3;  // 0,1,2 循环

  int centerX = area.x + area.width / 2;
  int centerY = area.y + area.height / 2;

  // 1. 绘制NFC标签椭圆 (左侧)
  int tagCenterX = centerX - 20;
  int tagCenterY = centerY;
  display.drawEllipse(tagCenterX, tagCenterY, 12, 18, U8G2_DRAW_ALL);  // 椭圆

  // 2. 绘制椭圆内的三条波纹线
  display.drawLine(tagCenterX - 6, tagCenterY - 6, tagCenterX - 6, tagCenterY + 6);
  display.drawLine(tagCenterX - 2, tagCenterY - 8, tagCenterX - 2, tagCenterY + 8);
  display.drawLine(tagCenterX + 2, tagCenterY - 6, tagCenterX + 2, tagCenterY + 6);

  // 3. 绘制无线信号波纹 (从NFC标签向右扩散)
  for(int i = 0; i < 3; i++) {
    if(i <= animFrame) {
      int waveX = tagCenterX + 15 + (i * 8);  // 波纹起点X
      int waveRadius = 6 + (i * 2);           // 波纹弧度

      // 绘制弧形波纹 (右侧半圆)
      display.drawCircle(tagCenterX + 15, tagCenterY, waveRadius, U8G2_DRAW_UPPER_RIGHT | U8G2_DRAW_LOWER_RIGHT);
    }
  }

  // 4. 绘制手的图标 (右侧)
  int handX = centerX + 25;
  int handY = centerY;

  // 手掌 (矩形)
  display.drawFrame(handX, handY - 8, 10, 16);

  // 手指 (三条短线)
  display.drawLine(handX + 2, handY - 8, handX + 2, handY - 12);
  display.drawLine(handX + 5, handY - 8, handX + 5, handY - 13);
  display.drawLine(handX + 8, handY - 8, handX + 8, handY - 12);

  // 提示文字（底部）
  display.setFont(u8g2_font_helvB08_tf);
  const char* prompt = "Tap VIP Card";
  display.drawStr(getCenterX(prompt), area.y + area.height - 4, prompt);

  display.sendBuffer();
}

void displayVIPInfo() {
  if (!sysStatus.displayWorking) return;

  DisplayArea area = getDisplayArea();
  display.clearBuffer();

  // 标题
  display.setFont(u8g2_font_helvB10_tf);  // Bold 10pt
  const char* title = "VIP INFO";
  int titleY = area.y + 11;
  display.drawStr(getCenterX(title), titleY, title);

  // 横向布局：卡号 | 余额
  int centerY = area.y + area.height / 2 + 4;

  // 左侧：卡号标签 + 卡号
  display.setFont(u8g2_font_helvB08_tf);  // Bold 8pt
  const char* cardLabel = "Card:";
  int labelX = area.x + 5;
  display.drawStr(labelX, centerY, cardLabel);

  // 卡号值（紧跟标签）
  display.setFont(u8g2_font_helvR08_tf);  // Regular 8pt
  char cardNum[20];
  snprintf(cardNum, sizeof(cardNum), "%s", currentCardInfo.displayCardNumber.c_str());
  int cardNumX = labelX + display.getStrWidth(cardLabel) + 3;
  display.drawStr(cardNumX, centerY, cardNum);

  // 右侧：余额（大号显示）
  display.setFont(u8g2_font_helvB10_tf);  // Bold 10pt
  char balance[16];
  snprintf(balance, sizeof(balance), "$%.2f", currentCardInfo.balance);
  int balanceX = area.x + area.width - display.getStrWidth(balance) - 5;
  display.drawStr(balanceX, centerY, balance);

  // 底部状态（居中）
  display.setFont(u8g2_font_helvR08_tf);
  char status[16];
  snprintf(status, sizeof(status), "%s", currentCardInfo.isActive ? "Active" : "Inactive");
  display.drawStr(getCenterX(status), area.y + area.height - 4, status);

  display.sendBuffer();
}

void displayProcessing(const char* message, float progress) {
  if (!sysStatus.displayWorking) return;

  DisplayArea area = getDisplayArea();
  display.clearBuffer();
  // 不画边框，节省空间

  // 消息居中显示 - 使用较大字体
  display.setFont(u8g2_font_helvB10_tf);  // Bold 10pt
  int titleY = area.y + 11;
  display.drawStr(getCenterX(message), titleY, message);

  // 进度条（居中，更大）
  int barW = area.width - 16;
  int barX = area.x + 8;
  int barY = area.y + area.height / 2 - 4;
  int barH = 12;  // 更高的进度条

  display.drawFrame(barX, barY, barW, barH);
  int fillWidth = (int)(progress * (barW - 4));
  if (fillWidth > 0) {
    display.drawBox(barX + 2, barY + 2, fillWidth, barH - 4);
  }

  // 进度百分比（居中）- 使用较大字体
  display.setFont(u8g2_font_helvB08_tf);  // Bold 8pt
  char percentBuffer[8];
  sprintf(percentBuffer, "%d%%", (int)(progress * 100));
  display.drawStr(getCenterX(percentBuffer), barY + 20, percentBuffer);

  display.sendBuffer();
}

void displayWashProgress(int current, int total, int remainingMin, int remainingSec) {
  if (!sysStatus.displayWorking) return;

  DisplayArea area = getDisplayArea();
  display.clearBuffer();

  // 标题居中
  display.setFont(u8g2_font_helvB10_tf);
  int titleY = area.y + 9;
  display.drawStr(getCenterX(TEXT_PROCESSING[currentLanguage]), titleY, TEXT_PROCESSING[currentLanguage]);

  // 齿轮动画区域 (屏幕中心，放大)
  int centerX = area.x + area.width / 2;
  int centerY = area.y + area.height / 2 + 5;

  // 齿轮旋转动画
  static unsigned long lastGearUpdate = 0;
  static int gearAngle = 0;
  if (millis() - lastGearUpdate > 150) {  // 更快的旋转
    gearAngle = (gearAngle + 45) % 360;
    lastGearUpdate = millis();
  }

  // 绘制大齿轮 (右侧，放大2倍)
  int gearX = centerX + 35;
  int gearY = centerY;
  int gearRadius = 16;  // 放大到16像素

  // 齿轮外圈 (双圈加粗)
  display.drawCircle(gearX, gearY, gearRadius);
  display.drawCircle(gearX, gearY, gearRadius - 1);
  display.drawCircle(gearX, gearY, gearRadius - 4);

  // 齿轮齿 (8个齿，加粗)
  for (int i = 0; i < 8; i++) {
    int angle = (i * 45 + gearAngle) % 360;
    float rad = angle * 3.14159 / 180.0;
    int x1 = gearX + (gearRadius - 2) * cos(rad);
    int y1 = gearY + (gearRadius - 2) * sin(rad);
    int x2 = gearX + (gearRadius + 4) * cos(rad);
    int y2 = gearY + (gearRadius + 4) * sin(rad);

    // 加粗齿（绘制两条线）
    display.drawLine(x1, y1, x2, y2);
    display.drawLine(x1 + 1, y1, x2 + 1, y2);
  }

  // 齿轮中心点
  display.drawDisc(gearX, gearY, 3);

  // 脉冲信号动画 (从左到右，更大的数字)
  static unsigned long lastPulse = 0;
  static int pulsePos = 0;
  if (millis() - lastPulse > 120) {  // 更快的流动
    pulsePos = (pulsePos + 10) % 100;
    lastPulse = millis();
  }

  // 绘制大号数字信号流
  display.setFont(u8g2_font_helvB08_tf);  // 大字体
  for (int i = 0; i < 4; i++) {
    int digitX = area.x + 5 + pulsePos + (i * 25);
    if (digitX < gearX - 25) {
      const char* digit = (i % 2 == 0) ? "1" : "0";
      display.drawStr(digitX, centerY + 3, digit);

      // 信号连接线（加粗）
      if (digitX > area.x + 5) {
        display.drawLine(digitX - 8, centerY, digitX - 2, centerY);
        display.drawLine(digitX - 8, centerY + 1, digitX - 2, centerY + 1);
      }
    }
  }

  // 箭头指向齿轮（加粗）
  int arrowX = gearX - 30;
  display.drawLine(arrowX, centerY, arrowX + 15, centerY);
  display.drawLine(arrowX, centerY + 1, arrowX + 15, centerY + 1);
  display.drawLine(arrowX + 15, centerY, arrowX + 10, centerY - 3);
  display.drawLine(arrowX + 15, centerY, arrowX + 10, centerY + 3);

  // 脉冲计数 (底部居中，从1开始)
  display.setFont(u8g2_font_helvB08_tf);
  char buffer[16];
  sprintf(buffer, "%d/%d", current + 1, total);  // current+1 使计数从1开始
  display.drawStr(getCenterX(buffer), area.y + area.height - 3, buffer);

  display.sendBuffer();
}

void displayComplete() {
  if (!sysStatus.displayWorking) return;

  DisplayArea area = getDisplayArea();
  display.clearBuffer();

  // 标题文字居中
  display.setFont(u8g2_font_helvB10_tf);  // Bold 10pt
  int titleY = area.y + 9;
  const char* complete = "Enjoy Wash!";  // 洗车愉快
  display.drawStr(getCenterX(complete), titleY, complete);

  // 对勾图标（居中）
  int centerX = area.x + area.width / 2;
  int centerY = titleY + 16;
  // 画对勾（双线加粗效果）
  display.drawLine(centerX - 10, centerY, centerX - 4, centerY + 8);
  display.drawLine(centerX - 4, centerY + 8, centerX + 10, centerY - 6);
  display.drawLine(centerX - 10, centerY + 1, centerX - 4, centerY + 9);
  display.drawLine(centerX - 4, centerY + 9, centerX + 10, centerY - 5);

  // 剩余余额显示
  display.setFont(u8g2_font_helvB10_tf);  // Bold 10pt
  char balanceText[20];
  sprintf(balanceText, "Remain $%.2f", currentCardInfo.balance);
  int balanceY = centerY + 18;
  display.drawStr(getCenterX(balanceText), balanceY, balanceText);

  display.sendBuffer();
}

void displayError(const char* message) {
  if (!sysStatus.displayWorking) {
    logError(String(message));
    return;
  }

  DisplayArea area = getDisplayArea();
  display.clearBuffer();
  // 不画边框，节省空间

  // 错误标题居中 - 使用较大字体
  display.setFont(u8g2_font_helvB10_tf);  // Bold 10pt
  const char* errorTitle = "ERROR!";
  int titleY = area.y + 11;
  display.drawStr(getCenterX(errorTitle), titleY, errorTitle);

  // X图标（居中在显示区域，更大）
  int centerX = area.x + area.width / 2;
  int centerY = area.y + area.height / 2 + 2;
  // 画更大的X（双线加粗效果）
  display.drawLine(centerX - 10, centerY - 10, centerX + 10, centerY + 10);
  display.drawLine(centerX + 10, centerY - 10, centerX - 10, centerY + 10);
  display.drawLine(centerX - 10, centerY - 9, centerX + 10, centerY + 11);
  display.drawLine(centerX + 10, centerY - 9, centerX - 10, centerY + 11);

  // 错误消息居中 - 使用较大字体
  display.setFont(u8g2_font_helvR08_tf);  // Regular 8pt
  int msgWidth = display.getStrWidth(message);

  if (msgWidth <= area.width - 4) {
    display.drawStr(getCenterX(message), area.y + area.height - 4, message);
  } else {
    display.drawStr(area.x + 2, area.y + area.height - 4, message);
  }

  display.sendBuffer();
}
