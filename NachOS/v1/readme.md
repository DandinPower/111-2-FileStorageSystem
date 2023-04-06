# Version1 Installation

# 安裝步驟

1. 安裝Ubuntu 18.04 LTS (64bits)
2. 安裝工具
    
    ```bash
    sudo apt-get install build-essential
    dpkg --print-architecture
    # 顯示amd64表示distribution為64bit
    dpkg --print-foreign-architectures
    # 顯示i386表示支援
    sudo dpkg --add-architecture i386
    sudo apt-get install csh
    sudo apt-get install gcc
    sudo apt-get install g++
    sudo apt-get update
    sudo apt-get dist-upgrade #解決相依性問題（有點風險的升級）
    sudo apt-get install gcc-multilib g++-multilib
    sudo apt-get install lib32ncurses5 lib32z1
    #ubuntu 20.04 請改為lib32ncurses5-dev
    sudo apt-get install zlib1g:i386 libstdc++6:i386
    sudo apt-get install libc6:i386 libncurses5:i386
    sudo apt-get install libgcc1:i386 libstdc++5:i386
    ```
    
3. 解壓縮NachOS Source Code
    
    ```bash
    tar –zxvf nachos-4.0.tar.gz
    ```
    
4. 解壓縮交叉編譯工具
    
    ```bash
    sudo mv mips-decstation.linux-xgcc /
    cd /
    tar -zxvf mips-decstation.linux-xgcc
    ```
    
5. 修改MakeFile來使用32位元編譯
    1. 在/code/Makefile.common
        
        ```makefile
        # These definitions may change as the software is updated.
        # Some of them are also system dependent
        CPP=/lib/cpp
        CC = g++ -m32 -Wno-deprecated
        LD = g++ -m32 -Wno-deprecated
        AS = as --32                  
        
        PROGRAM = nachos
        ```
        
    2. 在/code/bin/Makefile
        
        ```makefile
        # Copyright (c) 1992-1996 The Regents of the University of California.
        # All rights reserved.  See copyright.h for copyright notice and limitation 
        # of liability and disclaimer of warranty provisions.
        
        include ../Makefile.dep
        CC=gcc -m32                    
        CFLAGS=-I../lib -I../threads $(HOST)
        LD=gcc -m32
        
        all: coff2noff 
        #$(DISASM)
        ```
        
6. 編譯
    
    ```bash
    #切至 code資料夾
    cd /code
    make
    ```
    
7. 測試
    
    ```bash
    cd userprog
    ./nachos –e ../test/test1
    ```
    

# 參考資料

1. [在Windows 10中使用WSL2安裝NachOS - HackMD](https://hackmd.io/@seanpeng12/HkrEHYsu5#%E5%9C%A8Windows-10%E4%B8%AD%E4%BD%BF%E7%94%A8WSL2%E5%AE%89%E8%A3%9DNachOS) 
2. [nachos安装 Ubuntu18.04为例__Raymond_的博客-CSDN博客](https://blog.csdn.net/weixin_43745072/article/details/105764548)