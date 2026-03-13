# GitHub 操作流程指南

> ⚠️ **重要提醒**：每次推送到远程仓库之前，都必须先征得用户同意后方可执行。

本文档介绍如何使用 GitHub CLI (`gh`) 进行代码推送、PR 创建和代码合并。

## 准备工作

### 1. 安装 GitHub CLI

```bash
# macOS
brew install gh

# Linux (Ubuntu/Debian)
apt install gh

# 验证安装
gh --version
```

### 2. 认证 GitHub

#### 方式一：浏览器登录（推荐）
```bash
gh auth login
```

交互步骤：
1. **Where do you use GitHub?** → 选择 `GitHub.com`
2. **Protocol** → 选择 `HTTPS`
3. **Authenticate Git with your GitHub credentials?** → 输入 `Y`
4. **How would you like to authenticate?** → 选择 `Login with a web browser`
5. 复制一次性验证码（如 `63CE-8105`），在浏览器中完成验证

#### 方式二：使用 PAT Token
```bash
echo "你的PAT_TOKEN" | gh auth login --with-token
```

获取 Token：https://github.com/settings/tokens

#### 验证登录状态
```bash
gh auth status
```

## 推送流程

### 场景一：从零创建新仓库并推送

```bash
# 1. 初始化 Git 仓库（如果还没有）
git init

# 2. 创建 .gitignore 文件
cat > .gitignore << 'EOF'
.DS_Store
node_modules/
*.log
EOF

# 3. 添加文件并提交
git add .
git commit -m "Initial commit"

# 4. 创建 GitHub 仓库并推送
gh repo create my-repo --public --source=. --push --clone=false
```

### 场景二：推送到已有仓库

```bash
# 1. 添加远程仓库（如果还没有）
git remote add origin https://github.com/USERNAME/REPO.git

# 2. 设置默认仓库（可选）
gh repo set-default USERNAME/REPO

# 3. 推送到远程
git push -u origin main
```

### 场景三：使用 Token 认证推送

如果遇到认证问题，可以将 Token 嵌入 URL：

```bash
# 修改远程仓库 URL 包含 Token
git remote set-url origin https://ghp_TOKEN@github.com/USERNAME/REPO.git

# 然后推送
git push -u origin main
```

## Pull Request 流程

### 1. 创建分支并开发

```bash
# 创建新分支
git checkout -b feature/my-feature

# 开发完成后提交
git add .
git commit -m "Add my feature"
```

### 2. 推送分支并创建 PR

```bash
# 推送分支
git push -u origin feature/my-feature

# 创建 PR
gh pr create --title "My Feature" --body "## Description\n- Feature description\n\n## Testing\n- How to test"
```

### 3. 查看 PR 状态

```bash
# 查看所有 PR
gh pr status

# 查看 PR 详情
gh pr view 1

# 查看 PR 差异
gh pr diff 1
```

### 4. 代码审查

```bash
# 批准 PR
gh pr review 1 --approve --body "Looks good!"

# 请求更改
gh pr review 1 --request-changes --body "Please fix..."

# 评论 PR
gh pr review 1 --comment --body "Nice work!"
```

### 5. 合并 PR

```bash
# 合并 PR
gh pr merge 1

# 或者在 GitHub 网页上点击 "Merge pull request"
```

### 6. 清理

```bash
# 切换回主分支
git checkout main

# 拉取最新代码
git pull

# 删除已合并的分支
git branch -d feature/my-feature
```

## 常用命令速查

| 操作 | 命令 |
|------|------|
| 登录 | `gh auth login` |
| 登录状态 | `gh auth status` |
| 创建仓库 | `gh repo create NAME --public --source=. --push` |
| 列出仓库 | `gh repo list` |
| 创建 PR | `gh pr create --title "TITLE" --body "DESC"` |
| 查看 PR | `gh pr view 1` |
| 查看 PR 差异 | `gh pr diff 1` |
| 批准 PR | `gh pr review 1 --approve` |
| 合并 PR | `gh pr merge 1` |
| 查看状态 | `gh pr status` |

## 故障排除

### 问题：push 时提示需要登录

```bash
# 刷新 Token
gh auth refresh -h github.com -s repo
```

### 问题：无法读取 Username

```bash
# 使用 Token 方式认证
git remote set-url origin https://ghp_YOUR_TOKEN@github.com/USER/REPO.git
```

### 问题：无法批准自己的 PR

自己创建的 PR 无法自己在 CLI 中批准，需要：
- 在 GitHub 网页上合并
- 或让同事帮忙 review

## 参考链接

- [GitHub CLI 官方文档](https://cli.github.com/manual)
- [GitHub REST API](https://docs.github.com/en/rest)
