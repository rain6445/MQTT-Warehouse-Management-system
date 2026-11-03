#!/usr/bin/env python3
"""
简单的ESP32项目编译脚本
"""
import os
import sys
import shutil
import subprocess

def main():
    print("开始编译ESP32项目...")
    
    # 检查当前目录
    current_dir = os.getcwd()
    print(f"当前目录: {current_dir}")
    
    # 检查必要的文件
    required_files = ["CMakeLists.txt", "main/CMakeLists.txt", "main/2340711139main.c"]
    for file in required_files:
        if not os.path.exists(file):
            print(f"错误: 找不到文件 {file}")
            return 1
    
    # 删除旧的build目录
    if os.path.exists("build"):
        print("删除旧的build目录...")
        shutil.rmtree("build")
    
    # 创建build目录
    os.makedirs("build", exist_ok=True)
    
    # 尝试使用cmake配置项目
    print("配置CMake...")
    cmake_result = subprocess.run(["cmake", "-S", ".", "-B", "build"], 
                                 capture_output=True, text=True)
    
    if cmake_result.returncode != 0:
        print("CMake配置失败:")
        print(cmake_result.stderr)
        return 1
    
    print("CMake配置成功")
    
    # 使用make编译项目
    print("编译项目...")
    make_result = subprocess.run(["make", "-C", "build"], 
                                capture_output=True, text=True)
    
    if make_result.returncode != 0:
        print("编译失败:")
        print(make_result.stderr)
        return 1
    
    print("编译成功!")
    
    # 检查生成的固件文件
    firmware_file = "build/2340711139qx.bin"
    if os.path.exists(firmware_file):
        file_size = os.path.getsize(firmware_file)
        print(f"固件文件已生成: {firmware_file} ({file_size} 字节)")
    else:
        print("警告: 未找到固件文件")
    
    return 0

if __name__ == "__main__":
    sys.exit(main())