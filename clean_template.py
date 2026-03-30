import os
import re
from pathlib import Path

# 设置输入和输出文件夹
SOURCE_DIR = 'APP'
OUTPUT_DIR = 'APP_Lite'

def remove_comments_and_format(text, filename):
    # 1. 核心正则：匹配字符串(保留) 或 匹配C语言注释(删除)
    # 这样可以防止误删字符串内部的 // 或 /*
    pattern = r'(".*?"|\'.*?\')|(/\*.*?\*/|//[^\r\n]*)'
    regex = re.compile(pattern, re.MULTILINE | re.DOTALL)
    
    def _replacer(match):
        if match.group(2) is not None:
            return ""  # 捕获到注释，替换为空（删除）
        else:
            return match.group(1) # 捕获到字符串，原样返回

    clean_text = regex.sub(_replacer, text)
    
    # 2. 压缩多余的空行 (将连续的多个空行压缩为一个或两个，保持排版清爽)
    clean_text = re.sub(r'\n\s*\n', '\n\n', clean_text)
    
    # 3. 在文件最顶端添加 // 文件名
    final_text = f"// {filename}\n{clean_text.strip()}\n"
    return final_text

def main():
    src_path = Path(SOURCE_DIR)
    out_path = Path(OUTPUT_DIR)

    if not src_path.exists() or not src_path.is_dir():
        print(f"❌ 错误：找不到 [{SOURCE_DIR}] 文件夹！请确保脚本与 {SOURCE_DIR} 文件夹放在同一目录。")
        return

    print(f"🚀 开始极限压缩模板... 原文件极其安全，精简版将生成在 [{OUTPUT_DIR}] 中！\n")

    # 递归遍历 APP 文件夹下的所有文件
    file_count = 0
    for file in src_path.rglob('*'):
        if file.is_file() and file.suffix.lower() in ['.c', '.h']:
            # 读取文件内容 (自动兼容 Keil 的 GBK 编码和现代的 UTF-8 编码)
            try:
                with open(file, 'r', encoding='utf-8') as f:
                    content = f.read()
            except UnicodeDecodeError:
                with open(file, 'r', encoding='gbk', errors='ignore') as f:
                    content = f.read()

            # 执行删除注释和格式化
            new_content = remove_comments_and_format(content, file.name)

            # 计算输出路径，并在 APP_Lite 中自动创建对应的子文件夹 (如 Inc, Src)
            relative_path = file.relative_to(src_path)
            target_file = out_path / relative_path
            target_file.parent.mkdir(parents=True, exist_ok=True)

            # 写入精简后的代码
            with open(target_file, 'w', encoding='utf-8') as f:
                f.write(new_content)
            
            print(f"✅ 处理完成: {file.name}")
            file_count += 1

    print(f"\n🎉 搞定！共精简了 {file_count} 个文件。请去 [{OUTPUT_DIR}] 文件夹查看你的考场纯净版代码！")

if __name__ == '__main__':
    main()