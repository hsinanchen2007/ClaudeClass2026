/*
# 第 3 課：C++ 編譯環境設置（g++、clang++）

---

## 3.1 課程目標

在開始寫 C++ 程式之前，我們需要正確設置編譯環境。本課將介紹主流的 C++ 編譯器、常用的編譯選項，以及如何在不同平台上配置開發環境。

由於你已經在使用 VS Code 和 MSVC 編譯器進行 C++ 開發，本課也會涵蓋 MSVC 的使用方式。

---

## 3.2 主流 C++ 編譯器

| 編譯器 | 開發者 | 平台 | 特點 |
|--------|--------|------|------|
| **GCC (g++)** | GNU | Linux、macOS、Windows | 開源、廣泛使用、標準符合度高 |
| **Clang (clang++)** | LLVM | Linux、macOS、Windows | 錯誤訊息友善、編譯速度快 |
| **MSVC (cl)** | Microsoft | Windows | Visual Studio 整合、Windows 開發首選 |

---

## 3.3 GCC / g++ 編譯器

### 3.3.1 安裝方式

**Linux (Ubuntu/Debian)**

```bash
sudo apt update
sudo apt install g++
```

**macOS**

```bash
# 安裝 Xcode Command Line Tools
xcode-select --install
```

**Windows**

可以透過 MinGW-w64 或 MSYS2 安裝：

```bash
# 使用 MSYS2
pacman -S mingw-w64-x86_64-gcc
```

### 3.3.2 確認安裝

```bash
g++ --version
```

輸出範例：

```
g++ (Ubuntu 11.4.0-1ubuntu1~22.04) 11.4.0
Copyright (C) 2021 Free Software Foundation, Inc.
```

### 3.3.3 基本編譯指令

```bash
# 最基本的編譯
g++ hello.cpp

# 指定輸出檔名
g++ hello.cpp -o hello

# 執行程式
./hello        # Linux / macOS
hello.exe      # Windows
```

---

## 3.4 g++ 常用編譯選項

### 3.4.1 選項總覽

| 選項 | 說明 |
|------|------|
| `-o <name>` | 指定輸出檔案名稱 |
| `-std=c++17` | 指定 C++ 標準版本 |
| `-Wall` | 啟用大部分警告 |
| `-Wextra` | 啟用額外警告 |
| `-Werror` | 將警告視為錯誤 |
| `-g` | 加入除錯資訊（用於 GDB） |
| `-O0` | 不優化（除錯用） |
| `-O2` | 優化等級 2（平衡速度與大小） |
| `-O3` | 最高優化等級 |
| `-c` | 只編譯不連結，產生 .o 檔 |
| `-I<path>` | 新增標頭檔搜尋路徑 |
| `-L<path>` | 新增函式庫搜尋路徑 |
| `-l<name>` | 連結函式庫 |

### 3.4.2 C++ 標準版本

```bash
g++ -std=c++11 program.cpp -o program   # C++11
g++ -std=c++14 program.cpp -o program   # C++14
g++ -std=c++17 program.cpp -o program   # C++17
g++ -std=c++20 program.cpp -o program   # C++20
g++ -std=c++23 program.cpp -o program   # C++23（需要較新的 g++）
```

### 3.4.3 建議的開發編譯選項

```bash
# 開發時（啟用除錯、所有警告）
g++ -std=c++17 -Wall -Wextra -g -O0 program.cpp -o program

# 發布時（最佳化、無除錯資訊）
g++ -std=c++17 -Wall -Wextra -O2 program.cpp -o program
```

---

## 3.5 Clang / clang++ 編譯器

### 3.5.1 安裝方式

**Linux (Ubuntu/Debian)**

```bash
sudo apt update
sudo apt install clang
```

**macOS**

macOS 的 `g++` 實際上預設就是 clang++。

**Windows**

可以從 LLVM 官網下載，或透過 Visual Studio 安裝。

### 3.5.2 確認安裝

```bash
clang++ --version
```

### 3.5.3 使用方式

clang++ 的選項與 g++ 幾乎相同：

```bash
clang++ -std=c++17 -Wall -Wextra -g program.cpp -o program
```

### 3.5.4 clang++ 的優勢

Clang 的錯誤訊息通常更清晰易懂：

**g++ 的錯誤訊息**

```
error: 'string' was not declared in this scope
```

**clang++ 的錯誤訊息**

```
error: unknown type name 'string'; did you mean 'std::string'?
    string name;
    ^~~~~~
    std::string
```

---

## 3.6 MSVC 編譯器（cl.exe）

由於你使用 Windows 和 VS Code，MSVC 是很好的選擇。

### 3.6.1 安裝方式

安裝 Visual Studio 或 Visual Studio Build Tools，選擇「C++ 桌面開發」工作負載。

### 3.6.2 使用 Developer Command Prompt

安裝後，需要使用「Developer Command Prompt」或「Developer PowerShell」來存取 cl.exe。

### 3.6.3 基本編譯指令

```cmd
rem 基本編譯
cl hello.cpp

rem 指定輸出檔名
cl hello.cpp /Fe:hello.exe

rem 指定 C++ 標準
cl /std:c++17 hello.cpp

rem 啟用警告
cl /W4 hello.cpp

rem 加入除錯資訊
cl /Zi hello.cpp
```

### 3.6.4 MSVC 常用選項

| 選項 | 說明 |
|------|------|
| `/Fe:<name>` | 指定輸出執行檔名稱 |
| `/std:c++17` | 指定 C++ 標準版本 |
| `/W4` | 警告等級 4（建議使用） |
| `/WX` | 將警告視為錯誤 |
| `/Zi` | 加入除錯資訊 |
| `/Od` | 停用優化（除錯用） |
| `/O2` | 優化速度 |
| `/EHsc` | 啟用 C++ 例外處理 |
| `/utf-8` | 原始碼和執行使用 UTF-8 |

### 3.6.5 處理中文（UTF-8 設定）

根據你之前的學習，處理中文時需要特別設定：

```cmd
rem 編譯時指定 UTF-8
cl /std:c++17 /utf-8 /EHsc program.cpp

rem 或使用完整選項
cl /std:c++17 /source-charset:utf-8 /execution-charset:utf-8 /EHsc program.cpp
```

---

## 3.7 VS Code 配置

### 3.7.1 tasks.json（編譯任務）

在專案的 `.vscode` 資料夾中建立 `tasks.json`：

**使用 MSVC**

```json
{
    "version": "2.0.0",
    "tasks": [
        {
            "label": "MSVC Build",
            "type": "shell",
            "command": "cl",
            "args": [
                "/std:c++17",
                "/utf-8",
                "/EHsc",
                "/W4",
                "/Zi",
                "${file}",
                "/Fe:${fileDirname}\\${fileBasenameNoExtension}.exe"
            ],
            "group": {
                "kind": "build",
                "isDefault": true
            },
            "problemMatcher": "$msCompile"
        }
    ]
}
```

**使用 g++**

```json
{
    "version": "2.0.0",
    "tasks": [
        {
            "label": "g++ Build",
            "type": "shell",
            "command": "g++",
            "args": [
                "-std=c++17",
                "-Wall",
                "-Wextra",
                "-g",
                "${file}",
                "-o",
                "${fileDirname}/${fileBasenameNoExtension}"
            ],
            "group": {
                "kind": "build",
                "isDefault": true
            },
            "problemMatcher": "$gcc"
        }
    ]
}
```

### 3.7.2 launch.json（除錯配置）

```json
{
    "version": "0.2.0",
    "configurations": [
        {
            "name": "Debug",
            "type": "cppvsdbg",
            "request": "launch",
            "program": "${fileDirname}\\${fileBasenameNoExtension}.exe",
            "args": [],
            "stopAtEntry": false,
            "cwd": "${workspaceFolder}",
            "environment": [],
            "console": "externalTerminal",
            "preLaunchTask": "MSVC Build"
        }
    ]
}
```

### 3.7.3 c_cpp_properties.json（IntelliSense 配置）

```json
{
    "configurations": [
        {
            "name": "Win32",
            "includePath": [
                "${workspaceFolder}/**"
            ],
            "defines": [
                "_DEBUG",
                "UNICODE",
                "_UNICODE"
            ],
            "compilerPath": "cl.exe",
            "cStandard": "c17",
            "cppStandard": "c++17",
            "intelliSenseMode": "windows-msvc-x64"
        }
    ],
    "version": 4
}
```

---

## 3.8 多檔案編譯

當程式變大時，我們通常會分成多個檔案。

### 3.8.1 檔案結構範例

```
project/
├── main.cpp
├── calculator.h
└── calculator.cpp
```

### 3.8.2 calculator.h

```cpp
#ifndef CALCULATOR_H
#define CALCULATOR_H

class Calculator {
public:
    int add(int a, int b);
    int subtract(int a, int b);
};

#endif
```

### 3.8.3 calculator.cpp

```cpp
#include "calculator.h"

int Calculator::add(int a, int b) {
    return a + b;
}

int Calculator::subtract(int a, int b) {
    return a - b;
}
```

### 3.8.4 main.cpp

```cpp
#include <iostream>
#include "calculator.h"

int main() {
    Calculator calc;
    
    std::cout << "3 + 5 = " << calc.add(3, 5) << std::endl;
    std::cout << "10 - 4 = " << calc.subtract(10, 4) << std::endl;
    
    return 0;
}
```

### 3.8.5 編譯多個檔案

**方法一：一次編譯所有檔案**

```bash
# g++
g++ -std=c++17 main.cpp calculator.cpp -o program

# MSVC
cl /std:c++17 /utf-8 /EHsc main.cpp calculator.cpp /Fe:program.exe
```

**方法二：分別編譯再連結**

```bash
# g++
g++ -std=c++17 -c main.cpp -o main.o
g++ -std=c++17 -c calculator.cpp -o calculator.o
g++ main.o calculator.o -o program

# MSVC
cl /std:c++17 /c main.cpp
cl /std:c++17 /c calculator.cpp
link main.obj calculator.obj /OUT:program.exe
```

---

## 3.9 完整範例程式

讓我們建立一個程式來測試編譯環境是否正確設置：

```cpp
#include <iostream>
#include <string>
#include <vector>

// ============================================================
// 測試編譯環境設置
// ============================================================

// 顯示編譯器資訊
void showCompilerInfo() {
    std::cout << "========================================" << std::endl;
    std::cout << "    編譯器資訊" << std::endl;
    std::cout << "========================================" << std::endl;
    
#if defined(__clang__)
    std::cout << "編譯器: Clang " << __clang_major__ << "." 
              << __clang_minor__ << "." << __clang_patchlevel__ << std::endl;
#elif defined(__GNUC__)
    std::cout << "編譯器: GCC " << __GNUC__ << "." 
              << __GNUC_MINOR__ << "." << __GNUC_PATCHLEVEL__ << std::endl;
#elif defined(_MSC_VER)
    std::cout << "編譯器: MSVC " << _MSC_VER << std::endl;
#else
    std::cout << "編譯器: Unknown" << std::endl;
#endif
}

// 顯示 C++ 標準版本
void showCppStandard() {
    std::cout << std::endl;
    std::cout << "C++ 標準: ";
    
#if __cplusplus >= 202302L
    std::cout << "C++23";
#elif __cplusplus >= 202002L
    std::cout << "C++20";
#elif __cplusplus >= 201703L
    std::cout << "C++17";
#elif __cplusplus >= 201402L
    std::cout << "C++14";
#elif __cplusplus >= 201103L
    std::cout << "C++11";
#elif __cplusplus >= 199711L
    std::cout << "C++98";
#else
    std::cout << "Pre-C++98";
#endif
    
    std::cout << " (__cplusplus = " << __cplusplus << ")" << std::endl;
}

// 顯示平台資訊
void showPlatformInfo() {
    std::cout << std::endl;
    std::cout << "作業系統: ";
    
#if defined(_WIN32) || defined(_WIN64)
    std::cout << "Windows";
    #if defined(_WIN64)
        std::cout << " (64-bit)";
    #else
        std::cout << " (32-bit)";
    #endif
#elif defined(__linux__)
    std::cout << "Linux";
#elif defined(__APPLE__)
    std::cout << "macOS";
#else
    std::cout << "Unknown";
#endif
    
    std::cout << std::endl;
}

// 測試基本 C++ 功能
void testBasicFeatures() {
    std::cout << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "    基本功能測試" << std::endl;
    std::cout << "========================================" << std::endl;
    
    // 測試 auto（C++11）
    auto number = 42;
    std::cout << "auto 變數: " << number << std::endl;
    
    // 測試範圍 for 迴圈（C++11）
    std::vector<int> vec = {1, 2, 3, 4, 5};
    std::cout << "vector 內容: ";
    for (const auto& v : vec) {
        std::cout << v << " ";
    }
    std::cout << std::endl;
    
    // 測試 lambda（C++11）
    auto add = [](int a, int b) { return a + b; };
    std::cout << "lambda 計算 3+5: " << add(3, 5) << std::endl;
    
    // 測試初始化列表（C++11）
    std::vector<std::string> fruits = {"蘋果", "香蕉", "橘子"};
    std::cout << "水果清單: ";
    for (const auto& fruit : fruits) {
        std::cout << fruit << " ";
    }
    std::cout << std::endl;
}

// 測試中文輸出
void testChineseOutput() {
    std::cout << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "    中文輸出測試" << std::endl;
    std::cout << "========================================" << std::endl;
    
    std::string greeting = "你好，世界！";
    std::cout << greeting << std::endl;
    std::cout << "這是一段中文測試文字。" << std::endl;
    std::cout << "數字：一二三四五" << std::endl;
}

// 主程式
int main() {
    // 設定 Windows 控制台 UTF-8 輸出
#ifdef _WIN32
    // 如果需要更完整的處理，可加入：
    // #include <windows.h>
    // SetConsoleOutputCP(65001);
#endif
    
    showCompilerInfo();
    showCppStandard();
    showPlatformInfo();
    testBasicFeatures();
    testChineseOutput();
    
    std::cout << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "    編譯環境測試完成！" << std::endl;
    std::cout << "========================================" << std::endl;
    
    return 0;
}
```

### 編譯與執行

**使用 g++**

```bash
g++ -std=c++17 -Wall -Wextra lesson03.cpp -o lesson03
./lesson03
```

**使用 MSVC**

```cmd
cl /std:c++17 /utf-8 /EHsc /W4 lesson03.cpp /Fe:lesson03.exe
lesson03.exe
```

### 執行結果（MSVC 範例）

```
========================================
    編譯器資訊
========================================
編譯器: MSVC 1940

C++ 標準: C++17 (__cplusplus = 199711)

作業系統: Windows (64-bit)

========================================
    基本功能測試
========================================
auto 變數: 42
vector 內容: 1 2 3 4 5 
lambda 計算 3+5: 8
水果清單: 蘋果 香蕉 橘子 

========================================
    中文輸出測試
========================================
你好，世界！
這是一段中文測試文字。
數字：一二三四五

========================================
    編譯環境測試完成！
========================================
```

> **注意**：MSVC 預設的 `__cplusplus` 值是 `199711`，即使使用 C++17。要讓它顯示正確的值，需要加入 `/Zc:__cplusplus` 選項。

---

## 3.10 常見編譯錯誤與解決方案

### 錯誤一：找不到編譯器

```
'g++' is not recognized as an internal or external command
```

**解決方案**：將編譯器路徑加入系統環境變數 PATH。

### 錯誤二：找不到標頭檔

```
fatal error: iostream: No such file or directory
```

**解決方案**：確認編譯器安裝正確，或使用正確的 include 路徑。

### 錯誤三：未定義的引用

```
undefined reference to `function_name'
```

**解決方案**：確認所有 .cpp 檔案都有被編譯和連結。

### 錯誤四：C++ 標準不支援

```
error: 'auto' changes meaning in C++11
```

**解決方案**：使用 `-std=c++11` 或更高版本的標準。

---

## 3.11 本課重點回顧

| 項目 | 說明 |
|------|------|
| 主流編譯器 | g++、clang++、MSVC |
| 指定標準 | `-std=c++17`（g++）、`/std:c++17`（MSVC） |
| 啟用警告 | `-Wall -Wextra`（g++）、`/W4`（MSVC） |
| 除錯資訊 | `-g`（g++）、`/Zi`（MSVC） |
| UTF-8 支援 | `/utf-8`（MSVC） |
| 多檔案編譯 | 列出所有 .cpp 檔案，或分別編譯再連結 |

---

## 3.12 下一課預告

下一課我們將學習 **命名空間（namespace）**，這是 C++ 用來組織程式碼、避免名稱衝突的重要機制。

準備好進入 **第 4 課：命名空間（namespace）基礎** 了嗎？
*/



#include <iostream>
#include <string>
#include <vector>

// ============================================================
// 測試編譯環境設置
// ============================================================

// 顯示編譯器資訊
void showCompilerInfo() {
    std::cout << "========================================" << std::endl;
    std::cout << "    編譯器資訊" << std::endl;
    std::cout << "========================================" << std::endl;
    
#if defined(__clang__)
    std::cout << "編譯器: Clang " << __clang_major__ << "." 
              << __clang_minor__ << "." << __clang_patchlevel__ << std::endl;
#elif defined(__GNUC__)
    std::cout << "編譯器: GCC " << __GNUC__ << "." 
              << __GNUC_MINOR__ << "." << __GNUC_PATCHLEVEL__ << std::endl;
#elif defined(_MSC_VER)
    std::cout << "編譯器: MSVC " << _MSC_VER << std::endl;
#else
    std::cout << "編譯器: Unknown" << std::endl;
#endif
}

// 顯示 C++ 標準版本
void showCppStandard() {
    std::cout << std::endl;
    std::cout << "C++ 標準: ";
    
#if __cplusplus >= 202302L
    std::cout << "C++23";
#elif __cplusplus >= 202002L
    std::cout << "C++20";
#elif __cplusplus >= 201703L
    std::cout << "C++17";
#elif __cplusplus >= 201402L
    std::cout << "C++14";
#elif __cplusplus >= 201103L
    std::cout << "C++11";
#elif __cplusplus >= 199711L
    std::cout << "C++98";
#else
    std::cout << "Pre-C++98";
#endif
    
    std::cout << " (__cplusplus = " << __cplusplus << ")" << std::endl;
}

// 顯示平台資訊
void showPlatformInfo() {
    std::cout << std::endl;
    std::cout << "作業系統: ";
    
#if defined(_WIN32) || defined(_WIN64)
    std::cout << "Windows";
    #if defined(_WIN64)
        std::cout << " (64-bit)";
    #else
        std::cout << " (32-bit)";
    #endif
#elif defined(__linux__)
    std::cout << "Linux";
#elif defined(__APPLE__)
    std::cout << "macOS";
#else
    std::cout << "Unknown";
#endif
    
    std::cout << std::endl;
}

// 測試基本 C++ 功能
void testBasicFeatures() {
    std::cout << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "    基本功能測試" << std::endl;
    std::cout << "========================================" << std::endl;
    
    // 測試 auto（C++11）
    auto number = 42;
    std::cout << "auto 變數: " << number << std::endl;
    
    // 測試範圍 for 迴圈（C++11）
    std::vector<int> vec = {1, 2, 3, 4, 5};
    std::cout << "vector 內容: ";
    for (const auto& v : vec) {
        std::cout << v << " ";
    }
    std::cout << std::endl;
    
    // 測試 lambda（C++11）
    auto add = [](int a, int b) { return a + b; };
    std::cout << "lambda 計算 3+5: " << add(3, 5) << std::endl;
    
    // 測試初始化列表（C++11）
    std::vector<std::string> fruits = {"蘋果", "香蕉", "橘子"};
    std::cout << "水果清單: ";
    for (const auto& fruit : fruits) {
        std::cout << fruit << " ";
    }
    std::cout << std::endl;
}

// 測試中文輸出
void testChineseOutput() {
    std::cout << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "    中文輸出測試" << std::endl;
    std::cout << "========================================" << std::endl;
    
    std::string greeting = "你好，世界！";
    std::cout << greeting << std::endl;
    std::cout << "這是一段中文測試文字。" << std::endl;
    std::cout << "數字：一二三四五" << std::endl;
}

// 主程式
int main() {
    // 設定 Windows 控制台 UTF-8 輸出
#ifdef _WIN32
    // 如果需要更完整的處理，可加入：
    // #include <windows.h>
    // SetConsoleOutputCP(65001);
#endif
    
    showCompilerInfo();
    showCppStandard();
    showPlatformInfo();
    testBasicFeatures();
    testChineseOutput();
    
    std::cout << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "    編譯環境測試完成！" << std::endl;
    std::cout << "========================================" << std::endl;
    
    return 0;
}
