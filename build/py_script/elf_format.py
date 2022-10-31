#!/usr/bin/python
# -*- coding:utf-8 -*-

import os
import sys
import getopt

format_tool = ""
elf_path = ""
bin_path = ""
hex_path = ""
dump_path = ""
lib_path = ""
out_dir = ""
flag = ""
elf_flag = ""
lib_lds = ""
 
#获取命令行参数
try:
    opts, args = getopt.getopt(sys.argv[1:],"",["format_tool=","elf=","bin=","hex=","dump=","lib=","dir=","flag=","elf_flag=","lib_lds="])
except getopt.GetoptError:
    print("elf_format.py --format_tool <tool_path> --elf <elf_path> --bin <bin_path> --hex <hex_path> --dump <dump_path> --lib <lib_path> --dir <out_dir> --flag <b or h or d or l> --elf_flag <> --lib_lds<lib_lds>")
    sys.exit(2)
for opt, val in opts:
    if opt in ("--format_tool"):
        format_tool = val
    if opt in ("--elf"):
        elf_path = val
    if opt in ("--bin"):
        bin_path = val
    if opt in ("--hex"):
        hex_path = val
    if opt in ("--dump"):
        dump_path = val
    if opt in ("--lib"):
        lib_path = val
    if opt in ("--dir"):
        out_dir = val
    if opt in ("--flag"):
        flag = val
    if opt in ("--elf_flag"):
        elf_flag = val
    if opt in ("--lib_lds"):
        lib_lds = val

#print(format_tool)
#print(elf_path)
#print(bin_path)
#print(hex_path)
#print(dump_path)
#print(lib_path)
#print(out_dir)
#print(flag)
 
objcopy = format_tool+"objcopy"
objdump = format_tool+"objdump"
ld = format_tool+"ld"

#生成bin文件
if "b" in flag:
    os.system(objcopy+" -O binary "+elf_path+" "+bin_path)
    
#生成hex文件
if "h" in flag:
    os.system(objcopy+" -O ihex "+elf_path+" "+hex_path)
    
#生成dump文件
if "d" in flag:
    os.system(objdump+" -D -Slx "+elf_path+" > "+dump_path)
    
#生成lib文件
if "l" in flag:
    os.system(ld+" --oformat "+elf_flag+" -T "+lib_lds+" -r -b binary -o "+lib_path+" "+bin_path)