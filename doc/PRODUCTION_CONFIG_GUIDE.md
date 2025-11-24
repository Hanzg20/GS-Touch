# 生产环境配置指南

## 🎯 生产环境最佳实践

### 问题：第一次使用 ConfigManager 会从 config.h 读取吗？

**答案**：是的。

#### 启动流程

```
┌─────────────────────────────────────────┐
│ ESP32 启动                               │
└───────────┬─────────────────────────────┘
            │
            v
┌─────────────────────────────────────────┐
│ 检查 NVS 是否已初始化                    │
│ prefs->getBool("config_init")           │
└───────┬─────────────────────────────────┘
        │
        ├─── 首次启动 (false) ──────────┐
        │                               │
        │                               v
        │                    ┌──────────────────────┐
        │                    │ 从 config.h 读取      │
        │                    │ DEFAULT_WIFI_SSID    │
        │                    │ DEFAULT_WIFI_PASSWORD│
        │                    └──────────┬───────────┘
        │                               │
        │                               v
        │                    ┌──────────────────────┐
        │                    │ 保存到 NVS            │
        │                    │ prefs->putString()   │
        │                    └──────────┬───────────┘
        │                               │
        └─── 后续启动 (true) ───────────┤
                                        │
                                        v
                            ┌──────────────────────┐
                            │ 从 NVS 加载           │
                            │ prefs->getString()   │
                            └──────────────────────┘
```

---

### 问题：生产环境要删除明文吗？

**答案**：强烈建议删除。

#### ❌ 不安全的做法

**config.h**:
```cpp
// 真实密码硬编码
#define DEFAULT_WIFI_SSID "hanzg_hanyh"
#define DEFAULT_WIFI_PASSWORD "han1314521"
#define DEFAULT_SUPABASE_KEY "eyJhbGci..."  // 真实 API Key
```

**风险**:
- 代码泄露 → 密码泄露
- Git 提交 → 历史记录中有密码
- 所有设备使用相同密码

---

#### ✅ 安全的做法

**config.h**:
```cpp
// 使用占位符
#define DEFAULT_WIFI_SSID "SETUP_REQUIRED"
#define DEFAULT_WIFI_PASSWORD "SETUP_REQUIRED"
#define DEFAULT_SUPABASE_URL "https://SETUP_REQUIRED.supabase.co"
#define DEFAULT_SUPABASE_KEY "SETUP_REQUIRED"
```

**优点**:
- 代码安全，可以提交到 Git
- 每台设备独立配置
- 密码泄露只影响单台设备

---

## 🔧 生产环境部署方案

### 方案1: 串口配置（推荐）

#### 步骤1: 修改 config.h

```cpp
// config.h
// 生产环境默认值（占位符）
#define DEFAULT_WIFI_SSID "SETUP_REQUIRED"
#define DEFAULT_WIFI_PASSWORD "SETUP_REQUIRED"
#define DEFAULT_SUPABASE_URL "https://SETUP_REQUIRED.supabase.co"
#define DEFAULT_SUPABASE_KEY "SETUP_REQUIRED"
```

#### 步骤2: 添加串口配置功能

在主程序中添加：

```cpp
// 全局变量
Preferences prefs;
ConfigManager config(&prefs);

void handleSerialConfig() {
    if (Serial.available()) {
        String cmd = Serial.readStringUntil('\n');
        cmd.trim();

        if (cmd.startsWith("SET_WIFI,")) {
            // 格式: SET_WIFI,ssid,password
            int idx1 = cmd.indexOf(',');
            int idx2 = cmd.indexOf(',', idx1 + 1);

            String ssid = cmd.substring(idx1 + 1, idx2);
            String pass = cmd.substring(idx2 + 1);

            config.setWiFiCredentials(ssid, pass);
            Serial.println("✅ WiFi 配置已保存");
            Serial.println("SSID: " + ssid);
        }
        else if (cmd.startsWith("SET_SUPABASE,")) {
            // 格式: SET_SUPABASE,url,key
            int idx1 = cmd.indexOf(',');
            int idx2 = cmd.indexOf(',', idx1 + 1);

            String url = cmd.substring(idx1 + 1, idx2);
            String key = cmd.substring(idx2 + 1);

            config.setSupabaseConfig(url, key);
            Serial.println("✅ Supabase 配置已保存");
        }
        else if (cmd == "SHOW_CONFIG") {
            Serial.println("当前配置:");
            Serial.println("  SSID: " + config.getWiFiSSID());
            Serial.println("  URL: " + config.getSupabaseURL());
        }
        else if (cmd == "RESTART") {
            ESP.restart();
        }
    }
}

void setup() {
    Serial.begin(115200);

    // 初始化 NVS
    prefs.begin("goldsky", false);

    // 初始化 ConfigManager
    config.init(DEFAULT_WIFI_SSID,
                DEFAULT_WIFI_PASSWORD,
                DEFAULT_SUPABASE_URL,
                DEFAULT_SUPABASE_KEY,
                MACHINE_ID);

    // 检查是否需要配置
    if (config.getWiFiSSID() == "SETUP_REQUIRED") {
        Serial.println("╔════════════════════════════════════╗");
        Serial.println("║  ⚠️  首次启动 - 需要配置          ║");
        Serial.println("╚════════════════════════════════════╝");
        Serial.println();
        Serial.println("请发送以下命令:");
        Serial.println("1. 配置WiFi:");
        Serial.println("   SET_WIFI,your_ssid,your_password");
        Serial.println();
        Serial.println("2. 配置Supabase:");
        Serial.println("   SET_SUPABASE,https://xxx.supabase.co,your_api_key");
        Serial.println();
        Serial.println("3. 查看配置:");
        Serial.println("   SHOW_CONFIG");
        Serial.println();
        Serial.println("4. 重启设备:");
        Serial.println("   RESTART");
        Serial.println();

        // 等待配置完成
        while (config.getWiFiSSID() == "SETUP_REQUIRED") {
            handleSerialConfig();
            delay(100);
        }

        Serial.println("✅ 配置完成，正在启动...");
    }

    // 继续正常启动流程...
}

void loop() {
    // 运行时也支持配置更新
    handleSerialConfig();

    // 其他业务逻辑...
}
```

#### 步骤3: 首次部署操作

1. **编译上传固件**
   ```bash
   # Arduino IDE: 点击上传
   # 或使用命令行
   arduino-cli compile --fqbn esp32:esp32:esp32s3
   arduino-cli upload -p COM3 --fqbn esp32:esp32:esp32s3
   ```

2. **打开串口监视器**
   ```
   工具 → 串口监视器
   波特率: 115200
   ```

3. **查看提示信息**
   ```
   ╔════════════════════════════════════╗
   ║  ⚠️  首次启动 - 需要配置          ║
   ╚════════════════════════════════════╝

   请发送以下命令:
   1. 配置WiFi:
      SET_WIFI,your_ssid,your_password
   ...
   ```

4. **发送配置命令**
   ```
   SET_WIFI,hanzg_hanyh,han1314521
   ```

   串口返回:
   ```
   ✅ WiFi 配置已保存
   SSID: hanzg_hanyh
   ```

5. **配置 Supabase**
   ```
   SET_SUPABASE,https://ttbtxxpnvkcbyugzdqfw.supabase.co,eyJhbGci...
   ```

   串口返回:
   ```
   ✅ Supabase 配置已保存
   ```

6. **验证配置**
   ```
   SHOW_CONFIG
   ```

   串口返回:
   ```
   当前配置:
     SSID: hanzg_hanyh
     URL: https://ttbtxxpnvkcbyugzdqfw.supabase.co
   ```

7. **重启设备**
   ```
   RESTART
   ```

8. **配置完成**
   - 设备重启
   - 自动从 NVS 加载配置
   - 连接 WiFi
   - 正常运行

---

### 方案2: 预烧录配置（适合批量部署）

#### 步骤1: 创建配置工具

**upload_config.py**:
```python
#!/usr/bin/env python3
import serial
import time
import sys

def upload_config(port, ssid, password, supa_url, supa_key):
    ser = serial.Serial(port, 115200, timeout=1)
    time.sleep(2)  # 等待连接

    # 发送配置
    ser.write(f"SET_WIFI,{ssid},{password}\n".encode())
    time.sleep(0.5)

    ser.write(f"SET_SUPABASE,{supa_url},{supa_key}\n".encode())
    time.sleep(0.5)

    # 验证
    ser.write(b"SHOW_CONFIG\n")
    time.sleep(0.5)

    # 读取响应
    while ser.in_waiting:
        print(ser.readline().decode(), end='')

    # 重启
    ser.write(b"RESTART\n")
    ser.close()

    print("✅ 配置上传完成")

if __name__ == "__main__":
    if len(sys.argv) != 6:
        print("用法: python upload_config.py <端口> <SSID> <密码> <URL> <Key>")
        sys.exit(1)

    upload_config(sys.argv[1], sys.argv[2], sys.argv[3],
                  sys.argv[4], sys.argv[5])
```

#### 步骤2: 使用脚本

```bash
python upload_config.py COM3 hanzg_hanyh han1314521 \
    https://ttbtxxpnvkcbyugzdqfw.supabase.co \
    eyJhbGci...
```

#### 步骤3: 批量部署

```bash
# 设备1
python upload_config.py COM3 wifi1 pass1 url1 key1

# 设备2
python upload_config.py COM4 wifi2 pass2 url2 key2

# 设备3
python upload_config.py COM5 wifi3 pass3 url3 key3
```

---

## 🔒 安全检查清单

### 代码安全

- [ ] config.h 中没有真实密码
- [ ] 默认值使用 "SETUP_REQUIRED" 或类似占位符
- [ ] Git 中没有包含真实密码的提交
- [ ] .gitignore 包含可能的配置文件

### 部署安全

- [ ] 首次配置通过串口完成
- [ ] 配置保存到加密的 NVS
- [ ] 配置完成后关闭串口或禁用配置命令
- [ ] 记录每台设备的配置（设备ID → 配置映射）

### 运行时安全

- [ ] 启用 Flash 加密（ESP32 功能）
- [ ] 禁用 JTAG 调试接口
- [ ] 定期更换 API 密钥
- [ ] 监控异常访问

---

## 📋 对比总结

| 方面 | 开发环境 | 生产环境 |
|------|----------|----------|
| config.h | 真实密码 | 占位符 |
| 首次配置 | 自动 | 手动/脚本 |
| 配置方式 | 硬编码 | 串口/工具 |
| 安全性 | 低 | 高 |
| Git 提交 | 不要提交 | 可以提交 |
| 部署速度 | 快 | 稍慢 |

---

## 🎯 推荐方案

### 开发阶段
```cpp
// config.h
#define DEFAULT_WIFI_SSID "hanzg_hanyh"      // 真实密码
#define DEFAULT_WIFI_PASSWORD "han1314521"   // 真实密码
```
- 快速迭代
- 不提交到 Git

### 生产部署
```cpp
// config.h
#define DEFAULT_WIFI_SSID "SETUP_REQUIRED"
#define DEFAULT_WIFI_PASSWORD "SETUP_REQUIRED"
```
- 首次启动通过串口配置
- 配置永久保存到 NVS
- 代码可以安全提交到 Git

---

## ❓ 常见问题

### Q: 如果忘记配置怎么办？

**A**: 擦除 NVS 分区重新配置

```bash
# 方法1: 使用 esptool
esptool.py --port COM3 erase_region 0x9000 0x5000

# 方法2: 串口命令（需要先添加）
# 发送: RESET_CONFIG
```

### Q: 如何更新已部署设备的配置？

**A**: 通过串口发送新配置

```
SET_WIFI,new_ssid,new_password
RESTART
```

### Q: 批量部署如何提高效率？

**A**: 使用配置工具脚本
- 准备配置文件 (CSV/JSON)
- 自动化脚本批量上传
- 记录部署日志

---

**文档版本**: v1.0
**最后更新**: 2025-11-08
**作者**: Hanzg20
