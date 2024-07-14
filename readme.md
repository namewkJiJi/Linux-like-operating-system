# build your own linux in c



## Version 1.0

### 项目介绍

- 一个模仿 Linux 0.11 的操作系统，通过命令行与用户进行交互。一款基于intel x86 cpu芯片的32位操作系统，在linux平台模拟运行
- 编程语言：大部分为c + 少部分x86汇编
- 编译链接：cmake + gcc + gdb
- 开发环境：ubuntu + vscode + qemu
- 基本功能
  - 支持多进程运行
  - 支持shell加载磁盘上应用程序运行
  - 支持虚拟内存管理，实现进程之间的隔离
  - 键盘和显示器的支持
  - 引用标准C库，使得应用程序开发更加方便
  - 十余个系统调用：fork()、execve()、open()、write()、exit()等
  - 进程与操作系统不同特权级分离

### 目录组织形式

- src/img 磁盘影像文件
- src/start 程序代码



- start/newlib 开源的c库实现
- start/os_code 操作系统主要代码



- .vscode
    用于存放项目配置和工具相关文件
- build
    用于存放构建任务生成的文件
- doc
    用于存放说明文档
- script
    用于存放bash脚本文件
- source
    用于存放操作系统的核心代码
- CMakeLists.txt
    用于配置编译流程

### source/comm

用于存放头文件，这些头文件包括整个操作系统能够公用的一些代

### source/boot

系统引导部分，启动时，bios将该部分代码从硬盘加载到内存，然后完成对二级引导程序loader的加载

### source/loader

BIOS加载的代码长度有限，故进行二级引导，负责进行硬件检测，进入保护模式，然后加载内核，并跳转至内核运行

### source/kernel

内核代码，内含操作系统核心代码

### kernel/include

内含kernel部分的所有头文件，在include中进行分组管理

### kernel/init

- start.S
    内核代码的入口
- init.c
    内核的初始化代码
- first_task
    内核的第一个进程，在进程中会调用fork/exec指令创建更多进程


### kernel/fs

文件系统相关的接口实现

### kernel/tools

内核的一些功能函数

### kernel/ipc

interprocess communication 用于实现进程间相互通信的功能

### kernel/dev

管理计算机系统的外部设备，包括定时器，键盘与控制台等。

### kernel/cpu

涉及cpu细节的相关数据结构与相关操作，与cpu的设计关联较紧密


### kernel/core

操作系统的核心代码，用于内存管理，系统调用，任务管理

### core/memory

内存管理：分页机制


### core/syscall

系统调用，实际上就是引发一个异常，通过异常处理函数跳转到指定的代码区域，在跳转过程在完成模式转换


### core/task

任务管理

### source/applib

为操作系统支持的应用程序（如shell）提供c语言接口

### source/loop  

应用程序，在用户模式下运行，循环对输入的字符串进行输出


### source/init

在用户模式下运行，测试来自用户的中断对操作系统的影像


### source/shell

shell程序，提供用户与操作系统的接口



