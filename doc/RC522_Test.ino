/*
 * RC522 NFC 模块测试程序
 * 用于诊断 RC522 初始化问题
 *
 * 接线：
 * RC522 SDA  -> GPIO 10
 * RC522 SCK  -> GPIO 12
 * RC522 MOSI -> GPIO 11
 * RC522 MISO -> GPIO 13
 * RC522 RST  -> GPIO 14
 * RC522 3.3V -> 3.3V
 * RC522 GND  -> GND
 */

#include <SPI.h>
#include <MFRC522.h>

// 引脚定义
#define RC522_CS        10
#define SPI_MOSI        11
#define SPI_SCK         12
#define SPI_MISO        13
#define RC522_RST       14

MFRC522 mfrc522(RC522_CS, RC522_RST);

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("=====================================");
  Serial.println("RC522 NFC 模块测试程序");
  Serial.println("=====================================");

  // 测试 UID 转换函数
  Serial.println("\n🧪 测试 UID 转换:");
  testUIDConversion("2345408116");
  testUIDConversion("1210711100");
  testUIDConversion("305419896");

  // 配置引脚
  Serial.println("\n📌 配置 GPIO 引脚...");
  pinMode(RC522_CS, OUTPUT);
  pinMode(RC522_RST, OUTPUT);

  // 先设置为高电平
  digitalWrite(RC522_CS, HIGH);
  digitalWrite(RC522_RST, HIGH);
  delay(100);

  Serial.printf("   CS 引脚状态: %d\n", digitalRead(RC522_CS));
  Serial.printf("   RST 引脚状态: %d\n", digitalRead(RC522_RST));

  // 硬件复位序列
  Serial.println("\n🔄 执行硬件复位序列...");
  digitalWrite(RC522_RST, LOW);
  Serial.printf("   步骤1: RST 拉低 -> %d\n", digitalRead(RC522_RST));
  delay(50);

  digitalWrite(RC522_RST, HIGH);
  Serial.printf("   步骤2: RST 拉高 -> %d\n", digitalRead(RC522_RST));
  delay(100);

  // 初始化 SPI
  Serial.println("\n📡 初始化 SPI 总线...");
  SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI, RC522_CS);
  delay(50);
  Serial.println("   SPI 初始化完成");

  // 初始化 RC522
  Serial.println("\n🔧 初始化 RC522...");
  mfrc522.PCD_Init();
  delay(100);

  // 软复位
  Serial.println("   执行软复位...");
  mfrc522.PCD_Reset();
  delay(100);

  // 读取版本号
  Serial.println("\n📊 读取 RC522 寄存器:");
  byte version = mfrc522.PCD_ReadRegister(mfrc522.VersionReg);
  Serial.printf("   固件版本: 0x%02X ", version);

  if (version == 0x91 || version == 0x92) {
    Serial.println("✅ 正常 (MFRC522 v1.0/v2.0)");
  } else if (version == 0x00) {
    Serial.println("❌ 读取失败 (0x00 = 无响应)");
  } else if (version == 0xFF) {
    Serial.println("❌ 读取失败 (0xFF = 总线错误)");
  } else {
    Serial.printf("⚠️ 未知版本 (可能是兼容芯片)\n");
  }

  // 读取其他关键寄存器
  byte comIrq = mfrc522.PCD_ReadRegister(mfrc522.ComIrqReg);
  byte error = mfrc522.PCD_ReadRegister(mfrc522.ErrorReg);
  byte status = mfrc522.PCD_ReadRegister(mfrc522.Status1Reg);
  byte control = mfrc522.PCD_ReadRegister(mfrc522.ControlReg);

  Serial.printf("   ComIrq寄存器: 0x%02X\n", comIrq);
  Serial.printf("   Error寄存器: 0x%02X\n", error);
  Serial.printf("   Status寄存器: 0x%02X\n", status);
  Serial.printf("   Control寄存器: 0x%02X\n", control);

  // 天线状态
  byte txControl = mfrc522.PCD_ReadRegister(mfrc522.TxControlReg);
  Serial.printf("   天线控制: 0x%02X ", txControl);
  if (txControl & 0x03) {
    Serial.println("✅ 天线已开启");
  } else {
    Serial.println("❌ 天线关闭");
    Serial.println("   尝试开启天线...");
    mfrc522.PCD_AntennaOn();
    delay(50);
    txControl = mfrc522.PCD_ReadRegister(mfrc522.TxControlReg);
    Serial.printf("   重新检测: 0x%02X %s\n", txControl, (txControl & 0x03) ? "✅" : "❌");
  }

  // 执行自检
  Serial.println("\n🔬 执行 RC522 自检测试...");
  Serial.println("   (这将花费约 5 秒钟)");
  bool selfTestResult = mfrc522.PCD_PerformSelfTest();

  if (selfTestResult) {
    Serial.println("   ✅ 自检通过！RC522 硬件正常");
  } else {
    Serial.println("   ❌ 自检失败！可能原因:");
    Serial.println("      1. SPI 接线错误（MOSI/MISO 是否接反？）");
    Serial.println("      2. 供电不足（用万用表测量 3.3V）");
    Serial.println("      3. RC522 模块损坏");
  }

  // 重新初始化（自检会改变寄存器）
  mfrc522.PCD_Init();
  delay(50);

  Serial.println("\n=====================================");
  Serial.println("✅ 测试完成！");
  Serial.println("请将 NFC 卡片靠近 RC522 模块...");
  Serial.println("=====================================\n");
}

void loop() {
  // 检测卡片
  if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
    Serial.println("\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
    Serial.println("🎉 检测到 NFC 卡片！");

    // 显示 UID
    Serial.print("   UID (HEX): ");
    String hexUID = "";
    for (byte i = 0; i < mfrc522.uid.size; i++) {
      if (mfrc522.uid.uidByte[i] < 0x10) {
        Serial.print("0");
        hexUID += "0";
      }
      Serial.print(mfrc522.uid.uidByte[i], HEX);
      hexUID += String(mfrc522.uid.uidByte[i], HEX);
      if (i < mfrc522.uid.size - 1) Serial.print(":");
    }
    hexUID.toUpperCase();
    Serial.println();

    // 转换为十进制
    String decimalUID = hexToDecimal(hexUID);
    Serial.printf("   UID (DEC): %s\n", decimalUID.c_str());

    // 验证转换
    String backToHex = decimalToHex(decimalUID);
    Serial.printf("   转换验证: %s %s\n", backToHex.c_str(),
                  (backToHex == hexUID) ? "✅" : "❌");

    // 卡片类型
    byte piccType = mfrc522.PICC_GetType(mfrc522.uid.sak);
    Serial.printf("   卡片类型: %s\n", mfrc522.PICC_GetTypeName(piccType));
    Serial.printf("   SAK 值: 0x%02X\n", mfrc522.uid.sak);
    Serial.printf("   UID 长度: %d 字节\n", mfrc522.uid.size);

    Serial.println("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");

    // 停止读卡
    mfrc522.PICC_HaltA();
    mfrc522.PCD_StopCrypto1();

    delay(1000);  // 防止重复读取
  }

  delay(100);
}

// ===== 工具函数 =====

void testUIDConversion(const String& decimalUID) {
  String hexUID = decimalToHex(decimalUID);
  String backToDec = hexToDecimal(hexUID);

  Serial.printf("   DEC %s -> HEX %s -> DEC %s %s\n",
                decimalUID.c_str(), hexUID.c_str(), backToDec.c_str(),
                (backToDec == decimalUID) ? "✅" : "❌");
}

String hexToDecimal(const String& hexUID) {
  unsigned long decimalValue = 0;
  String cleanHex = hexUID;
  cleanHex.replace(":", "");  // 移除冒号

  for (int i = 0; i < cleanHex.length(); i++) {
    char c = cleanHex.charAt(i);
    decimalValue *= 16;

    if (c >= '0' && c <= '9') {
      decimalValue += c - '0';
    } else if (c >= 'A' && c <= 'F') {
      decimalValue += c - 'A' + 10;
    } else if (c >= 'a' && c <= 'f') {
      decimalValue += c - 'a' + 10;
    }
  }

  return String(decimalValue);
}

String decimalToHex(const String& decimalUID) {
  // 🔥 修正：手动转换避免 toInt() 溢出
  unsigned long decimalValue = 0;

  for (int i = 0; i < decimalUID.length(); i++) {
    char c = decimalUID.charAt(i);
    if (c >= '0' && c <= '9') {
      decimalValue = decimalValue * 10 + (c - '0');
    }
  }

  String hexUID = String(decimalValue, HEX);
  hexUID.toUpperCase();

  while(hexUID.length() < 8) {
    hexUID = "0" + hexUID;
  }

  return hexUID;
}
