# CuteCore Operating System Kernel

> 一个简洁、优雅的单核多架构通用操作系统内核

## 项目概述

CuteCore是一个现代化的操作系统内核，专注于提供简洁而高效的系统服务。项目采用模块化设计，支持多种硬件架构，当前主要支持RISC-V架构，同时具备扩展到其他架构（如LoongArch）的能力。

## 架构设计

### 目录结构

```
cuteCore/
├── os/                     # 内核源代码
│   ├── src/                # 核心内核实现
│   │   ├── arch/           # 架构相关代码
│   │   │   ├── riscv/      # RISC-V架构支持
│   │   │   └── loongarch/  # LoongArch架构支持
│   │   ├── driver/         # 设备驱动
│   │   ├── syscall/        # 系统调用实现
│   │   ├── task/           # 任务管理
│   │   └── utils/          # 工具函数
│   ├── scripts/            # 构建脚本
│   └── tools/              # 开发工具
├── user/                   # 用户态程序
│   ├── bin/                # 用户程序源代码
│   └── lib/                # 用户态库
├── include/                # 头文件
├── build/                  # 构建输出
└── docs/                   # 文档
```

### 核心组件

- **内核主程序** (`os/src/main.c`): 内核初始化和启动流程
- **架构抽象层** (`os/src/arch/`): 硬件架构相关的底层实现
- **系统调用层** (`os/src/syscall/`): 提供用户态与内核态的接口
- **任务管理器** (`os/src/task/`): 进程/任务的创建、调度和管理
- **设备驱动** (`os/src/driver/`): 硬件设备的驱动程序
- **工具库** (`os/src/utils/`): 内核工具函数和数据结构

## 快速开始

### 环境准备

确保系统中安装了以下工具：
- GCC或Clang编译器
- QEMU模拟器（用于RISC-V）
- Make构建工具
- Python 3

### 初始化环境

```bash
# 初始化开发环境
./init.sh

# 重新加载shell配置
source ~/.zshrc  # 或 source ~/.bashrc
```

### 配置内核

```bash
# 进入内核配置菜单
make menuconfig
```

### 编译内核

```bash
make kernel
```

### 运行内核

```bash
make qemu
```

## 开发指南

### 添加新的架构支持

1. 在 `os/src/arch/` 下创建新架构目录
2. 实现架构相关的启动代码、中断处理、内存管理等
3. 更新 `os/Kconfig` 添加配置选项
4. 更新构建脚本

### 添加新的系统调用

1. 在相应的 `sys_*.c` 文件中实现系统调用
2. 更新系统调用表
3. 更新用户态库的系统调用封装

### 添加新的驱动

1. 在 `os/src/driver/` 下创建驱动目录
2. 实现驱动接口
3. 在内核初始化中注册驱动

## 构建系统

项目使用多层构建系统：

- **Kconfig**: 内核配置管理
- **Make**: 主要构建工具
- **Python**: 辅助构建脚本 (`build.py`)

*CuteCore - 让操作系统内核开发变得更加简洁和优雅*