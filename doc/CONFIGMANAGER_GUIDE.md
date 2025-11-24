# ConfigManager 使用指南

## 📚 什么是 ConfigManager？

ConfigManager 是一个配置管理类，用于安全地存储和管理敏感信息（WiFi凭证、API密钥等）。

### 核心功能
- ✅ 使用 ESP32 的 NVS (Non-Volatile Storage) 加密存储
- ✅ 首次启动自动从 config.h 加载默认值
- ✅ 支持运行时动态更新配置
- ✅ 重启后自动恢复配置
- ✅ 无需重新编译即可修改配置

---

## 🔄 两种使用方式对比

### 方式1: 明文配置（当前使用）

**config.h**:
```cpp
#define WIFI_SSID "hanzg_hanyh"
#define WIFI_PASSWORD "han1314521"
#define SUPABASE_URL "https://ttbtxxpnvkcbyugzdqfw.supabase.co"
#define SUPABASE_KEY "eyJhbGci..."
```

**主程序**:
```cpp
WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
String url = SUPABASE_URL;
String key = SUPABASE_KEY;
```

**优点**:
- ✅ 简单直接
- ✅ 不需要额外代码

**缺点**:
- ❌ 密码明文硬编码
- ❌ 修改配置需重新编译上传
- ❌ 代码泄露=密码泄露
- ❌ 不适合生产环境

---

### 方式2: ConfigManager（推荐）

**config.h**:
```cpp
// 仅作为默认值（首次启动时使用）
#define DEFAULT_WIFI_SSID "default_ssid"
#define DEFAULT_WIFI_PASSWORD "default_password"
#define DEFAULT_SUPABASE_URL "https://example.supabase.co"
#define DEFAULT_SUPABASE_KEY "default_key"
```

**主程序**:
```cpp
// 全局变量
Preferences prefs;
ConfigManager config(&prefs);

void setup() {
    // 初始化 NVS
    prefs.begin("goldsky", false);

    // 初始化 ConfigManager
    config.init(DEFAULT_WIFI_SSID,
                DEFAULT_WIFI_PASSWORD,
                DEFAULT_SUPABASE_URL,
                DEFAULT_SUPABASE_KEY,
                MACHINE_ID);

    // 使用配置
    WiFi.begin(config.getWiFiSSID(), config.getWiFiPassword());
}
```

**优点**:
- ✅ 配置加密存储在 NVS
- ✅ 可运行时更新，无需重新编译
- ✅ 代码不包含真实密码
- ✅ 适合生产环境

**缺点**:
- ❌ 需要额外代码
- ❌ 稍微复杂一点

---

## 🚀 如何启用 ConfigManager

### 步骤1: 修改 config.h

将明文配置改为默认值：

```cpp
// =================== WiFi和网络配置 ===================
// 注意：这些是默认值，首次启动时会保存到NVS
// 之后可通过ConfigManager运行时修改
#define DEFAULT_WIFI_SSID "hanzg_hanyh"
#define DEFAULT_WIFI_PASSWORD "han1314521"
#define WIFI_TIMEOUT_MS 20000

// =================== Supabase 配置 ===================
#define DEFAULT_SUPABASE_URL "https://ttbtxxpnvkcbyugzdqfw.supabase.co"
#define DEFAULT_SUPABASE_KEY "eyJhbGci..."
```

### 步骤2: 修改主程序初始化

**GoldSky_Lite.ino** 中找到 `setup()` 函数：

```cpp
// 在文件顶部添加全局变量
Preferences prefs;
ConfigManager config(&prefs);

void setup() {
    Serial.begin(115200);

    // 初始化 NVS
    prefs.begin("goldsky", false);  // "goldsky" 是命名空间

    // 初始化 ConfigManager
    config.init(DEFAULT_WIFI_SSID,
                DEFAULT_WIFI_PASSWORD,
                DEFAULT_SUPABASE_URL,
                DEFAULT_SUPABASE_KEY,
                MACHINE_ID);

    // ... 其他初始化代码
}
```

### 步骤3: 修改 WiFi 连接代码

找到所有使用 `WIFI_SSID` 和 `WIFI_PASSWORD` 的地方：

**旧代码**:
```cpp
WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
```

**新代码**:
```cpp
WiFi.begin(config.getWiFiSSID().c_str(),
           config.getWiFiPassword().c_str());
```

### 步骤4: 修改 API 调用代码

找到所有使用 `SUPABASE_URL` 和 `SUPABASE_KEY` 的地方：

**旧代码**:
```cpp
String url = String(SUPABASE_URL) + "/rest/v1/transactions";
http.addHeader("apikey", SUPABASE_KEY);
```

**新代码**:
```cpp
String url = config.getSupabaseURL() + "/rest/v1/transactions";
http.addHeader("apikey", config.getSupabaseKey());
```

---

## 📖 ConfigManager API 参考

### 初始化

```cpp
void init(const String& defaultSSID,
          const String& defaultPass,
          const String& defaultURL,
          const String& defaultKey,
          const String& defaultMachineID);
```

**说明**:
- 首次启动：保存默认值到 NVS
- 后续启动：从 NVS 加载配置
- 打印配置摘要到串口

**示例**:
```cpp
config.init("my_wifi", "my_password",
            "https://api.example.com", "api_key_123",
            "MACHINE_01");
```

---

### 读取配置

```cpp
String getWiFiSSID();        // 获取 WiFi SSID
String getWiFiPassword();    // 获取 WiFi 密码
String getSupabaseURL();     // 获取 Supabase URL
String getSupabaseKey();     // 获取 Supabase API Key
String getMachineID();       // 获取机器 ID
```

**示例**:
```cpp
String ssid = config.getWiFiSSID();
String url = config.getSupabaseURL();
```

---

### 更新配置

```cpp
void setWiFiCredentials(const String& ssid, const String& pass);
void setSupabaseConfig(const String& url, const String& key);
void setMachineID(const String& id);
```

**说明**:
- 立即更新内存中的值
- 同时保存到 NVS
- 无需重启即可生效

**示例**:
```cpp
// 更新 WiFi 配置
config.setWiFiCredentials("new_ssid", "new_password");

// 更新 Supabase 配置
config.setSupabaseConfig("https://new.supabase.co", "new_key");

// 更新机器 ID
config.setMachineID("MACHINE_02");
```

---

### 重置配置

```cpp
void resetToDefaults(const String& defaultSSID,
                     const String& defaultPass,
                     const String& defaultURL,
                     const String& defaultKey,
                     const String& defaultMachineID);
```

**说明**:
- 清除 NVS 中的所有配置
- 恢复为默认值

**示例**:
```cpp
config.resetToDefaults(DEFAULT_WIFI_SSID,
                       DEFAULT_WIFI_PASSWORD,
                       DEFAULT_SUPABASE_URL,
                       DEFAULT_SUPABASE_KEY,
                       MACHINE_ID);
```

---

### 检查初始化状态

```cpp
bool isInitialized();
```

**示例**:
```cpp
if (config.isInitialized()) {
    Serial.println("配置已加载");
}
```

---

## 🎯 实际使用场景

### 场景1: 首次部署

1. 在 config.h 中设置默认值
2. 上传代码到 ESP32
3. ESP32 启动时自动保存到 NVS
4. 串口输出：
   ```
   ⚙️ 首次启动，保存默认配置...
   ✅ 配置已加载:
      WiFi SSID: hanzg_hanyh
      Machine ID: VIP_TERMINAL_01
      Supabase URL: https://ttbtxxpnvkcbyugzdqfw...
   ```

---

### 场景2: 更换 WiFi

**不使用 ConfigManager**:
1. 修改 config.h
2. 重新编译
3. 上传代码
4. 等待上传完成（~30秒）

**使用 ConfigManager**:
1. 通过串口或网页发送命令：
   ```cpp
   config.setWiFiCredentials("new_ssid", "new_password");
   ```
2. 立即生效
3. 下次启动自动使用新配置

**节省时间**: ~29秒

---

### 场景3: 更换 API 密钥

**不使用 ConfigManager**:
1. Supabase 密钥过期
2. 修改 config.h
3. 重新编译上传
4. 停机时间：~5分钟

**使用 ConfigManager**:
1. 串口命令：
   ```cpp
   config.setSupabaseConfig(new_url, new_key);
   ```
2. 立即生效
3. 停机时间：~0秒

---

### 场景4: 批量部署

**不使用 ConfigManager**:
- 每台设备需要单独编译不同的 config.h
- 容易出错

**使用 ConfigManager**:
- 所有设备使用相同固件
- 首次启动后通过串口或网页配置
- 方便管理

---

## 🔒 安全性

### NVS 存储特点

1. **加密存储**: ESP32 的 NVS 支持 Flash 加密
2. **持久化**: 断电不丢失
3. **独立分区**: 与代码分区独立
4. **擦除保护**: 需要特殊命令才能擦除

### 安全建议

1. **生产环境**: 启用 Flash 加密
2. **默认值**: config.h 中使用假的默认值
3. **首次配置**: 通过安全通道（串口/HTTPS）设置真实值
4. **代码管理**: 不要将包含真实密码的 config.h 提交到 Git

---

## 🛠️ 故障排查

### 问题1: 配置不生效

**症状**: 修改配置后还是用旧的

**原因**: 没有调用 `setXXX()` 方法

**解决**:
```cpp
// ❌ 错误
config.wifiSSID = "new_ssid";  // 直接赋值无效

// ✅ 正确
config.setWiFiCredentials("new_ssid", "new_password");
```

---

### 问题2: 首次启动失败

**症状**: 串口输出配置错误

**原因**: NVS 未初始化

**解决**:
```cpp
void setup() {
    // 必须先初始化 NVS
    prefs.begin("goldsky", false);  // ✅ 正确

    // 再初始化 ConfigManager
    config.init(...);
}
```

---

### 问题3: 配置丢失

**症状**: 重启后配置恢复为默认值

**原因**: NVS 被清除或损坏

**解决**:
```bash
# 擦除 NVS 分区
esptool.py --port COM3 erase_region 0x9000 0x5000

# 重新上传代码
```

---

## 📊 对比总结

| 功能 | 明文配置 | ConfigManager |
|------|----------|---------------|
| 安全性 | ❌ 低 | ✅ 高 |
| 灵活性 | ❌ 需重新编译 | ✅ 运行时修改 |
| 部署速度 | ❌ 慢 | ✅ 快 |
| 代码复杂度 | ✅ 简单 | ❌ 稍复杂 |
| 适用场景 | 开发测试 | 生产环境 |

---

## 💡 建议

### 开发阶段
- 使用**明文配置**，快速迭代
- config.h 不要提交到 Git

### 生产部署
- 使用 **ConfigManager**
- 启用 Flash 加密
- 通过安全通道配置

---

## 🔗 相关文档

- [ConfigManager.h](ConfigManager.h) - 源代码
- [ESP32 NVS 文档](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/storage/nvs_flash.html)
- [Arduino Preferences 库](https://github.com/espressif/arduino-esp32/tree/master/libraries/Preferences)

---

**文档版本**: v1.0
**最后更新**: 2025-11-08
**作者**: Hanzg20
