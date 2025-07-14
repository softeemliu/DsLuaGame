# -*- coding: utf-8 -*-
import os
import codecs
import chardet
import argparse
import sys

def convert_file_to_utf8(file_path):
    """将单个文件转换为UTF-8编码"""
    try:
        # 检测原始编码
        with open(file_path, 'rb') as f:
            raw_data = f.read()
            if not raw_data:  # 空文件
                return
            
            # 检测文件编码
            result = chardet.detect(raw_data)
            encoding = result['encoding']
            confidence = result['confidence']
            
            # 如果无法检测编码或置信度低，使用默认编码
            if not encoding or confidence < 0.7:
                encoding = 'iso-8859-1'  # 回退编码
            
        # 读取文件内容并转换为UTF-8
        with codecs.open(file_path, 'r', encoding) as f:
            content = f.read()
        
        # 写入UTF-8编码的文件
        with codecs.open(file_path, 'w', 'utf-8') as f:
            f.write(content)
            
        print(u"转换成功: {} (原始编码: {}, 置信度: {:.2f}%)".format(
            file_path, encoding, confidence * 100))
        
    except Exception as e:
        print(u"转换失败: {} - {}".format(file_path, str(e)))

def convert_directory(directory):
    """递归遍历目录并转换所有文件"""
    for root, dirs, files in os.walk(directory):
        for file_name in files:
            file_path = os.path.join(root, file_name)
            convert_file_to_utf8(file_path)

def main():
    parser = argparse.ArgumentParser(description='将目录中所有文件的编码转换为UTF-8')
    parser.add_argument('directory', help='要处理的目录路径')
    args = parser.parse_args()
    
    if not os.path.isdir(args.directory):
        print(u"错误: 指定的路径不是目录")
        sys.exit(1)
    
    print(u"开始转换目录: {}".format(args.directory))
    convert_directory(os.path.abspath(args.directory))
    print(u"\n所有文件转换完成!")

if __name__ == '__main__':
    main()