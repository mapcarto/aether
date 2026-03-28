#!/bin/bash
# 切换到 Aether 目录
cd /Users/dj/Downloads/Docs/md/Aether

OUTPUT_FILE="Aether_Full_Manual.md"
echo "正在合并文档..."

# 写入标题头
cat > "$OUTPUT_FILE" <<EOF
# Aether 时空内存引擎 - 完整技术白皮书

> 本文档由各分子文档合并生成，专供 NotebookLM / 大语言模型深度阅读与解析使用。

---

EOF

# 查找所有 md 文件并按顺序拼接
find docs -name "*.md" | sort | while read -r file; do
    echo "正在合并: $file"
    
    cat >> "$OUTPUT_FILE" <<EOF


<!-- ============================================== -->
<!-- 文档来源: $file -->
<!-- ============================================== -->

EOF
    cat "$file" >> "$OUTPUT_FILE"
    echo -e "\n\n" >> "$OUTPUT_FILE"
done

echo "✅ 合并完成！已更新: $OUTPUT_FILE"
