/*
 * ConfigManager.h
 * 配置管理类 - 使用NVS存储敏感信息
 *
 * 功能:
 * - WiFi凭证管理
 * - API密钥管理
 * - 首次启动时从config.h加载默认值
 * - 支持运行时更新配置
 */

#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <Preferences.h>
#include <Arduino.h>

class ConfigManager {
private:
  Preferences* prefs;

  String wifiSSID;
  String wifiPassword;
  String supabaseURL;
  String supabaseKey;
  String machineID;
  bool initialized = false;

public:
  ConfigManager(Preferences* p) : prefs(p) {}

  // 初始化配置（从NVS加载，如不存在则使用默认值）
  void init(const String& defaultSSID, const String& defaultPass,
            const String& defaultURL, const String& defaultKey,
            const String& defaultMachineID) {

    // 检查是否首次启动
    bool firstBoot = !prefs->getBool("config_init", false);

    if (firstBoot) {
      Serial.println("⚙️ 首次启动，保存默认配置...");
      // 保存默认配置到NVS
      prefs->putString("wifi_ssid", defaultSSID);
      prefs->putString("wifi_pass", defaultPass);
      prefs->putString("supa_url", defaultURL);
      prefs->putString("supa_key", defaultKey);
      prefs->putString("machine_id", defaultMachineID);
      prefs->putBool("config_init", true);
    }

    // 从NVS加载配置
    wifiSSID = prefs->getString("wifi_ssid", defaultSSID);
    wifiPassword = prefs->getString("wifi_pass", defaultPass);
    supabaseURL = prefs->getString("supa_url", defaultURL);
    supabaseKey = prefs->getString("supa_key", defaultKey);
    machineID = prefs->getString("machine_id", defaultMachineID);

    initialized = true;

    Serial.println("✅ 配置已加载:");
    Serial.println("   WiFi SSID: " + wifiSSID);
    Serial.println("   Machine ID: " + machineID);
    Serial.println("   Supabase URL: " + supabaseURL.substring(0, 30) + "...");
  }

  // Getter方法
  String getWiFiSSID() { return wifiSSID; }
  String getWiFiPassword() { return wifiPassword; }
  String getSupabaseURL() { return supabaseURL; }
  String getSupabaseKey() { return supabaseKey; }
  String getMachineID() { return machineID; }

  // Setter方法（运行时更新）
  void setWiFiCredentials(const String& ssid, const String& pass) {
    wifiSSID = ssid;
    wifiPassword = pass;
    prefs->putString("wifi_ssid", ssid);
    prefs->putString("wifi_pass", pass);
    Serial.println("✅ WiFi配置已更新");
  }

  void setSupabaseConfig(const String& url, const String& key) {
    supabaseURL = url;
    supabaseKey = key;
    prefs->putString("supa_url", url);
    prefs->putString("supa_key", key);
    Serial.println("✅ Supabase配置已更新");
  }

  void setMachineID(const String& id) {
    machineID = id;
    prefs->putString("machine_id", id);
    Serial.println("✅ 机器ID已更新: " + id);
  }

  // 重置为默认配置
  void resetToDefaults(const String& defaultSSID, const String& defaultPass,
                       const String& defaultURL, const String& defaultKey,
                       const String& defaultMachineID) {
    prefs->clear();
    init(defaultSSID, defaultPass, defaultURL, defaultKey, defaultMachineID);
    Serial.println("⚠️ 配置已重置为默认值");
  }

  // 检查是否已初始化
  bool isInitialized() { return initialized; }
};

#endif // CONFIG_MANAGER_H
