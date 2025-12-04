/*
 * HealthMonitor.h - 系统健康度监测模块
 *
 * 功能：
 * - 每30分钟自动采集系统健康度数据
 * - 上传到 Supabase 数据库
 * - 帮助诊断刷卡无响应等问题
 *
 * 版本: v1.0
 * 日期: 2025-12-04
 */

#ifndef HEALTH_MONITOR_H
#define HEALTH_MONITOR_H

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "config.h"
#include "ConfigManager.h"

// =================== 健康度监测配置 ===================
#define HEALTH_LOG_INTERVAL 1800000  // 30分钟 (毫秒)
// #define HEALTH_LOG_INTERVAL 300000   // 5分钟 (测试用)

// =================== 全局健康度指标 ===================
extern HealthMetrics healthMetrics;
extern ConfigManager config;  // 使用外部配置管理器

// =================== 健康度监测类 ===================
class HealthMonitor {
private:
  unsigned long lastHealthLogTime = 0;
  String deviceId = "";

  // 获取设备ID (使用MAC地址)
  String getDeviceId() {
    if (deviceId.length() == 0) {
      uint8_t mac[6];
      WiFi.macAddress(mac);
      char macStr[18];
      sprintf(macStr, "%02X:%02X:%02X:%02X:%02X:%02X",
              mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
      deviceId = String(macStr);
    }
    return deviceId;
  }

  // 构建健康度日志 JSON
  String buildHealthLogJSON() {
    JsonDocument doc;

    doc["device_id"] = getDeviceId();

    // 系统运行时间
    doc["uptime_seconds"] = healthMetrics.uptimeSeconds;
    doc["uptime_hours"] = (float)healthMetrics.uptimeSeconds / 3600.0;

    // 内存状态
    doc["free_heap"] = healthMetrics.freeHeap;
    doc["min_free_heap"] = healthMetrics.minFreeHeap;
    doc["heap_usage_percent"] = healthMetrics.getHeapUsagePercent();

    // WiFi 状态
    doc["wifi_connected"] = healthMetrics.wifiConnected;
    doc["wifi_rssi"] = healthMetrics.wifiRSSI;
    doc["wifi_reconnect_count"] = healthMetrics.wifiReconnectCount;

    // NFC 模块状态
    doc["nfc_initialized"] = healthMetrics.nfcInitialized;
    doc["nfc_firmware_version"] = healthMetrics.nfcFirmwareVersion;
    doc["nfc_last_read_success"] = healthMetrics.nfcLastReadSuccess;
    doc["nfc_read_success_count"] = healthMetrics.nfcReadSuccessCount;
    doc["nfc_read_fail_count"] = healthMetrics.nfcReadFailCount;
    doc["nfc_success_rate"] = healthMetrics.getNFCSuccessRate();
    doc["nfc_idle_minutes"] = healthMetrics.getNFCIdleMinutes();

    // OLED/I2C 状态
    doc["oled_working"] = healthMetrics.oledWorking;
    doc["i2c_error_count"] = healthMetrics.i2cErrorCount;

    // 业务统计
    doc["total_transactions"] = healthMetrics.totalTransactions;
    doc["transactions_last_hour"] = healthMetrics.transactionsLastHour;

    // 系统状态
    doc["current_state"] = healthMetrics.currentState;
    doc["loop_execution_time_ms"] = healthMetrics.loopExecutionTimeMs;
    doc["watchdog_reset_count"] = healthMetrics.watchdogResetCount;

    // 错误统计
    doc["last_error"] = healthMetrics.lastError;
    doc["error_count_last_30min"] = healthMetrics.errorCountLast30Min;

    String jsonString;
    serializeJson(doc, jsonString);
    return jsonString;
  }

public:
  // 初始化
  void begin() {
    lastHealthLogTime = millis();
    Serial.println("🏥 健康度监测系统已启动");
    Serial.println("   设备ID: " + getDeviceId());
    Serial.println("   上传间隔: " + String(HEALTH_LOG_INTERVAL / 60000) + " 分钟");
  }

  // 定期检查并上传健康度日志
  void checkAndUpload() {
    unsigned long currentTime = millis();

    // 检查是否到达上传时间
    if (currentTime - lastHealthLogTime >= HEALTH_LOG_INTERVAL) {
      uploadHealthLog();
      lastHealthLogTime = currentTime;
    }
  }

  // 立即上传健康度日志
  bool uploadHealthLog() {
    // 更新健康度指标
    healthMetrics.update();

    Serial.println("\n📊 正在上传健康度日志...");

    // 检查WiFi连接
    if (!WiFi.isConnected()) {
      Serial.println("❌ WiFi未连接，跳过健康度日志上传");
      return false;
    }

    HTTPClient http;
    String url = config.getSupabaseURL() + "/rest/v1/system_health_logs";

    http.begin(url);
    String apiKey = config.getSupabaseKey();
    http.addHeader("apikey", apiKey.c_str());
    http.addHeader("Authorization", ("Bearer " + apiKey).c_str());
    http.addHeader("Content-Type", "application/json");
    http.addHeader("Prefer", "return=minimal");

    String jsonPayload = buildHealthLogJSON();

    Serial.println("   设备ID: " + getDeviceId());
    Serial.println("   运行时长: " + String(healthMetrics.uptimeSeconds / 3600.0, 1) + " 小时");
    Serial.println("   剩余内存: " + String(healthMetrics.freeHeap / 1024) + " KB");
    Serial.println("   NFC成功率: " + String(healthMetrics.getNFCSuccessRate(), 1) + "%");

    int httpCode = http.POST(jsonPayload);

    bool success = false;
    if (httpCode == 201 || httpCode == 200) {
      Serial.println("✅ 健康度日志上传成功");
      success = true;
    } else {
      Serial.println("❌ 健康度日志上传失败");
      Serial.println("   HTTP Code: " + String(httpCode));
      if (httpCode > 0) {
        String response = http.getString();
        Serial.println("   Response: " + response);
      } else {
        Serial.println("   Error: " + http.errorToString(httpCode));
      }
    }

    http.end();
    return success;
  }

  // 记录错误
  void recordError(const String& error) {
    healthMetrics.lastError = error;
    healthMetrics.errorCountLast30Min++;
    healthMetrics.lastErrorTime = millis();
  }

  // 记录NFC读卡成功
  void recordNFCSuccess() {
    healthMetrics.nfcReadSuccessCount++;
    healthMetrics.nfcLastReadSuccess = true;
    healthMetrics.nfcLastActiveTime = millis();
  }

  // 记录NFC读卡失败
  void recordNFCFailure() {
    healthMetrics.nfcReadFailCount++;
    healthMetrics.nfcLastReadSuccess = false;
  }

  // 记录交易
  void recordTransaction() {
    healthMetrics.totalTransactions++;
    healthMetrics.transactionsLastHour++;
    healthMetrics.lastTransactionTime = millis();
  }

  // 重置30分钟错误计数
  void resetErrorCount30Min() {
    healthMetrics.errorCountLast30Min = 0;
  }

  // 重置最近1小时交易计数
  void resetTransactionCountLastHour() {
    healthMetrics.transactionsLastHour = 0;
  }

  // 打印当前健康度状态（用于调试）
  void printStatus() {
    healthMetrics.update();

    Serial.println("\n╔════════════════════════════════════════════════╗");
    Serial.println("║           系统健康度状态报告                   ║");
    Serial.println("╚════════════════════════════════════════════════╝");
    Serial.println("🔹 设备ID: " + getDeviceId());
    Serial.println("🔹 运行时长: " + String(healthMetrics.uptimeSeconds / 3600.0, 2) + " 小时");
    Serial.println("\n📱 内存状态:");
    Serial.println("   剩余堆内存: " + String(healthMetrics.freeHeap / 1024) + " KB");
    Serial.println("   最小堆内存: " + String(healthMetrics.minFreeHeap / 1024) + " KB");
    Serial.println("   堆使用率: " + String(healthMetrics.getHeapUsagePercent(), 1) + "%");
    Serial.println("\n📡 WiFi 状态:");
    Serial.println("   连接状态: " + String(healthMetrics.wifiConnected ? "已连接" : "未连接"));
    Serial.println("   信号强度: " + String(healthMetrics.wifiRSSI) + " dBm");
    Serial.println("   重连次数: " + String(healthMetrics.wifiReconnectCount));
    Serial.println("\n💳 NFC 状态:");
    Serial.println("   初始化状态: " + String(healthMetrics.nfcInitialized ? "正常" : "异常"));
    Serial.println("   固件版本: " + healthMetrics.nfcFirmwareVersion);
    Serial.println("   成功读卡: " + String(healthMetrics.nfcReadSuccessCount) + " 次");
    Serial.println("   失败读卡: " + String(healthMetrics.nfcReadFailCount) + " 次");
    Serial.println("   成功率: " + String(healthMetrics.getNFCSuccessRate(), 1) + "%");
    Serial.println("   空闲时长: " + String(healthMetrics.getNFCIdleMinutes()) + " 分钟");
    Serial.println("\n📺 OLED 状态:");
    Serial.println("   工作状态: " + String(healthMetrics.oledWorking ? "正常" : "异常"));
    Serial.println("   I2C错误: " + String(healthMetrics.i2cErrorCount) + " 次");
    Serial.println("\n💰 业务统计:");
    Serial.println("   总交易数: " + String(healthMetrics.totalTransactions));
    Serial.println("   最近1小时: " + String(healthMetrics.transactionsLastHour) + " 笔");
    Serial.println("\n⚙️ 系统状态:");
    Serial.println("   当前状态: " + healthMetrics.currentState);
    Serial.println("   循环耗时: " + String(healthMetrics.loopExecutionTimeMs) + " ms");
    Serial.println("\n⚠️ 错误统计:");
    Serial.println("   最后错误: " + (healthMetrics.lastError.length() > 0 ? healthMetrics.lastError : "无"));
    Serial.println("   最近30分钟: " + String(healthMetrics.errorCountLast30Min) + " 次错误");
    Serial.println("════════════════════════════════════════════════\n");
  }
};

#endif // HEALTH_MONITOR_H
