// =============================================================================
//  第 3 課 -1  —  環境驗證程式：UTF-8 輸出、std::endl 的代價、註解的邊界
// =============================================================================
//
// 【主題資訊 Information】
//   本檔用途  ： 一支「跑得起來就代表環境沒問題」的驗證程式
//   驗證項目  ： 編譯器與版本、C++ 標準、作業系統、C++11 基本語法、中文輸出
//   標準版本  ： 以 -std=c++17 編譯；程式內用 __cplusplus 自我回報
//   標頭檔    ： <iostream>、<string>、<vector>、<streambuf>（本檔新增的計數示範）
//   本機環境  ： g++ (Ubuntu 15.2.0-16ubuntu1) 15.2.0、x86-64 Linux
//   檔案結構  ： 第 1～688 行是一整塊 /* ... */ 說明文件（含 VS Code 設定範例、
//               多檔案專案範例），「真正會被編譯的程式碼」從第 692 行開始。
//               閱讀時請注意這件事 —— 同一段程式在本檔出現兩次，
//               上面那份在註解裡，不會被編譯。
//
// 【詳細解釋 Explanation】
//
// 【1. 本檔為什麼會產生一個 -Wcomment 警告（而且它是真實有用的教材）】
//   編譯本檔時 g++ 15.2 會報：
//       第 3 課...1.cpp:329:36: warning: '/*' within comment [-Wcomment]
//   第 329 行是 VS Code 設定範例裡的一行： "${workspaceFolder}/**"
//   那個 /** 剛好構成一個 /*，而它出現在一個已經開始的區塊註解「裡面」。
//   關鍵規則是：C++ 的區塊註解「不能巢狀」。
//   /* 開始之後，編譯器只找第一個 */ 當結尾，中間再出現多少個 /* 都只是普通字元。
//   所以這裡沒有真的出事（後面沒有跟著 */，註解仍在第 688 行才正確結束），
//   g++ 只是提醒你「這裡看起來像想巢狀註解，你確定嗎？」
//   但同樣的規則在別的情況下會咬人 —— 見【概念補充 (A)】。
//
// 【2. std::endl 與 '\n' 的差別：不是風格問題，是有沒有 flush】
//   本檔通篇使用 std::endl。它等價於：
//       os << '\n' << std::flush;
//   也就是「換行」再加上「強制把緩衝區內容送出去」。'\n' 只做前者。
//   這不是口味差異，而是實際的系統呼叫次數差異。本檔新增的
//   demoEndlVsNewline() 用一個會計數的 streambuf 量給你看（不是計時，
//   是數次數，所以每次跑都一樣）：
//       用 std::endl 輸出兩行 → sync（flush）被呼叫 2 次，字元 12 個
//       用 '\n'      輸出兩行 → sync 被呼叫 0 次，字元同樣 12 個
//   字元數完全相同，差的純粹是 flush 次數。在迴圈裡輸出十萬行時，
//   這就是十萬次不必要的緩衝區排空。
//
//   那什麼時候該用 std::endl？
//     * 程式可能異常終止（abort／segfault），你需要確保訊息已經寫出去。
//     * 互動式提示字元（「請輸入：」）後面，需要立刻讓使用者看到。
//     * 寫入的是給另一個行程即時讀取的管線／log。
//   其餘場合（尤其是大量輸出）一律用 '\n'，需要時再明確 std::flush。
//   註：程式「正常」結束時，std::cout 會被自動 flush，不會掉字；
//       會掉字的是異常終止（這也是收集會 crash 的程式輸出時要用
//       stdbuf -o0 的原因）。
//
// 【3. 中文輸出到底在測什麼】
//   testChineseOutput() 看似只是印幾行字，實際上串起三件必須一致的事：
//     ① 原始碼檔案的編碼      —— 本檔是 UTF-8
//     ② 編譯器怎麼解讀原始碼  —— g++ 預設就假設輸入是 UTF-8（-finput-charset）
//     ③ 終端機怎麼解讀輸出    —— Linux 終端機預設 UTF-8
//   三者一致，中文才會正確顯示。任何一環不同就會出現亂碼。
//   Windows + MSVC 是最容易出事的組合：MSVC 預設會用系統的 ANSI 代碼頁
//   （繁中系統是 CP950）去解讀原始碼，所以要加 /utf-8；
//   主控台則預設 CP950，要 chcp 65001 或 SetConsoleOutputCP(65001)。
//
// 【4. std::string 存中文時，size() 給的是「位元組數」不是「字數」】
//   本機實測：std::string("你好").size() == 6（每個中文字 3 個 UTF-8 位元組），
//   而 sizeof("你好") == 7（6 個位元組 + 結尾的 '\0'）。
//   這代表所有「取前 N 個字」「補空白對齊欄位」的直覺寫法都有 bug 風險：
//   從中間切下去會把一個中文字劈成兩半，產生無效的位元組序列。
//   本檔的實務範例 utf8Truncate() 就是在示範正確做法。
//
// 【概念補充 Concept Deep Dive】
//
// (A) 「區塊註解不能巢狀」造成的真實災難
//   想把一大段程式碼註解掉，而它裡面已經有 /* ... */：
//       /*  想註解掉的開始
//           int a = 1;   /* 原本就有的註解 */
//           int b = 2;   ← 這一行「不在註解裡」，它是活的程式碼！
//       */               ← 這個 */ 沒有對應的開頭，直接編譯錯誤
//   註解在第一個 */ 就結束了。本機實測一個最小例子：
//       /* outer /* inner */ int x = 1;
//   g++ 給出 warning: '/*' within comment [-Wcomment]，
//   而 int x = 1; 確實被當成程式碼編譯了。
//   所以「大段停用程式碼」的正確工具是 #if 0 ... #endif —— 它可以巢狀，
//   而且能一眼看出範圍。
//
// (B) 為什麼 __cplusplus 印出來是 201703 而不是 201703L
//   L 只是原始碼中「這個字面值是 long」的標記，不是數值的一部分。
//   輸出時印的是數值本身。
//
// (C) std::cout 與 printf 混用要小心
//   兩者各有自己的緩衝區。預設情況下 C++ 標準要求它們同步
//   （std::ios_base::sync_with_stdio(true)），所以順序不會亂；
//   但這個同步正是 iostream 效能較差的主因。
//   若為了效能呼叫 sync_with_stdio(false)，就「絕對不能」再混用 printf，
//   否則輸出順序會交錯錯亂。
//
// (D) 這支程式的真正價值：它是一份「可執行的環境報告」
//   新機器裝好工具鏈後，跑一次這支程式就能同時確認：編譯器裝對了、
//   標準版本設對了、C++11 語法可用、UTF-8 鏈路完整。
//   比一項項手動檢查快，也不會漏。
//
// 【注意事項 Pay Attention】
//   1. 本檔第 692 行以前都是註解，改程式時請確認自己改的是「下半部」那份。
//   2. -Wcomment 警告來自第 329 行的 VS Code 設定範例，是刻意保留的教材，
//      不是筆誤（見檔尾但書）。
//   3. main() 裡 #ifdef _WIN32 區塊內那兩行是「示意」，不能直接取消註解：
//      #include 必須寫在檔案最外層，不能放在函式本體內。
//   4. 大量輸出時 std::endl 會造成不必要的 flush；請改用 '\n'。
//      但要收集「會崩潰的程式」的輸出時，反而要確保有 flush
//      （或在外部用 stdbuf -o0），否則緩衝中的內容會隨行程一起消失。
//   5. 處理中文字串時，size() 是位元組數。切割、對齊、截斷都必須以
//      UTF-8 的多位元組邊界為單位，不能直接對 index 動手。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】std::endl、UTF-8 與註解規則
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. std::endl 和 '\n' 有什麼差別？該用哪一個？
//     答：std::endl 等於 '\n' 加上 std::flush，也就是多做一次「強制排空
//         緩衝區」。本機用計數型 streambuf 實測：輸出同樣兩行、同樣 12 個
//         字元，std::endl 觸發 2 次 sync，'\n' 觸發 0 次。
//         一般輸出（尤其迴圈內）用 '\n'；需要立即可見或程式可能異常終止時，
//         才用 std::endl 或明確 std::flush。
//     追問：程式正常結束時，用 '\n' 會不會掉字？
//         → 不會。正常結束時 std::cout 會被自動 flush。
//           會掉字的是 abort／未捕捉例外這類異常終止。
//
// 🔥 Q2. std::string s = "你好"; s.size() 是多少？
//     答：本機（UTF-8 原始碼）實測是 6，不是 2 —— 每個中文字佔 3 個位元組。
//         std::string 存的是位元組序列，它完全不知道「字元」的概念。
//         另外 sizeof("你好") 是 7，多的那一個是字串字面值結尾的 '\0'。
//     追問：那要怎麼正確地取前 N 個字？
//         → 必須依 UTF-8 編碼規則判斷每個字元佔幾個位元組
//           （後續位元組的高兩位固定是 10），只在字元邊界切割。
//           本檔的 utf8Truncate() 就是這個做法。
//
// 🔥 Q3. 為什麼中文在 Windows 上特別容易變亂碼？
//     答：因為要三個環節同時是 UTF-8：原始碼檔案編碼、編譯器解讀原始碼的
//         編碼、終端機解讀輸出的編碼。Windows 上 MSVC 預設用系統 ANSI
//         代碼頁（繁中是 CP950）讀原始碼，主控台預設也是 CP950，
//         所以要加 /utf-8，必要時再 chcp 65001。Linux 三者預設都是 UTF-8。
//     追問：加了 /utf-8 還是亂碼怎麼辦？
//         → 檢查原始碼檔案本身是否真的存成 UTF-8（而不是 CP950），
//           以及主控台代碼頁。三環缺一不可。
//
// ⚠️ 陷阱 1. 「想暫時停用一大段程式碼，就用 /* */ 包起來」——為什麼會出事？
//     答：C++ 的區塊註解不能巢狀。如果被包住的程式碼裡本來就有 /* ... */，
//         整段註解會在「裡面那個 */」就提前結束，後面的程式碼變成活的，
//         而最外層的 */ 則變成沒有開頭的孤兒，直接編譯錯誤。
//         正確工具是 #if 0 ... #endif，它可以巢狀。
//     為什麼會錯：把註解想成有配對的括號。它不是 —— 掃到第一個 */ 就結束，
//         沒有任何配對邏輯。本檔第 329 行的 -Wcomment 警告就是這條規則
//         在提醒你。
//
// ⚠️ 陷阱 2. 「每行都用 std::endl 比較保險」——保險在哪、代價是什麼？
//     答：保險在異常終止時不會掉輸出；代價是每一行都強迫排空緩衝區。
//         輸出十萬行就是十萬次多餘的 flush，這在 log 密集的程式裡是
//         實際可量到的效能損失。正確策略是預設 '\n'，
//         只在關鍵節點（或偵錯 crash 時）明確 flush。
//     為什麼會錯：把 std::endl 當成「C++ 版的換行符號」。
//         它是「換行 + flush」兩個動作的組合，名字完全沒有透露第二個動作。
//
// ⚠️ 陷阱 3. 「字串長度就用 size()，中英文都一樣」——什麼時候會爆？
//     答：size() 回傳位元組數。用它做「截斷到 10 個字」或「補空白對齊」時，
//         中文會被從中間切斷，產生不完整的 UTF-8 序列，
//         終端機／瀏覽器會顯示成問號或替代字元，寫進資料庫還可能被拒絕。
//     為什麼會錯：在只有 ASCII 的年代，位元組數 == 字元數 == 顯示寬度。
//         UTF-8 之後這三個數字全部不同（更別提全形字的顯示寬度是 2）。
// ═══════════════════════════════════════════════════════════════════════════

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
#include <streambuf>

// ============================================================
// 【日常實務範例 1】用「計數」而不是「計時」證明 std::endl 的代價
// ------------------------------------------------------------
// 情境：團隊 code review 常爭論「std::endl 到底有沒有比較慢」。用碼錶量
//       會受機器負載影響、每次結果都不同，說服力不足也無法寫進測試。
//       正確做法是量「可觀察的操作次數」——次數是決定性的，每次跑都一樣。
// 做法：自訂一個 streambuf，覆寫 sync()（flush 時會被呼叫）與 overflow()
//       （每輸出一個字元會被呼叫），各自累加計數，然後把 std::ostream
//       接到它上面。這樣就能精確數出「同樣的輸出內容，觸發了幾次 flush」。
// ============================================================
class CountingStreambuf : public std::streambuf {
public:
    int syncCount = 0;   // flush 次數
    int charCount = 0;   // 實際輸出的字元數
protected:
    int sync() override {
        ++syncCount;
        return 0;
    }
    int_type overflow(int_type c) override {
        if (c != traits_type::eof()) ++charCount;
        return c;   // 回傳非 eof 表示「已接受這個字元」
    }
};

void demoEndlVsNewline() {
    std::cout << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "    std::endl vs '\\n'（計數證據）" << std::endl;
    std::cout << "========================================" << std::endl;

    {
        CountingStreambuf buf;
        std::ostream os(&buf);
        os << "line1" << std::endl << "line2" << std::endl;
        std::cout << "使用 std::endl : flush(sync) 次數 = " << buf.syncCount
                  << " , 輸出字元數 = " << buf.charCount << std::endl;
    }
    {
        CountingStreambuf buf;
        std::ostream os(&buf);
        os << "line1" << '\n' << "line2" << '\n';
        std::cout << "使用 '\\n'      : flush(sync) 次數 = " << buf.syncCount
                  << " , 輸出字元數 = " << buf.charCount << std::endl;
    }

    std::cout << "結論：輸出內容完全相同（同樣 12 個字元），" << std::endl;
    std::cout << "      差別只在 std::endl 每行都強制排空緩衝區。" << std::endl;
}

// ============================================================
// 【日常實務範例 2】安全截斷 UTF-8 字串（log 欄位對齊、資料庫長度限制）
// ------------------------------------------------------------
// 情境：要把使用者暱稱塞進固定寬度的 log 欄位，或存進限制 20 bytes 的
//       資料庫欄位。直接 s.substr(0, 20) 會把中文字從中間切斷，
//       產生不完整的 UTF-8 序列 —— 終端機顯示成替代字元，
//       嚴格的資料庫（如 MySQL utf8mb4）甚至會直接拒絕寫入。
// 為什麼用到本主題：std::string 存的是位元組，size() 給的是位元組數。
//       要正確處理就必須認得 UTF-8 的多位元組結構。
// UTF-8 規則（本函式依此判斷）：
//       0xxxxxxx                            → 1 byte（ASCII）
//       110xxxxx 10xxxxxx                   → 2 bytes
//       1110xxxx 10xxxxxx 10xxxxxx          → 3 bytes（中日韓多在此）
//       11110xxx 10xxxxxx 10xxxxxx 10xxxxxx → 4 bytes（emoji 等）
//       關鍵：後續位元組一律是 10xxxxxx，所以「是不是字元開頭」可以
//             靠 (b & 0xC0) != 0x80 判斷，不需要任何函式庫。
// ============================================================
std::string utf8Truncate(const std::string& s, std::size_t maxBytes) {
    if (s.size() <= maxBytes) return s;

    std::size_t cut = maxBytes;
    // 往回退到字元邊界：只要目前位置是「後續位元組」(10xxxxxx) 就再退一格
    while (cut > 0) {
        unsigned char b = static_cast<unsigned char>(s[cut]);
        if ((b & 0xC0) != 0x80) break;   // 不是後續位元組 → 這裡就是邊界
        --cut;
    }
    return s.substr(0, cut);
}

std::size_t utf8CharCount(const std::string& s) {
    std::size_t n = 0;
    for (unsigned char b : s) {
        if ((b & 0xC0) != 0x80) ++n;     // 只數「字元開頭」的位元組
    }
    return n;
}

void demoUtf8Truncate() {
    std::cout << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "    UTF-8 位元組 vs 字元" << std::endl;
    std::cout << "========================================" << std::endl;

    const std::string name = "張三豐and王五";
    std::cout << "原字串        : " << name << std::endl;
    std::cout << "size() 位元組數: " << name.size() << std::endl;
    std::cout << "實際字元數    : " << utf8CharCount(name) << std::endl;

    // 錯誤示範：直接切位元組，會把中文字劈開
    // （這裡只印「切了幾個位元組」，不印被切壞的內容本身，
    //   因為不完整的 UTF-8 序列在不同終端機的顯示方式並不一致）
    // 判斷「切點是否落在某個字元的中間」：若原字串在 index 8 的位元組
    // 是 UTF-8 的後續位元組(10xxxxxx)，就代表這一刀砍在字元內部。
    bool cutInsideChar =
        (static_cast<unsigned char>(name[8]) & 0xC0) == 0x80;
    std::string naive = name.substr(0, 8);
    std::cout << "直接 substr(0,8) : 取得 " << naive.size()
              << " 個位元組；切點落在字元中間？ "
              << (cutInsideChar ? "是（產生不完整序列）" : "否")
              << std::endl;

    // 正確做法：退回字元邊界
    std::string safe = utf8Truncate(name, 8);
    std::cout << "utf8Truncate(,8): \"" << safe << "\" ("
              << safe.size() << " 位元組, " << utf8CharCount(safe)
              << " 個字元)" << std::endl;

    std::string safe2 = utf8Truncate(name, 15);
    std::cout << "utf8Truncate(,15): \"" << safe2 << "\" ("
              << safe2.size() << " 位元組, " << utf8CharCount(safe2)
              << " 個字元)" << std::endl;
}
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
    demoEndlVsNewline();
    demoUtf8Truncate();
    
    std::cout << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "    編譯環境測試完成！" << std::endl;
    std::cout << "========================================" << std::endl;
    
    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 3 課：C++ 編譯環境設置（g++、clang++）1.cpp" -o lesson3_1

// 【編譯時的 -Wcomment 警告是刻意保留的教材】
//   本機 g++ 15.2 會報：
//       第 3 課：C++ 編譯環境設置（g++、clang++）1.cpp:329:36:
//       warning: '/*' within comment [-Wcomment]
//   第 329 行是檔頭那一大塊說明文件裡的 VS Code 設定範例：
//       "${workspaceFolder}/**"
//   其中的 /** 構成一個 /*，而它出現在一個已經開始的區塊註解裡。
//   C++ 的區塊註解不能巢狀，所以這個 /* 只是普通字元，
//   註解仍在第 688 行才正確結束 —— 此處為良性警告。
//   但同樣的規則在「用 /* */ 停用一大段本來就含註解的程式碼」時會真的出事
//   （見上方【概念補充 (A)】的最小重現）。要停用大段程式碼請用 #if 0 ... #endif。
//
// 【輸出會隨編譯器／平台而不同】
//   下方預期輸出取自 g++ (Ubuntu 15.2.0-16ubuntu1) 15.2.0、-std=c++17、
//   x86-64 Linux。改用 clang++ 建置時，「編譯器:」那一行會變成
//   Clang 21.1.8（本機實測）；在 Windows 上「作業系統」會變成 Windows。
//   其餘各行（含 flush 計數與 UTF-8 位元組數）不隨平台改變。
//
// 【flush 計數為何可信】
//   demoEndlVsNewline() 量的是「sync() 被呼叫幾次」這種可數的操作，
//   不是耗時。次數是決定性的，重跑幾次都是 2 與 0，
//   不會像計時那樣受機器負載影響。
//
// 【為何本檔沒有 LeetCode 範例】
//   本課主題是工具鏈與環境驗證（編譯器巨集、編譯選項、UTF-8 鏈路、
//   註解與 flush 語意），不是演算法。LeetCode 沒有對應題型，
//   硬掛一題只會製造假關聯，故從缺。
//   本檔改以兩個真實情境（用計數證明 std::endl 的代價、
//   安全截斷 UTF-8 字串）呈現實戰價值。

// === 預期輸出 ===
// ========================================
//     編譯器資訊
// ========================================
// 編譯器: GCC 15.2.0
//
// C++ 標準: C++17 (__cplusplus = 201703)
//
// 作業系統: Linux
//
// ========================================
//     基本功能測試
// ========================================
// auto 變數: 42
// vector 內容: 1 2 3 4 5
// lambda 計算 3+5: 8
// 水果清單: 蘋果 香蕉 橘子
//
// ========================================
//     中文輸出測試
// ========================================
// 你好，世界！
// 這是一段中文測試文字。
// 數字：一二三四五
//
// ========================================
//     std::endl vs '\n'（計數證據）
// ========================================
// 使用 std::endl : flush(sync) 次數 = 2 , 輸出字元數 = 12
// 使用 '\n'      : flush(sync) 次數 = 0 , 輸出字元數 = 12
// 結論：輸出內容完全相同（同樣 12 個字元），
//       差別只在 std::endl 每行都強制排空緩衝區。
//
// ========================================
//     UTF-8 位元組 vs 字元
// ========================================
// 原字串        : 張三豐and王五
// size() 位元組數: 18
// 實際字元數    : 8
// 直接 substr(0,8) : 取得 8 個位元組；切點落在字元中間？ 是（產生不完整序列）
// utf8Truncate(,8): "張三" (6 位元組, 2 個字元)
// utf8Truncate(,15): "張三豐and王" (15 位元組, 7 個字元)
//
// ========================================
//     編譯環境測試完成！
// ========================================
