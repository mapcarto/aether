#!/bin/bash
export https_proxy=http://127.0.0.1:1082
export http_proxy=http://127.0.0.1:1082

cd /Users/dj/Downloads/Docs/md/Aether

echo "=========================================================="
echo "          Aether 引擎仓库分支合并与清理程序               "
echo "=========================================================="
echo ""

# 1. 强制推送本地 main（由于本地 main 已经含有刚建好的所有文档骨架，直接上推主线即可）
echo ">>> [1/2] 正在向远端的主干 main 分支投递并覆盖完整的架构文档..."
git push -f origin main

# 2. 删除之前由第三方工具误创建的干扰分支
echo ">>> [2/2] 正在销毁远端的闲散分支 copilot/high-performance-event-engine..."
git push origin --delete copilot/high-performance-event-engine

echo ""
echo "=========================================================="
echo "✅ 搞定！您的远端已独尊 main 分支，张亮现在看到的是最整洁的代码结构。"
echo "此窗口将在 10 秒后自动散去..."
echo "=========================================================="
sleep 10
