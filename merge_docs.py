import os
import glob

def merge_markdown_files(source_dir, output_file):
    print(f"开始拼合文档，源目录: {source_dir}")
    
    # 查找所有 md 文件并按名称排序（这保证了 01_xx, 02_xx 会被顺序拼接）
    search_pattern = os.path.join(source_dir, '**', '*.md')
    md_files = sorted(glob.glob(search_pattern, recursive=True))
    
    if not md_files:
        print("未找到任何 Markdown 文件！")
        return

    with open(output_file, 'w', encoding='utf-8') as outfile:
        # 写入整个聚合文档的标题头
        outfile.write("# Aether 时空内存引擎 - 完整技术白皮书\n\n")
        outfile.write("> 本文档由各分子文档合并生成，专供 NotebookLM / 大语言模型深度阅读与解析使用。\n\n")
        outfile.write("---\n\n")
        
        for file_path in md_files:
            print(f"正在合并: {file_path}")
            # 拿到相对路径，方便作为注释
            rel_path = os.path.relpath(file_path, start=os.path.dirname(source_dir))
            
            # 添加文件分割与提示
            outfile.write(f"\n\n<!-- ============================================== -->\n")
            outfile.write(f"<!-- 文档来源: {rel_path} -->\n")
            outfile.write(f"<!-- ============================================== -->\n\n")
            
            with open(file_path, 'r', encoding='utf-8') as infile:
                content = infile.read()
                outfile.write(content)
                outfile.write("\n\n")
                
    print(f"\n✅ 合并完成！完整文档已生成至: {output_file}")
    print(f"总计合并了 {len(md_files)} 个文件。可以直接将其导入到 NotebookLM 中。")

if __name__ == '__main__':
    # 获取当前脚本所在目录
    base_dir = os.path.dirname(os.path.abspath(__file__))
    docs_dir = os.path.join(base_dir, 'docs')
    output_path = os.path.join(base_dir, 'Aether_Full_Manual.md')
    
    merge_markdown_files(docs_dir, output_path)
