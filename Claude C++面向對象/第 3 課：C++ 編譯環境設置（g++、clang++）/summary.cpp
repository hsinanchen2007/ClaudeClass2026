/*
 * ================================================================
 * 【第3課：C++ 編譯環境設置（g++、clang++）】總複習 summary.cpp
 * ================================================================
 * 編譯方式：g++ -std=c++17 -Wall -Wextra -o summary summary.cpp
 *
 * 本課重點：
 * 1. 三大主流 C++ 編譯器：g++、clang++、MSVC（cl）
 * 2. g++ 常用編譯選項（-std、-Wall、-Wextra、-g、-O2 等）
 * 3. MSVC 常用選項（/std:c++17、/W4、/Zi、/utf-8 等）
 * 4. 編譯器自動偵測巨集（__clang__、__GNUC__、_MSC_VER）
 * 5. C++ 標準版本偵測（__cplusplus）
 * 6. 跨平台作業系統偵測（_WIN32、__linux__、__APPLE__）
 * 7. 多檔案編譯概念（分開編譯再連結）
 * 8. 常見編譯錯誤與解決方法
 * ================================================================
 */

#include <iostream>
#include <string>
#include <vector>

// ===== 重點一：編譯器識別巨集 =====
// 說明：C++ 編譯器在編譯時會預先定義特定的巨集，讓程式碼
//       可以在執行前（編譯期）判斷使用的是哪個編譯器。
// 重要性：讓同一份原始碼能針對不同編譯器做不同處理，
//         例如使用某編譯器特有的最佳化選項或警告抑制。
// 使用方式：搭配 #if defined(...)  #elif  #else  #endif 使用。

void showCompilerInfo() {
    std::cout << "========================================\n";
    std::cout << "    [重點一] 編譯器識別\n";
    std::cout << "========================================\n";

    // __clang__ 由 Clang/clang++ 定義
    // __GNUC__  由 GCC/g++ 定義（注意 Clang 也會定義此值以保持相容性，
    //           所以必須先判斷 __clang__）
    // _MSC_VER  由 MSVC/cl 定義，其值為版本號（例如 1940 代表 VS 2022）
#if defined(__clang__)
    std::cout << "編譯器: Clang " << __clang_major__ << "."
              << __clang_minor__ << "." << __clang_patchlevel__ << "\n";
#elif defined(__GNUC__)
    std::cout << "編譯器: GCC " << __GNUC__ << "."
              << __GNUC_MINOR__ << "." << __GNUC_PATCHLEVEL__ << "\n";
#elif defined(_MSC_VER)
    std::cout << "編譯器: MSVC " << _MSC_VER << "\n";
    // 注意：MSVC 預設 __cplusplus 值是 199711，要正確反映版本
    // 需加上 /Zc:__cplusplus 編譯選項
#else
    std::cout << "編譯器: Unknown\n";
#endif
}

// ===== 重點二：C++ 標準版本偵測 =====
// 說明：__cplusplus 是一個特殊巨集，其值代表正在使用的 C++ 標準。
//       C++11=201103L、C++14=201402L、C++17=201703L、C++20=202002L
// 重要性：可在程式碼中根據標準版本條件性地啟用新功能，
//         確保向後相容性。
// 使用方式：g++ 需加 -std=c++17；MSVC 需加 /std:c++17 /Zc:__cplusplus

void showCppStandard() {
    std::cout << "\n[重點二] C++ 標準版本: ";

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
    std::cout << "C++98/03";
#else
    std::cout << "Pre-C++98";
#endif

    std::cout << "  (__cplusplus = " << __cplusplus << ")\n";
}

// ===== 重點三：作業系統偵測 =====
// 說明：編譯器根據目標平台自動定義對應的巨集。
//       _WIN32 / _WIN64 → Windows（32位或64位）
//       __linux__        → Linux
//       __APPLE__        → macOS / iOS
// 重要性：撰寫跨平台程式碼時，可針對不同 OS 執行不同邏輯，
//         例如路徑分隔符、API 呼叫、文字編碼設定等。

void showPlatformInfo() {
    std::cout << "\n[重點三] 作業系統: ";

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

    std::cout << "\n";
}

// ===== 重點四：常用 g++ 編譯選項速查 =====
// 說明：正確使用編譯選項可以及早發現 bug 並控制最佳化程度。
//
// 開發階段建議指令：
//   g++ -std=c++17 -Wall -Wextra -g -O0 程式.cpp -o 程式
//
//   -std=c++17  → 使用 C++17 標準
//   -Wall       → 啟用大部分常見警告
//   -Wextra     → 啟用額外的警告（如未使用參數）
//   -g          → 加入除錯資訊（GDB 可用）
//   -O0         → 停用最佳化，方便除錯
//
// 發布階段建議指令：
//   g++ -std=c++17 -Wall -Wextra -O2 程式.cpp -o 程式
//
//   -O2         → 啟用中等最佳化，平衡速度與二進位大小
//
// MSVC 等效選項：
//   /std:c++17  → 指定 C++ 標準
//   /W4         → 警告等級 4（相當於 -Wall -Wextra）
//   /Zi         → 加入除錯資訊
//   /Od         → 停用最佳化
//   /O2         → 速度最佳化
//   /utf-8      → 原始碼與執行期使用 UTF-8（處理中文必備）
//   /EHsc       → 啟用 C++ 例外處理

// ===== 重點五：C++11 以後的基礎新功能（測試編譯環境） =====
// 說明：以下功能可用來驗證編譯環境是否正確支援現代 C++。
//       若這些程式碼無法編譯，請確認 -std=c++11 或以上版本。

void testModernCppFeatures() {
    std::cout << "\n========================================\n";
    std::cout << "    [重點五] 現代 C++ 功能驗證\n";
    std::cout << "========================================\n";

    // auto 型別推導（C++11）
    // 重要性：減少冗長的型別宣告，讓程式碼更簡潔
    auto number = 42;
    auto pi = 3.14159;
    std::cout << "auto 推導 int: " << number << "\n";
    std::cout << "auto 推導 double: " << pi << "\n";

    // 範圍式 for 迴圈（C++11）
    // 重要性：更安全、更易讀地遍歷容器，無需手動管理索引
    std::vector<int> vec = {10, 20, 30, 40, 50};
    std::cout << "範圍 for 遍歷 vector: ";
    for (const auto& v : vec) {
        std::cout << v << " ";
    }
    std::cout << "\n";

    // Lambda 表達式（C++11）
    // 重要性：可就地定義匿名函式，常用於排序、演算法、回呼等場景
    auto multiply = [](int a, int b) -> int { return a * b; };
    std::cout << "lambda 計算 6 * 7 = " << multiply(6, 7) << "\n";

    // 初始化列表（C++11）
    // 重要性：更一致的物件初始化語法
    std::vector<std::string> fruits = {"蘋果", "香蕉", "橘子"};
    std::cout << "水果清單: ";
    for (const auto& fruit : fruits) {
        std::cout << fruit << " ";
    }
    std::cout << "\n";
}

// ===== 重點六：多檔案編譯概念 =====
// 說明：當程式變大，通常將程式碼拆分為多個 .cpp 與 .h 檔案。
//       編譯流程：
//         1. 前置處理（展開 #include、#define）
//         2. 編譯（.cpp → .o 目標檔）
//         3. 連結（多個 .o → 執行檔）
//
// 一次編譯多個 .cpp：
//   g++ -std=c++17 main.cpp calculator.cpp -o program
//
// 分步編譯（適合大型專案，只重新編譯修改過的檔案）：
//   g++ -std=c++17 -c main.cpp -o main.o
//   g++ -std=c++17 -c calculator.cpp -o calculator.o
//   g++ main.o calculator.o -o program
//
// 標頭檔保護（防止重複 include）：
//   #ifndef CALCULATOR_H
//   #define CALCULATOR_H
//   // ... 宣告內容 ...
//   #endif
//
// 或現代寫法（C++11 以後主流編譯器支援）：
//   #pragma once

// ===== 重點七：常見編譯錯誤速查 =====
// 說明：
//
// 錯誤一：'g++' is not recognized...
//   原因：編譯器未安裝或路徑未加入 PATH 環境變數
//   解法：重新安裝編譯器，並確認 PATH 設定
//
// 錯誤二：fatal error: iostream: No such file or directory
//   原因：編譯器安裝不完整，或 include 路徑設定錯誤
//   解法：重新安裝完整的 g++ 工具鏈
//
// 錯誤三：undefined reference to `function_name'
//   原因：宣告了函式但編譯時沒有包含對應的 .cpp 實作檔
//   解法：確認所有 .cpp 都有被列入編譯指令
//
// 錯誤四：error: 'auto' changes meaning in C++11
//   原因：未指定 C++ 標準，預設為舊版本
//   解法：加上 -std=c++11（或更高版本）

int main() {
    // 設定 Windows 控制台 UTF-8 輸出（編譯時加 /utf-8 或執行前設定）
#ifdef _WIN32
    // 若輸出中文出現亂碼，可取消以下兩行的註解：
    // #include <windows.h>
    // SetConsoleOutputCP(65001);
#endif

    std::cout << "================================================================\n";
    std::cout << "  第3課：C++ 編譯環境設置 總複習\n";
    std::cout << "================================================================\n";

    showCompilerInfo();
    showCppStandard();
    showPlatformInfo();
    testModernCppFeatures();

    std::cout << "\n================================================================\n";
    std::cout << "  本課重點速查\n";
    std::cout << "================================================================\n";
    std::cout << "  開發編譯：g++ -std=c++17 -Wall -Wextra -g -O0 程式.cpp -o 程式\n";
    std::cout << "  發布編譯：g++ -std=c++17 -Wall -Wextra -O2 程式.cpp -o 程式\n";
    std::cout << "  MSVC開發：cl /std:c++17 /utf-8 /EHsc /W4 /Zi 程式.cpp\n";
    std::cout << "  多檔案：  g++ -std=c++17 main.cpp other.cpp -o program\n";
    std::cout << "================================================================\n";

    return 0;
}
