// 在 Network.cpp 中更新使用 Utils 工具函数
#include "Network.h"
#include "Logger.h"
#include "Utils.h"  // 添加Utils引用

// 使用Utils的UID转换函数
String Network::hexUIDToDecimal(const String& hexUID) {
  return Utils::hexUIDToDecimal(hexUID);
}

String Network::decimalToHexUID(const String& decimalUID) {
  return Utils::decimalToHexUID(decimalUID);
}

// 使用Utils的系统信息
String Network::getConnectionStatus() {
  String status = Utils::getWiFiInfo();
  status += "API Retries: " + String(apiRetryCount) + "\n";
  status += "Last Error: " + lastError + "\n";
  
  return status;
}
