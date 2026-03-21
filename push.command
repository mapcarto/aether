#!/bin/bash
export http_proxy=http://127.0.0.1:1082
export https_proxy=http://127.0.0.1:1082
cd /Users/dj/Downloads/Docs/md/Aether
git push -f origin main > push_result.log 2>&1
if [ $? -eq 0 ]; then
    echo "========= 推送到 GitHub 成功，请关闭此窗口 ========="
else
    echo "========= 推送失败，请检查以下报错 ========="
    cat push_result.log
    echo "\n\n(提示：如果不通，这通常是因为当前终端无法连接 github，或需要鉴权。)"
fi
sleep 15
