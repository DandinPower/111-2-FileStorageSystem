# Version1 Installation

# 安裝步驟

1. 安裝Ubuntu 22.04 LTS (64bits)
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
    sudo apt-get install lib32ncurses5-dev lib32z1
    sudo apt-get install zlib1g:i386 libstdc++6:i386
    sudo apt-get install libc6:i386 libncurses5:i386
    sudo apt-get install libgcc1:i386 libstdc++5:i386
    ```
    
3. 解壓縮NachOS Source Code
    
    ```bash
    tar –xvf NachOS-4.0_MP1.tar
    ```
    
4. 解壓縮交叉編譯工具
    
    ```bash
    sudo mv mips-decstation.linux-xgcc /
    cd /
    sudo tar -zxvf mips-decstation.linux-xgcc
    ```
    
5. 修改MakeFile來使用32位元編譯
    1. 在/code/build.linux/Makefile
        
        ```makefile
            CPP=/lib/cpp
            CC = g++ -m32 -Wno-deprecated
            LD = g++ -m32 -Wno-deprecated
            AS = as --32
            RM = /bin/rm
        ```
        
6. 編譯
    
    ```bash
    #切至code/build.linux資料夾
    make depend
    make
    ```
    
7. 測試
    
    ```bash
    ./nachos –e ../test/halt
    ```
    
# 參考資料

1. [在Windows 10中使用WSL2安裝NachOS - HackMD](https://hackmd.io/@seanpeng12/HkrEHYsu5#%E5%9C%A8Windows-10%E4%B8%AD%E4%BD%BF%E7%94%A8WSL2%E5%AE%89%E8%A3%9DNachOS) 
2. [nachos安装 Ubuntu18.04为例__Raymond_的博客-CSDN博客](https://blog.csdn.net/weixin_43745072/article/details/105764548)