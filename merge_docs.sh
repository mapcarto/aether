#!/bin/bash
TARGET="Aether_Full_Manual.md"
echo "正在物理剥离所有 YAML 前瞻信息并合并..."
> $TARGET

# 递归寻找 docs 下的所有 md 文件
find docs -name "*.md" | sort | while read -r file; do
    echo "正在清洗并合并: $file"
    echo "" >> $TARGET
    echo "<!-- Source: $file -->" >> $TARGET
    # 终极 AWK 逻辑：通过状态机，严格跳过两个 '---' 之间的所有行
    awk '
        BEGIN { skip=0; count=0 }
        /^---/ { 
            if (count < 2) {
                if (skip == 0) { skip=1; next }
                else if (skip == 1) { skip=0; count=2; next }
            }
        }
        { if (skip == 0) print }
    ' "$file" >> $TARGET
done
echo "✅ 原始大稿清理完成: $TARGET"
