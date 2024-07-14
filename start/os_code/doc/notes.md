### 项目介绍

- 一个模仿 Linux 0.11 的操作系统，通过命令行与用户进行交互。一款基于intel x86 cpu芯片的32位操作系统，支持多任务（进程）,
- 编程语言：大部分为c + 少部分x86汇编
- 编译链接：cmake + gcc + gdb
- 开发环境：ubuntu + vscode + qemu
- xiang mu gong neng
  - 支持多进程运行
  - 支持shell加载磁盘上应用程序运行
  - 支持虚拟内存管理，实现进程之间的隔离
  - 键盘和显示器的支持
  - 引用标准C库，使得应用程序开发更加方便
  - 十余个系统调用：fork()、execve()、open()、write()、exit()等
  - 进程与操作系统不同特权级分离


### 目录组织形式

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

用于存放头文件，这些头文件包括整个操作系统能够公用的一些代码

- boot_info.h
    存放启动信息相关的数据结构和定义，主要是对内存的记录
- cpu_instr.h
    对汇编指令的封装，避免大量的汇编代码掺杂在c中，也利于更新迭代
- elf.h
    与elf解析相关的数据结构和定义
- types.h
    定义了特定大小的数据类型


### source/boot

系统引导部分，启动时，bios将该部分代码从硬盘加载到内存，然后完成对二级引导程序loader的加载

- 从计算机启动到操作系统：16位实模式 -> BIOS -> 引导代码 -> 操作系统

- start.S
    整个操作系统的入口代码，该部分代码做一些简单的初始化工作并将loader从磁盘加载到内存
- boot.c
    引导部分的c语言代码，由汇编进入c语言，引导计算机进入loader，再由loader进入操作系统内核
    - `boot_entry` : 根据地址，跳转到loader处
- boot.h

### source/loader

BIOS加载的代码长度有限，故进行二级引导，负责进行硬件检测，进入保护模式，然后加载内核，并跳转至内核运行

### loader/start.S 两个部分

- 16位部分：跳转到loader_entry,进入c语言环境
- 32位部分：重置段寄存器，进入32位c语言环境

### loader/loader_16.c

16位模式下的引导代码，进行显示字符，探测内存，进入保护模式（32位）

### loader/loader_32.c

- `read_disk` : 将指定磁盘区域加载到内存的指定位置（缓存），此时加载的是kernel生成的elf文件
- `reload_elf_file` ： 解析缓存中的所有elf文件，将其解析（分成代码段与数据段）并装载到合适的内存区域
- `enable_page_mode` : 开启分页机制
- `kernel_entry` : 内核代码的入口地址，由reload_elf_file返回，将系统引导到kernel部分进行执行，这也是该部分存在的意义



### source/kernel

内核代码，内含操作系统核心代码

- kernel.lds:
    链接脚本：描述输入文件中的各个段(数据段,代码段,堆,栈,bss)如何被映射到输出文件中,并控制输出文件的各部分在程序地址空间内的布局




### kernel/include

内含kernel部分的所有头文件，在include中进行分组管理

- os_cfg: 操作系统的配置参数，如进程数量，内核栈大小等

### kernel/init

- start.S
    内核代码的入口
    - `_start.S` : 内核代码的入口，负责调用内核初始化函数（c），传递来自引导程序的启动信息并完成初始化工作
    - `gdt_reload`：重新加载gdt表，在init中设置了gdt表，在此函数中重新设置数据段寄存器和栈
    - `exception_handler` ： 异常处理程序，对相应的异常进行处理，对寄存器进行设置，然后跳转到中断处理函数（c）
- init.c
    内核的初始化代码
    - `kernel_init` ： 初始化代码，由汇编函数调用
    - `init_main`：真正启动内核，并开启第一个进程
- first_task
    内核的第一个进程，在进程中会调用fork/exec指令创建更多进程


### kernel/fs

文件系统相关的接口实现，不完整

### kernel/tools

内核的一些功能函数

- kilb.c 字符串处理相关的函数
- list.c 链表数据结构定义及相关操作
- log.c 实现日志打印功能，方便调试
- bitmap.c 位图数据结构的定义及相关操作
    - 管理分页后的内存空间：每个位代表该部分对应的内存页是否被占用

### kernel/ipc

interprocess communication 用于实现进程间相互通信的功能

- mutex.c   `mutex_t` 定义互斥锁数据结构并定义相关操作
- sem.c     `sem_t`   定义信号量数据结构并定义相关操作


### kernel/dev

管理计算机系统的外部设备，包括定时器，键盘与控制台等。

- console.c  控制台相关代码，用于显示，接受与处理字符
- kbd.c 处理键盘引发的中断，对输入进行处理
- time.c 定时器程序，处理定时器中断



### kernel/cpu

### cpu/cpu

涉及cpu细节的相关数据结构与相关操作，与cpu的设计关联较紧密

### cpu/gdt表：

- 内存的分段存储
- flat mode 
  - 代码段/数据段 起始地址都是0，大小都是4kb
  - 分段机制是x86架构的固有机制。实质上，内存平坦模型没有分段，仍然可以划分各段，只是这里的划分没有在内存管理中发挥作用，所有段都重合，仅在逻辑上将数据进行归纳分类
- 段寄存器存储选择子 高13位用于索引gdt表
- 逻辑地址经过gdt转换成线性地址
- `segment_desc_t`
  - base    该内存段的内存基址
  - limit   该内存段的大小
  - attr    该内存段的性质
- `void segment_desc_set(int selector, uint32_t base,uint32_t limit,uint16_t attr)` ： selector为传入的段寄存器内的值，在函数内右移三位

### cpu/idt表

- 中断门描述符
- 引发中断，产生向量号->根据向量号取idt表项：段描述符+偏移量->根据取出的idt表项查gdt表得到中断处理函数的地址，并跳转执行
- `gate_desc_t`：中断门描述符，在irq.c中，有一个该类型的数组，用来作为idt表
- `void gate_desc_set(gate_desc_t * desc, uint16_t selector, uint32_t offset, uint16_t attr)` ：传入要设置的idt表项描述符和idt表项的内容

### cpu/tss任务状态段

- `tss_t` : 任务状态段数据结构结构，在发生任务切换时，将任务保存和恢复
- `void switch_to_tss (uint32_t tss_selector)` 跳转到指定tss选择子表示的任务


### cpu/irq

Interrupt request,进行中断处理

- `exception_frame_t`：异常栈信息类型，触发异常时，硬件自动将信息压入栈中
- `int irq_install(int irq_num, irq_handler_t handler);` ：将汇编的函数地址设置到对应的表项中
- `void init_pic(void)` ：初始化中断控制器 计算机通过两片8259芯片与外部设备连接，管理外部设备引发的中断
- `void exception_handler_unknown (void)` ：对未知中断进行处理的函数 



### cpu/mmu.h

内存管理模块，将线性（=逻辑）地址映射为物理地址，该映射在loader中是一级映射，在kernel中为2级映射

- `pde_t` 页目录表项 第一级映射表
- `pte_t` 页表项    第二级映射表




### kernel/core

操作系统的核心代码，用于内存管理，系统调用，任务管理

### core/memory

内存管理：分页机制

- `addr_alloc_t` ： 地址分配结构，管理内存的分配和释放
- `memory_map_t`：内存映射结构，建立物理地址与逻辑地址的映射，在memory.c中被用来设置`static pde_t kernel_page_dir[PDE_CNT]` 这是一个内存映射数组，存储映射关系


### core/syscall

系统调用，实际上就是引发一个异常，通过异常处理函数跳转到指定的代码区域，在跳转过程在完成模式转换

-`syscall_frame_t` ： 同异常处理模块一样，该部分结构保存引发异常处理时的任务信息
- `static const syscall_handler_t sys_table[]` ：系统调用表。一张由指向实现各种系统调用的内核函数的函数指针组成的表，该表可以基于系统调用编号进行索引，来定位函数地址，完成系统调用


### core/task

任务管理

Task Register保存一个选择子，指向当前正在执行任务在GDT表中的TSS表项

- `task_t`：记录任务信息的数据结构，如入口地址，tss等
- `task_manager_t` 任务管理数据结构，用于管理内存中的多个结构，记录就绪队列，当前执行进程等
- `void task_set_ready(task_t *task)`：将任务设置为ready，类似的还有sleep/block等
- `void task_dispatch (void)`：进程调度算法，主要思想为时间片轮转算法，在task_time_tick函数中被调用
- `task_time_tick`：在定时器中断时被调用，判断当前进程是否已经用完时间片，睡眠队列中的任务是否睡够了
  - 当前小**bug**：若在时间片内就结束任务，系统将引发页异常
- `idle_task_entry`: 闲置任务，永远为ready状态且任务优先级最低。当其他所有任务都不能执行时，为了出现错误而一直运行，直到有新进程处于ready状态






