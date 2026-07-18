/*
 * ============================================================
 * 【第 1 課：C++ 的歷史與設計哲學】總複習 summary.cpp
 * ============================================================
 *
 * 本課程重點：
 * 1. C++ 的誕生背景與歷史時間線
 * 2. 創造者 Bjarne Stroustrup 的設計動機
 * 3. 名稱「C++」的由來
 * 4. C++ 的四大設計哲學原則
 * 5. C++ 的多範式程式設計
 * 6. 各種程式設計範式的實際範例
 *
 * ============================================================
 * 學習目的：
 * 理解 C++ 的歷史背景和設計哲學，有助於理解「為什麼 C++
 * 要這樣設計」，而不只是「C++ 能做什麼」。當你了解設計者
 * 的思維，很多看似複雜的語法規則都會變得合理。
 * ============================================================
 */

#include <iostream>
#include <vector>
#include <algorithm>
#include <string>

// ===== 重點一：C++ 的誕生背景 =====
//
// 歷史時間線：
//   1972年：Dennis Ritchie 在貝爾實驗室創造了 C 語言
//   1979年：Bjarne Stroustrup 開始開發「C with Classes」
//   1983年：正式更名為 C++（Rick Mascitti 建議）
//   1985年：第一版《The C++ Programming Language》出版
//   1998年：C++98 成為第一個 ISO 標準
//   2011年：C++11 發布（現代 C++ 的起點）
//   2014年：C++14 發布
//   2017年：C++17 發布
//   2020年：C++20 發布
//   2023年：C++23 發布
//
// 創造動機：
//   Bjarne Stroustrup（丹麥電腦科學家）在博士期間使用 Simula 語言，
//   Simula 是世界上第一個支援「類別」和「物件」的語言。
//   - Simula 的優點：程式結構清晰，易於組織大型程式
//   - Simula 的缺點：執行效率太慢
//   - C 語言：執行效率極高，但缺乏組織大型程式的機制
//
//   Stroustrup 的核心想法：
//   「能不能把 Simula 的組織能力加到 C 語言上，同時保持 C 的效率？」
//   這就是 C++ 誕生的核心動機。


// ===== 重點二：名稱「C++」的由來 =====
//
// 原名「C with Classes」（帶有類別的 C）
// 1983年更名為「C++」
//
// 名稱來自 C 語言的遞增運算子 ++，意思是「C 的下一個版本」：
//   int c = 1;
//   c++;  // c 變成 2，代表「進化後的 C」
//
// 有趣的是：按照 C 語言語義，c++ 是「先使用 c 的值，再遞增」
// 所以有人開玩笑說應該叫「++C」才對！
// 意思是：「C++ 是先以 C 為基礎，然後再進化」


// ===== 重點三：C++ 的四大設計哲學原則 =====

// 原則一：零開銷原則（Zero-overhead Principle）
// 「不為你不用的東西付出代價」
// 如果你不使用某個功能，它不應該對程式產生任何額外開銷
// （無論是執行時間還是記憶體）

// 示範：你可以只用 C 風格，不會有 OOP 的開銷
int simpleAdd(int a, int b) {
    return a + b;  // 純 C 風格函數，零 OOP 開銷
}

// 只有當你選擇使用類別時，才會有類別的機制
class SimpleCalculator {
private:
    int lastResult;  // 只有使用時才有此開銷
public:
    SimpleCalculator() : lastResult(0) {}

    int multiply(int a, int b) {
        lastResult = a * b;
        return lastResult;
    }

    int getLastResult() const {
        return lastResult;
    }
};

// 原則二：直接映射硬體（Direct Mapping to Hardware）
// C++ 的抽象機制不應該阻止程式設計師直接操作硬體
// 保留了 C 的底層能力：指標、直接存取記憶體、位元操作
void demonstrateHardwareMapping() {
    int value = 42;
    int* ptr = &value;       // 指標操作：直接取得記憶體位址
    *ptr = 100;              // 直接修改記憶體中的值

    unsigned char flags = 0b00001111;  // 二進位初始化
    flags = flags << 2;                // 位元左移操作
    // 這些底層操作在 C++ 中完全保留，不受任何限制
}

// 原則三：C 的相容性（C Compatibility）
// C++ 被設計為 C 的超集，大部分合法的 C 程式碼也是合法的 C++ 程式碼
// 這讓已有的大量 C 程式碼可以逐步遷移到 C++，而不是完全重寫

// 原則四：提供高階抽象，但不強制使用
// 類別、繼承、多型、模板等高階功能都是可選的
// 你可以根據需要選擇使用，不需要全部用上


// ===== 重點四：C++ 的多範式程式設計 =====
//
// C++ 支援多種程式設計範式（programming paradigms）：
//
// 1. 程序式（Procedural）：使用函數組織程式碼，類似 C
//    → 函數、流程控制、迴圈
//
// 2. 物件導向（Object-Oriented）：使用類別和物件組織程式碼
//    → class、繼承（inheritance）、多型（polymorphism）
//
// 3. 泛型（Generic）：使用模板寫出適用於多種類型的程式碼
//    → template<typename T>
//
// 4. 函數式（Functional）：使用函數作為一等公民
//    → lambda 表達式、std::function
//
// 這意味著你可以根據問題的性質，選擇最適合的方式來解決問題！


// 範式一：程序式風格（Procedural Style）
// 這就是純 C 風格的函數，沒有任何物件導向的元素
// C++ 完全支援這種寫法
int proceduralAdd(int a, int b) {
    return a + b;
}

// 範式二：物件導向風格（Object-Oriented Style）
// 展示：class 關鍵字、private/public 存取控制、建構函數、成員函數
class Calculator {
private:
    int lastResult;  // 私有成員變數，外界無法直接存取

public:
    // 建構函數：使用初始化列表初始化成員
    // 初始化列表語法「: lastResult(0)」比在函數體內賦值更高效
    Calculator() : lastResult(0) {}

    int multiply(int a, int b) {
        lastResult = a * b;
        return lastResult;
    }

    // const 成員函數：保證不會修改物件的狀態
    int getLastResult() const {
        return lastResult;
    }
};

// 範式三：泛型風格（Generic Style）
// template<typename T> 讓函數可以接受任何類型
// 只要該類型支援 > 運算子即可
// 這樣就不需要為 int、double、long 等各自寫一個版本
template<typename T>
T getMax(T a, T b) {
    return (a > b) ? a : b;
}

// 範式四：函數式風格（Functional Style）
// Lambda 表達式是一個「匿名函數」，可以直接傳遞給其他函數使用
// 語法：[捕獲列表](參數列表) { 函數體 }
// 範例：[](int a, int b) { return a < b; }
// 用於 std::sort 的比較函數

// ===== 主程式：展示所有範式 =====
int main() {
    std::cout << "=== C++ 多範式程式設計展示 ===" << std::endl;
    std::cout << std::endl;

    // --------------------------------------------------
    // 1. 程序式風格
    // --------------------------------------------------
    std::cout << "[程序式風格]" << std::endl;
    // 直接呼叫函數，沒有任何物件的概念
    int sum = proceduralAdd(3, 5);
    std::cout << "proceduralAdd(3, 5) = " << sum << std::endl;
    std::cout << std::endl;

    // --------------------------------------------------
    // 2. 物件導向風格
    // --------------------------------------------------
    std::cout << "[物件導向風格]" << std::endl;
    // 建立物件（Calculator 的實例）
    Calculator calc;
    // 透過物件呼叫成員函數
    int product = calc.multiply(4, 6);
    std::cout << "calc.multiply(4, 6) = " << product << std::endl;
    std::cout << "calc.getLastResult() = " << calc.getLastResult() << std::endl;
    std::cout << std::endl;

    // --------------------------------------------------
    // 3. 泛型風格
    // --------------------------------------------------
    std::cout << "[泛型風格]" << std::endl;
    // 同一個函數模板，自動推導類型
    // 當傳入 int 時，T 被推導為 int
    std::cout << "getMax(10, 20) = " << getMax(10, 20) << std::endl;
    // 當傳入 double 時，T 被推導為 double
    std::cout << "getMax(3.14, 2.71) = " << getMax(3.14, 2.71) << std::endl;
    // 甚至可以用於字元比較
    std::cout << "getMax('a', 'z') = " << getMax('a', 'z') << std::endl;
    std::cout << std::endl;

    // --------------------------------------------------
    // 4. 函數式風格
    // --------------------------------------------------
    std::cout << "[函數式風格]" << std::endl;
    std::vector<int> numbers = {5, 2, 8, 1, 9, 3};

    // Lambda 表達式：[](int a, int b) { return a < b; }
    // 這是一個「匿名函數」，作為排序的比較準則
    // 意義：當 a < b 時返回 true，表示 a 排在 b 前面（升序）
    std::sort(numbers.begin(), numbers.end(), [](int a, int b) {
        return a < b;  // 升序排列
    });

    std::cout << "排序後的數字: ";
    // 範圍 for 迴圈（C++11）：遍歷容器的每個元素
    for (int n : numbers) {
        std::cout << n << " ";
    }
    std::cout << std::endl;
    std::cout << std::endl;

    // --------------------------------------------------
    // 5. 零開銷原則示範
    // --------------------------------------------------
    std::cout << "[零開銷原則示範]" << std::endl;
    // 只用程序式，沒有 OOP 開銷
    std::cout << "simpleAdd(7, 8) = " << simpleAdd(7, 8) << std::endl;

    // 使用 OOP，但只在需要時才付出代價
    SimpleCalculator sc;
    std::cout << "sc.multiply(3, 7) = " << sc.multiply(3, 7) << std::endl;
    std::cout << std::endl;

    // --------------------------------------------------
    // 6. 硬體映射示範
    // --------------------------------------------------
    std::cout << "[硬體映射示範]" << std::endl;
    int value = 42;
    int* ptr = &value;
    std::cout << "value = " << value << std::endl;
    std::cout << "ptr 指向位址: " << ptr << std::endl;
    *ptr = 100;  // 透過指標直接修改記憶體
    std::cout << "透過指標修改後 value = " << value << std::endl;

    unsigned char flags = 0b00001111;  // 二進位：00001111 = 15
    std::cout << "flags 初始值: " << (int)flags << std::endl;
    flags = flags << 2;  // 左移兩位：00111100 = 60
    std::cout << "左移兩位後 flags: " << (int)flags << std::endl;

    return 0;
}

/*
 * ============================================================
 * 本課重點回顧表
 * ============================================================
 *
 * 概念              說明
 * ────────────────  ─────────────────────────────────────────
 * C++ 創造者        Bjarne Stroustrup（1979年開始開發）
 * 設計動機          結合 C 的效率和 Simula 的組織能力
 * 名稱由來          ++ 是 C 的遞增運算子，表示「C 的進化」
 * 零開銷原則        不使用的功能不產生開銷
 * 硬體映射          保留指標、位元操作等底層能力
 * C 相容性          大部分 C 程式碼也是合法的 C++ 程式碼
 * 多範式            支援程序式、物件導向、泛型、函數式
 *
 * ============================================================
 * 思考問題
 * ============================================================
 * 1. 為什麼 Stroustrup 選擇在 C 的基礎上擴展，而不是創造全新語言？
 *    → 保持向後相容性，讓已有的大量 C 程式碼可以繼續使用
 *    → 繼承 C 的高效能和底層控制能力
 *
 * 2. 「零開銷原則」對程式設計師有什麼實際意義？
 *    → 可以按需要選擇使用功能，不用為未使用的功能付代價
 *    → 適合嵌入式系統等資源受限的環境
 *
 * 3. 多範式語言有什麼優點和缺點？
 *    → 優點：靈活，可以選擇最適合的方式解決問題
 *    → 缺點：學習曲線較陡，可能導致風格不一致
 * ============================================================
 */
