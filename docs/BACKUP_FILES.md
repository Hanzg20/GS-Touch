# 📦 备份文件说明

所有备份文件位于 **[doc/](doc/)** 目录下。

---

## 📂 备份文件列表

### 主目录备份
当前主目录 **没有备份文件**，所有备份已移至 `doc/` 目录。

---

### doc/ 目录备份

| 文件名 | 大小 | 日期 | 说明 |
|--------|------|------|------|
| **[GoldSky_Lite.ino.bak](doc/GoldSky_Lite.ino.bak)** | 31KB | 2025-10-30 08:52 | 最早期备份 |
| **[GoldSky_Lite.ino.v5.2.bak](doc/GoldSky_Lite.ino.v5.2.bak)** | 33KB | 2025-10-30 09:53 | v5.2版本备份 |
| **[GoldSky_Lite.ino.v23.bak](doc/GoldSky_Lite.ino.v23.bak)** | 35KB | 2025-10-30 14:34 | v2.3版本（实施方案A前） |
| **[GoldSky_Lite.ino.v24.bak](doc/GoldSky_Lite.ino.v24.bak)** | 37KB | 2025-10-30 15:07 | v2.4版本（方案A优化后） |
| **[GoldSky_Lite.ino.old](doc/GoldSky_Lite.ino.old)** | 37KB | 2025-10-30 14:52 | 模块化重构前的完整版本 |

---

## 📋 版本历史

### 时间线

```
08:52 → GoldSky_Lite.ino.bak          (最早备份)
09:53 → GoldSky_Lite.ino.v5.2.bak     (v5.2版本)
14:34 → GoldSky_Lite.ino.v23.bak      (v2.3版本)
14:52 → GoldSky_Lite.ino.old          (重构前最后版本)
15:07 → GoldSky_Lite.ino.v24.bak      (v2.4方案A优化)
15:12 → 模块化重构完成 (当前版本)
```

---

## 🔍 备份文件详解

### 1. GoldSky_Lite.ino.bak (31KB)
**创建时间：** 2025-10-30 08:52
**内容：** 最早期的代码版本
**用途：** 历史参考，可能是项目初始版本

---

### 2. GoldSky_Lite.ino.v5.2.bak (33KB)
**创建时间：** 2025-10-30 09:53
**内容：** v5.2版本代码
**特点：**
- 8状态机架构
- 3个混用LED
- 较早的代码结构

---

### 3. GoldSky_Lite.ino.v23.bak (35KB)
**创建时间：** 2025-10-30 14:34
**内容：** v2.3修复版（方案A实施前）
**特点：**
- ✅ 6状态机（简化流程）
- ✅ 4个独立LED
- ✅ VIP查询功能
- ✅ 脉冲逻辑修复
- ❌ 无方案A优化（错误恢复、内存保护）

**用途：** 如果方案A优化有问题，可以回退到此版本

---

### 4. GoldSky_Lite.ino.v24.bak (37KB)
**创建时间：** 2025-10-30 15:07
**内容：** v2.4版本（方案A优化后，模块化重构前）
**特点：**
- ✅ v2.3的所有功能
- ✅ 方案A优化1：错误自动恢复（5分钟超时、连续错误重置）
- ✅ 方案A优化2：内存保护（<20KB重启、<40KB预警）
- ❌ 未模块化（单一大文件1340行）

**用途：**
- 如果模块化重构有问题，可以回退到此版本
- 这是最稳定的单文件版本

---

### 5. GoldSky_Lite.ino.old (37KB)
**创建时间：** 2025-10-30 14:52
**内容：** 模块化重构前的最后完整版本
**特点：** 与 v24.bak 内容相同或非常接近

**用途：** 模块化重构过程中的临时备份

---

## 🔄 如何恢复备份

### 恢复到某个版本

**步骤：**

1. **备份当前版本**
   ```bash
   cd "D:\My Project\PCB_Project\GoldSky_Touch\misc_projects\GoldSky_Lite"
   cp GoldSky_Lite.ino GoldSky_Lite.ino.current
   ```

2. **恢复目标版本**（以v2.4为例）
   ```bash
   cp doc/GoldSky_Lite.ino.v24.bak GoldSky_Lite.ino
   ```

3. **编译验证**
   - Arduino IDE → Verify/Compile

4. **上传测试**
   - Arduino IDE → Upload

---

## 📊 版本对比

| 版本 | 状态机 | LED | VIP查询 | 方案A优化 | 模块化 | 推荐度 |
|------|--------|-----|---------|----------|--------|--------|
| **v5.2** | 8状态 | 3混用 | ❌ | ❌ | ❌ | ⭐⭐ 历史版本 |
| **v2.3** | 6状态 | 4独立 | ✅ | ❌ | ❌ | ⭐⭐⭐ 基础稳定 |
| **v2.4** | 6状态 | 4独立 | ✅ | ✅ | ❌ | ⭐⭐⭐⭐ 优化稳定 |
| **当前** | 6状态 | 4独立 | ✅ | ✅ | ✅ | ⭐⭐⭐⭐⭐ 最佳 |

---

## 💡 推荐的备份策略

### 当前文件结构
```
GoldSky_Lite/
├── config.h                    ← 当前运行版本
├── GoldSky_Utils.ino
├── GoldSky_Display.ino
├── GoldSky_Lite.ino
├── doc/
│   ├── GoldSky_Lite.ino.bak       ← 历史备份
│   ├── GoldSky_Lite.ino.v5.2.bak
│   ├── GoldSky_Lite.ino.v23.bak   ← 方案A前版本
│   ├── GoldSky_Lite.ino.v24.bak   ← 重构前完整版本 ⭐
│   └── GoldSky_Lite.ino.old
└── 其他文件...
```

### 建议保留的关键备份

**必须保留：**
- ✅ **v24.bak** - 重构前最稳定版本（单文件完整功能）
- ✅ **v23.bak** - 方案A优化前版本（无错误恢复）

**可选保留：**
- ⚠️ **v5.2.bak** - 历史参考（不同架构）
- ⚠️ **old** - 与v24.bak重复

**可以删除：**
- ❌ **bak** - 最早期版本（功能不完整）

---

## 🗂️ 备份清理建议

### 精简备份（推荐）

保留2个关键备份即可：

```bash
cd "D:\My Project\PCB_Project\GoldSky_Touch\misc_projects\GoldSky_Lite\doc"

# 保留这两个：
# - GoldSky_Lite.ino.v23.bak (方案A前)
# - GoldSky_Lite.ino.v24.bak (重构前)

# 可选删除：
rm GoldSky_Lite.ino.bak          # 最早期版本
rm GoldSky_Lite.ino.v5.2.bak     # v5.2历史版本
rm GoldSky_Lite.ino.old          # 与v24.bak重复
```

---

## 🔐 重要备份位置

### 模块化重构后的完整备份

**当前运行版本（模块化）：**
```
GoldSky_Lite/
├── config.h           (193行)
├── GoldSky_Utils.ino  (230行)
├── GoldSky_Display.ino (230行)
└── GoldSky_Lite.ino   (640行)
```

**如需备份整个项目：**
```bash
cd "D:\My Project\PCB_Project\GoldSky_Touch\misc_projects"
tar -czf GoldSky_Lite_v2.4.1_modular_$(date +%Y%m%d).tar.gz GoldSky_Lite/
```

或者复制整个目录：
```bash
cp -r GoldSky_Lite GoldSky_Lite_v2.4.1_backup
```

---

## 📞 紧急恢复

### 如果当前版本出现问题

**快速回退到v2.4（重构前完整版本）：**

```bash
cd "D:\My Project\PCB_Project\GoldSky_Touch\misc_projects\GoldSky_Lite"

# 1. 备份当前模块化版本
mkdir backup_modular
cp *.h *.ino backup_modular/

# 2. 恢复v2.4单文件版本
cp doc/GoldSky_Lite.ino.v24.bak GoldSky_Lite.ino

# 3. 删除模块化文件（避免冲突）
rm GoldSky_Utils.ino
rm GoldSky_Display.ino
rm config.h

# 4. 编译上传
# Arduino IDE → Verify → Upload
```

---

## 📝 备份文件检查清单

在删除任何备份前，请确认：

- [ ] 当前版本已编译成功
- [ ] 当前版本功能测试通过
- [ ] 至少保留1个稳定版本备份（推荐v24.bak）
- [ ] 重要修改前创建新备份
- [ ] 备份文件已标注清晰的版本号和日期

---

## 🎯 总结

### 关键备份文件

**最重要：**
- 📌 **[doc/GoldSky_Lite.ino.v24.bak](doc/GoldSky_Lite.ino.v24.bak)** (37KB)
  - v2.4版本（方案A优化）
  - 重构前的完整单文件版本
  - 如果模块化有问题可以回退到此版本

**次要：**
- 📌 **[doc/GoldSky_Lite.ino.v23.bak](doc/GoldSky_Lite.ino.v23.bak)** (35KB)
  - v2.3版本（无方案A优化）
  - 如果方案A优化有问题可以回退到此版本

### 备份位置
- ✅ **所有备份都在 `doc/` 目录下**
- ✅ 主目录保持干净，只有当前运行的文件

---

**最后更新：** 2025-10-30
**当前版本：** v2.4.1 (模块化重构版 + 自动重启修复)
