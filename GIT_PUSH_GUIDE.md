# Git推送指南

## 📦 已完成准备工作

✅ Git仓库已初始化
✅ 所有文件已提交 (73个文件)
✅ 远程仓库已添加: https://github.com/Hanzg20/GS-Touch.git
✅ 分支已重命名为 main
✅ 版本标签已创建: v0.5

---

## 🚀 推送到GitHub

### 方法1: 使用命令行（推荐）

```bash
# 进入项目目录
cd "d:\My Project\PCB_Project\GoldSky_Touch\misc_projects\GoldSky_Lite"

# 推送代码到main分支
git push -u origin main

# 推送版本标签
git push origin v0.5
```

### 方法2: 使用GitHub Desktop

1. 打开 GitHub Desktop
2. File → Add Local Repository
3. 选择目录: `d:\My Project\PCB_Project\GoldSky_Touch\misc_projects\GoldSky_Lite`
4. 点击 "Publish repository"
5. 确认仓库名称: `GS-Touch`
6. 点击 "Publish"

---

## 🔐 身份验证

### 如果使用HTTPS推送

GitHub已经停止密码认证，需要使用Personal Access Token (PAT):

1. **生成Token**
   - 访问: https://github.com/settings/tokens
   - 点击 "Generate new token (classic)"
   - 勾选权限: `repo` (完整仓库访问)
   - 生成并复制Token

2. **使用Token**
   ```bash
   # 推送时，用户名使用GitHub用户名
   # 密码使用刚生成的Token
   git push -u origin main
   ```

### 如果使用SSH推送

1. **生成SSH密钥**
   ```bash
   ssh-keygen -t ed25519 -C "your_email@example.com"
   ```

2. **添加SSH密钥到GitHub**
   - 复制公钥: `cat ~/.ssh/id_ed25519.pub`
   - 访问: https://github.com/settings/keys
   - 点击 "New SSH key"
   - 粘贴公钥内容

3. **更改远程仓库URL**
   ```bash
   git remote set-url origin git@github.com:Hanzg20/GS-Touch.git
   git push -u origin main
   ```

---

## 📋 推送检查清单

推送前确认：

- [ ] 代码已编译通过
- [ ] 测试已完成
- [ ] 敏感信息已移除（WiFi密码、API密钥）
- [ ] .gitignore 已配置
- [ ] README.md 已完善
- [ ] CHANGELOG.md 已更新

---

## 🔍 验证推送成功

推送后访问: https://github.com/Hanzg20/GS-Touch

应该看到：

- ✅ 73个文件
- ✅ README.md 显示在首页
- ✅ v0.5 标签在 Releases/Tags 中
- ✅ 最新提交信息显示

---

## 📊 提交信息摘要

```
提交ID: 79ee3af
分支: main
标签: v0.5
文件数: 73
行数: 30,702+
```

**提交内容**:
- 核心代码: GoldSky_Lite.ino, GoldSky_Display.ino, GoldSky_Utils.ino
- 配置文件: config.h, ConfigManager.h
- 文档: README.md, CHANGELOG.md, COMPILE_GUIDE.md
- 技术文档: docs/ (17个文件)
- 历史文档: doc/ (40+个文件)

---

## ⚠️ 注意事项

### 敏感信息检查

确保以下信息已移除或使用默认值：

```cpp
// config.h
#define DEFAULT_WIFI_SSID "YourWiFiSSID"  // ✅ 默认值
#define DEFAULT_WIFI_PASSWORD "YourPassword"  // ✅ 默认值
#define DEFAULT_SUPABASE_URL "https://your-project.supabase.co"  // ✅ 默认值
#define DEFAULT_SUPABASE_KEY "your-anon-key"  // ✅ 默认值
```

如果包含真实凭证，**不要推送**！先修改为默认值。

### 大文件检查

检查是否有超大文件：
```bash
find . -type f -size +1M
```

如果有超过100MB的文件，需要：
1. 添加到 .gitignore
2. 使用 Git LFS (Large File Storage)

---

## 🆘 常见问题

### Q: 推送失败 "rejected"
```bash
# 解决方法：先拉取远程更改
git pull origin main --rebase
git push -u origin main
```

### Q: 提示 "fatal: remote origin already exists"
```bash
# 解决方法：先删除再添加
git remote remove origin
git remote add origin https://github.com/Hanzg20/GS-Touch.git
```

### Q: 推送超时
```bash
# 解决方法：使用代理或SSH
git config --global http.proxy http://127.0.0.1:7890
# 或使用SSH
git remote set-url origin git@github.com:Hanzg20/GS-Touch.git
```

---

## 📝 后续操作

推送成功后：

1. **创建Release**
   - 访问: https://github.com/Hanzg20/GS-Touch/releases
   - 点击 "Create a new release"
   - 选择标签: v0.5
   - 填写发布说明（可复制CHANGELOG.md）
   - 上传编译好的.bin文件（可选）

2. **更新README徽章**
   ```markdown
   ![Version](https://img.shields.io/badge/version-0.5-blue)
   ![License](https://img.shields.io/badge/license-MIT-green)
   ![Arduino](https://img.shields.io/badge/Arduino-2.x-blue)
   ```

3. **设置GitHub Pages**（可选）
   - Settings → Pages
   - Source: Deploy from a branch
   - Branch: main, /docs

---

## 🎉 完成！

推送完成后，代码已安全备份到GitHub，可以：

- ✅ 团队协作开发
- ✅ 版本历史追踪
- ✅ Issue问题管理
- ✅ 代码审查
- ✅ 自动化CI/CD

---

**文档版本**: v1.0
**最后更新**: 2025-11-08
