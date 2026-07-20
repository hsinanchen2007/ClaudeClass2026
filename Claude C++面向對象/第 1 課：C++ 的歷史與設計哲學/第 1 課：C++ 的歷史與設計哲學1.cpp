/*
# 第 1 課：C++ 的歷史與設計哲學

---

## 1.1 為什麼要學習 C++ 的歷史？

在深入學習 C++ 的語法和技術之前，理解它的歷史背景和設計哲學是非常重要的。這能幫助你明白「為什麼 C++ 要這樣設計」，而不只是「C++ 能做什麼」。

當你理解了設計者的思維，很多看似複雜或奇怪的語法規則都會變得合理。

---

## 1.2 C++ 的誕生背景

### 時間線

| 年份 | 事件 |
|------|------|
| 1972 | Dennis Ritchie 在貝爾實驗室創造了 C 語言 |
| 1979 | Bjarne Stroustrup 開始開發「C with Classes」 |
| 1983 | 正式更名為 C++ |
| 1985 | 第一版《The C++ Programming Language》出版 |
| 1998 | C++98 成為第一個 ISO 標準 |
| 2011 | C++11 發布（現代 C++ 的起點） |
| 2014 | C++14 發布 |
| 2017 | C++17 發布 |
| 2020 | C++20 發布 |
| 2023 | C++23 發布 |

### 創造者：Bjarne Stroustrup

Bjarne Stroustrup 是丹麥電腦科學家，當時在貝爾實驗室工作。他在博士研究期間使用過一種叫做 **Simula** 的語言，這是世界上第一個支援「類別」和「物件」概念的語言。

Simula 的優點是程式結構清晰、易於組織大型程式，但缺點是執行效率太慢。

另一方面，C 語言執行效率極高，但缺乏組織大型程式的機制。

Stroustrup 的想法很簡單：**能不能把 Simula 的組織能力加到 C 語言上，同時保持 C 的效率？**

這就是 C++ 誕生的核心動機。

---

## 1.3 名稱的由來

最初這個語言叫做 **「C with Classes」**（帶有類別的 C）。

1983 年，Rick Mascitti 建議改名為 **C++**。

這個名稱來自 C 語言的遞增運算子 `++`，意思是「C 的下一個版本」或「C 的進化」。

```c
int c = 1;
c++;  // c 變成 2
```

有趣的是，按照 C 語言的語義，`c++` 是「先使用 c 的值，再遞增」，所以有人開玩笑說 C++ 應該叫 `++C` 才對！

---

## 1.4 C++ 的設計哲學

Bjarne Stroustrup 在設計 C++ 時，有幾個核心原則：

### 原則一：不為你不用的東西付出代價（Zero-overhead Principle）

這是 C++ 最重要的設計原則。

意思是：如果你不使用某個功能，它不應該對你的程式產生任何額外的開銷（無論是執行時間還是記憶體）。

```cpp
// 如果你只需要簡單的 C 風格程式碼
// C++ 不會強迫你付出 OOP 的開銷
int add(int a, int b) {
    return a + b;
}

// 只有當你選擇使用類別時，才會有類別的機制
class Calculator {
public:
    int add(int a, int b) {
        return a + b;
    }
};
```

### 原則二：直接映射硬體（Direct Mapping to Hardware）

C++ 的抽象機制不應該阻止程式設計師直接操作硬體。你仍然可以使用指標、直接存取記憶體、進行位元操作。

```cpp
// C++ 保留了 C 的底層能力
int value = 42;
int* ptr = &value;       // 指標操作
*ptr = 100;              // 直接修改記憶體

unsigned char flags = 0b00001111;
flags = flags << 2;      // 位元操作
```

### 原則三：C 的相容性（C Compatibility）

C++ 被設計為 C 的超集。大部分合法的 C 程式碼也是合法的 C++ 程式碼。

這個設計讓已有的大量 C 程式碼可以逐步遷移到 C++，而不是完全重寫。

```cpp
// 這段程式碼在 C 和 C++ 中都能編譯運行
#include <stdio.h>

int main() {
    printf("Hello from C style!\n");
    return 0;
}
```

### 原則四：提供高階抽象，但不強制使用

C++ 提供了類別、繼承、多型、模板等高階功能，但這些都是可選的。你可以根據需要選擇使用。

---

## 1.5 C++ 是多範式語言

C++ 支援多種程式設計範式（programming paradigms）：

| 範式 | 說明 | 範例 |
|------|------|------|
| 程序式（Procedural） | 使用函數組織程式碼，類似 C | 函數、流程控制 |
| 物件導向（Object-Oriented） | 使用類別和物件組織程式碼 | class、繼承、多型 |
| 泛型（Generic） | 使用模板寫出適用於多種類型的程式碼 | template |
| 函數式（Functional） | 使用函數作為一等公民 | lambda、std::function |

這意味著你可以根據問題的性質，選擇最適合的方式來解決問題。

---

## 1.6 完整範例程式

讓我們寫一個程式，展示 C++ 如何在一個程式中融合不同的程式設計風格：

```cpp
#include <iostream>
#include <vector>
#include <algorithm>

// ========================================
// 程序式風格（Procedural Style）
// ========================================
int add(int a, int b) {
    return a + b;
}

// ========================================
// 物件導向風格（Object-Oriented Style）
// ========================================
class Calculator {
private:
    int lastResult;  // 私有成員變數
    
public:
    Calculator() : lastResult(0) {}  // 建構函數
    
    int multiply(int a, int b) {
        lastResult = a * b;
        return lastResult;
    }
    
    int getLastResult() const {
        return lastResult;
    }
};

// ========================================
// 泛型風格（Generic Style）
// ========================================
template<typename T>
T getMax(T a, T b) {
    return (a > b) ? a : b;
}

// ========================================
// 主程式
// ========================================
int main() {
    std::cout << "=== C++ 多範式展示 ===" << std::endl;
    std::cout << std::endl;
    
    // 1. 程序式風格
    std::cout << "[程序式風格]" << std::endl;
    int sum = add(3, 5);
    std::cout << "add(3, 5) = " << sum << std::endl;
    std::cout << std::endl;
    
    // 2. 物件導向風格
    std::cout << "[物件導向風格]" << std::endl;
    Calculator calc;
    int product = calc.multiply(4, 6);
    std::cout << "calc.multiply(4, 6) = " << product << std::endl;
    std::cout << "calc.getLastResult() = " << calc.getLastResult() << std::endl;
    std::cout << std::endl;
    
    // 3. 泛型風格
    std::cout << "[泛型風格]" << std::endl;
    std::cout << "getMax(10, 20) = " << getMax(10, 20) << std::endl;
    std::cout << "getMax(3.14, 2.71) = " << getMax(3.14, 2.71) << std::endl;
    std::cout << std::endl;
    
    // 4. 函數式風格（使用 lambda）
    std::cout << "[函數式風格]" << std::endl;
    std::vector<int> numbers = {5, 2, 8, 1, 9, 3};
    
    // 使用 lambda 表達式排序
    std::sort(numbers.begin(), numbers.end(), [](int a, int b) {
        return a < b;  // 升序排列
    });
    
    std::cout << "排序後的數字: ";
    for (int n : numbers) {
        std::cout << n << " ";
    }
    std::cout << std::endl;
    
    return 0;
}
```

### 編譯與執行

```bash
g++ -std=c++11 -o lesson01 lesson01.cpp
./lesson01
```

### 執行結果

```
=== C++ 多範式展示 ===

[程序式風格]
add(3, 5) = 8

[物件導向風格]
calc.multiply(4, 6) = 24
calc.getLastResult() = 24

[泛型風格]
getMax(10, 20) = 20
getMax(3.14, 2.71) = 3.14

[函數式風格]
排序後的數字: 1 2 3 5 8 9
```

---

## 1.7 程式碼解析

### 程序式部分

```cpp
int add(int a, int b) {
    return a + b;
}
```

這就是純 C 風格的函數，沒有任何物件導向的元素。C++ 完全支援這種寫法。

### 物件導向部分

```cpp
class Calculator {
private:
    int lastResult;
    
public:
    Calculator() : lastResult(0) {}
    
    int multiply(int a, int b) {
        lastResult = a * b;
        return lastResult;
    }
};
```

這裡展示了：
- `class` 關鍵字定義類別
- `private` 和 `public` 控制存取權限
- 建構函數初始化物件
- 成員函數操作資料

這些概念我們在後續課程會詳細講解。

### 泛型部分

```cpp
template<typename T>
T getMax(T a, T b) {
    return (a > b) ? a : b;
}
```

這個函數模板可以接受任何類型（int、double、甚至自定義類別），只要該類型支援 `>` 運算子。

### 函數式部分

```cpp
std::sort(numbers.begin(), numbers.end(), [](int a, int b) {
    return a < b;
});
```

`[](int a, int b) { return a < b; }` 是一個 **lambda 表達式**，它是一個匿名函數，可以直接傳遞給其他函數使用。

---

## 1.8 本課重點回顧

| 概念 | 說明 |
|------|------|
| C++ 創造者 | Bjarne Stroustrup（1979 年開始開發） |
| 設計動機 | 結合 C 的效率和 Simula 的組織能力 |
| 名稱由來 | `++` 是 C 的遞增運算子，表示「C 的進化」 |
| 零開銷原則 | 不使用的功能不產生開銷 |
| 多範式 | 支援程序式、物件導向、泛型、函數式 |
| C 相容性 | 大部分 C 程式碼也是合法的 C++ 程式碼 |

---

## 1.9 思考問題

1. 為什麼 Stroustrup 選擇在 C 的基礎上擴展，而不是創造一個全新的語言？
2. 「零開銷原則」對程式設計師有什麼實際意義？
3. 多範式語言有什麼優點和缺點？

---

準備好了嗎？下一課我們將深入探討 **C 與 C++ 的關鍵差異**，幫助你從 C 程式設計師順利過渡到 C++ 程式設計師。

要繼續進入 **第 2 課** 嗎？
*/



// =============================================================================
//  第 1 課 -1  —  C++ 的歷史與設計哲學（可執行的多範式示範）
// =============================================================================
//
// 【主題資訊 Information】
//   本檔上半部是第 1 課的完整講義（以區塊註解保存），
//   下半部是可執行的四範式對照：程序式／物件導向／泛型／函數式。
//   關鍵人物：Bjarne Stroustrup（1979 年於貝爾實驗室開始 "C with Classes"）
//   關鍵年份：1972 C / 1979 C with Classes / 1983 更名 C++ /
//             1998 C++98（首個 ISO 標準）/ 2011 C++11（現代 C++ 分水嶺）
//   標頭檔：  <iostream>、<vector>、<algorithm>
//   本檔標準：使用 lambda 與 auto → 需 C++11 起；示範以 C++17 編譯。
//
// 【詳細解釋 Explanation】
//
// 【1. C++ 誕生的核心矛盾】
//   Stroustrup 的博士研究用 Simula 寫模擬程式 ——
//   類別與繼承讓他能清楚表達複雜系統，但**速度太慢**，跑不動真實規模。
//   改用接近機器的 BCPL 重寫後速度夠了，**抽象能力卻全失**。
//   C++ 就是在回答這個矛盾：
//       「能不能同時擁有 Simula 的表達力，與 C 的執行效率？」
//   這一句話解釋了 C++ 幾乎所有的設計取捨。
//
// 【2. 四大設計哲學】
//   (a) 零開銷抽象：「不用的不收費；用了的，你手寫也快不了。」
//   (b) 信任程式設計師：允許危險操作，代價是語言不會攔住你的錯誤。
//   (c) 直接映射硬體：指標對應位址、位元運算對應 CPU 指令。
//   (d) 多範式：不強迫單一風格，依問題選工具。
//
// 【3. 本檔四種範式各自的特徵】
//     程序式   add(a, b)          —— 資料與函式分離，最接近 C
//     物件導向 Calculator 類別    —— 資料與行為封裝在一起，有狀態
//     泛型     getMax<T>()        —— 一份原始碼、編譯期實體化成多份機器碼
//     函數式   sort + lambda      —— 行為當成值傳遞，強調轉換而非變更
//   關鍵認知：**四者沒有優劣之分**，只有適不適合當前問題。
//   有狀態要維護 → 物件導向；純資料轉換 → 函數式；
//   型別無關的演算法 → 泛型；簡單的一次性運算 → 程序式。
//
// 【概念補充 Concept Deep Dive】
//   泛型的零開銷是**編譯期實體化**達成的：
//   getMax<int> 與 getMax<double> 會各自產生一份獨立、針對該型別最佳化的
//   機器碼，執行期沒有任何型別判斷或裝箱（boxing）。
//   這與 Java 的 type erasure 是根本不同的路線 ——
//   Java 泛型在編譯後被抹除、執行期都是 Object，有裝箱與轉型成本。
//   C++ 的代價則是 code bloat（二進位變大）與編譯時間變長。
//
//   函數式風格在本檔用 lambda 表達。lambda 是 C++11 引入的，
//   本質上是編譯器自動產生的一個**匿名 function object（仿函式）**——
//   它有 operator()，可以被 inline，因此通常比 C 的函式指標**更快**
//   （函式指標無法內聯，lambda 可以）。這又是一個零開銷抽象的實例。
//
// 【注意事項 Pay Attention】
//   1. 「零開銷」＝「不用不收費、用了不比手寫差」，
//      **不是**「所有特性都免費」。virtual、例外、RTTI 都有真實成本。
//   2. C++ **不是** C 的超集：C 允許 void* 隱式轉 T*（malloc 直接賦值），
//      C++ 不允許；C99 的 VLA 也不在 C++ 標準內。
//   3. 多範式是「依情境選擇」，不是「全部混用」。
//      同一模組風格不一致會讓維護者要載入多套心智模型。
//   4. 本檔的 Calculator::getLastResult() 等唯讀函式若加上 const，
//      才能被 const 物件與 const 引用使用。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】C++ 的歷史與設計哲學
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 什麼是「零開銷抽象」？virtual 函式算不算零開銷？
//     答：Stroustrup 的定義是「你不使用的東西不付出代價；
//         你使用的東西，不可能手寫得更好」。
//         virtual **符合**零開銷原則，但**不是免費**：
//         它有一次間接跳轉與每物件一個 vptr 的成本 ——
//         然而你若在 C 裡自己實作動態分派（函式指標表），成本一模一樣。
//         零開銷保證的是「不浪費」，不是「不花錢」。
//     追問：那哪些東西是「不用就不收費」的？
//         → 沒有 virtual 函式的類別不會有 vptr；
//           沒寫 try/catch 的程式在正常路徑上不為例外付出成本。
//
// 🔥 Q2. C++ 是 C 的超集嗎？
//     答：**不是**。雖然相容性極高，但有效的 C 程式未必是有效的 C++。
//         最典型的例子：C 允許 void* 隱式轉成 T*，
//         所以 int* p = malloc(n); 在 C 合法、在 C++ 是編譯錯誤；
//         C99 的 VLA（變長陣列）也不在 C++ 標準內。
//     追問：那為什麼大家常這樣說？
//         → 因為設計目標就是「盡可能相容」，
//           實務上絕大多數 C 程式碼確實能直接編譯，口語上被簡化了。
//
// ⚠️ 陷阱. 「C++」這個名字，照 C++ 的運算子語意讀，它的值是多少？
//     答：還是 **C**。因為 ++ 在這裡是**後置**遞增，
//         語意是「先回傳舊值，再遞增」——
//         所以這個運算式的值是遞增前的 C。
//         要表達「先進化再使用」，該寫成 ++C。
//     為什麼會錯：大家把 C++ 當成一個專有名詞直接讀成「C 加強版」，
//         沒有把它當作真正的運算式去解析。
//         這題實際考的是前置／後置遞增的語意差異。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <vector>
#include <algorithm>
#include <numeric>
#include <string>

// ========================================
// 程序式風格（Procedural Style）
// ========================================
int add(int a, int b) {
    return a + b;
}

// ========================================
// 物件導向風格（Object-Oriented Style）
// ========================================
class Calculator {
private:
    int lastResult;  // 私有成員變數
    
public:
    Calculator() : lastResult(0) {}  // 建構函數
    
    int multiply(int a, int b) {
        lastResult = a * b;
        return lastResult;
    }
    
    int getLastResult() const {
        return lastResult;
    }
};

// ========================================
// 泛型風格（Generic Style）
// ========================================
template<typename T>
T getMax(T a, T b) {
    return (a > b) ? a : b;
}

// ========================================
// 主程式
// ========================================

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 1512. Number of Good Pairs
//   題目：給一個整數陣列，回傳「好數對」的個數 ——
//         好數對定義為 i < j 且 nums[i] == nums[j]。
//   為什麼用到本主題：這題夠簡單，卻剛好能用三種範式各寫一遍，
//         把本課「多範式」講的東西變成可執行的對照：
//           程序式 → 雙層迴圈暴力比對，最直覺，O(n^2)
//           物件導向 → 用一個維護計數狀態的物件逐一餵入，O(n)
//           函數式 → 先分組計數，再用 accumulate 做組合數加總，O(n)
//         三種寫法答案完全相同，但表達的思考方式截然不同。
//   複雜度：程序式 O(n^2)；物件導向與函數式皆 O(n)。
// -----------------------------------------------------------------------------

// 【範式一：程序式】最直覺的暴力解
int numIdenticalPairs_procedural(const std::vector<int>& nums) {
    int count = 0;
    for (size_t i = 0; i < nums.size(); ++i) {
        for (size_t j = i + 1; j < nums.size(); ++j) {
            if (nums[i] == nums[j]) ++count;
        }
    }
    return count;
}

// 【範式二：物件導向】維護「每個數字已出現幾次」的狀態
//   關鍵洞見：新來的 x 若之前已出現 k 次，就會新增 k 個好數對。
class GoodPairCounter {
    std::vector<int> m_seen;   // m_seen[v] = 數字 v 至今出現次數（題目保證 1..100）
    int m_pairs = 0;
public:
    GoodPairCounter() : m_seen(101, 0) {}

    GoodPairCounter& feed(int v) {
        if (v < 0 || v > 100) return *this;   // 超出假設範圍就略過，維持不變量
        m_pairs += m_seen[v];                 // 與先前每一個相同值都配成一對
        ++m_seen[v];
        return *this;
    }
    int pairs() const { return m_pairs; }
};

int numIdenticalPairs_oop(const std::vector<int>& nums) {
    GoodPairCounter c;
    for (int v : nums) c.feed(v);
    return c.pairs();
}

// 【範式三：函數式】先分組計數，再把每組的 C(k,2) 加總
int numIdenticalPairs_functional(const std::vector<int>& nums) {
    std::vector<int> freq(101, 0);
    std::for_each(nums.begin(), nums.end(), [&freq](int v) {
        if (v >= 0 && v <= 100) ++freq[v];
    });
    // 每組 k 個相同的數字，可組成 k*(k-1)/2 個好數對
    return std::accumulate(freq.begin(), freq.end(), 0,
                           [](int acc, int k) { return acc + k * (k - 1) / 2; });
}

// -----------------------------------------------------------------------------
// 【日常實務範例】設定字串解析：三種範式在真實工作中的分工
//   情境：讀進一行以逗號分隔的功能開關設定，例如
//         "dark_mode=on,telemetry=off,beta_ui=on"，
//         要判斷某個開關是否啟用、並統計共啟用了幾項。
//   為什麼用到本主題：這是最能看出「範式該怎麼選」的日常任務 ——
//     - splitPairs()：純粹的資料轉換（字串 → 結構化資料），
//       沒有需要維護的狀態 → 寫成**自由函式**（程序式／函數式風格）最清楚。
//     - FeatureFlags：需要被反覆查詢、有內部索引要維護 → **物件導向**。
//     - countEnabled()：對現成資料做彙總 → **函數式**（count_if）一行解決。
//   反面示範是把三件事全塞進一個大類別，或全部寫成一長串迴圈 ——
//   前者過度設計，後者難以複用。
// -----------------------------------------------------------------------------
struct FlagPair {              // 純資料聚合 → struct
    std::string key;
    bool        enabled = false;
};

// 函數式／程序式：純轉換，無狀態
std::vector<FlagPair> splitPairs(const std::string& config) {
    std::vector<FlagPair> out;
    size_t start = 0;
    while (start <= config.size()) {
        size_t comma = config.find(',', start);
        if (comma == std::string::npos) comma = config.size();
        std::string token = config.substr(start, comma - start);
        size_t eq = token.find('=');
        if (eq != std::string::npos) {
            out.push_back(FlagPair{token.substr(0, eq), token.substr(eq + 1) == "on"});
        }
        if (comma == config.size()) break;
        start = comma + 1;
    }
    return out;
}

// 物件導向：需要被反覆查詢，且要維護「已解析」這個狀態
class FeatureFlags {
    std::vector<FlagPair> m_flags;
public:
    explicit FeatureFlags(const std::string& config) : m_flags(splitPairs(config)) {}

    bool isEnabled(const std::string& key) const {
        for (const FlagPair& f : m_flags) {
            if (f.key == key) return f.enabled;
        }
        return false;      // 未知的開關一律視為關閉（安全預設）
    }

    const std::vector<FlagPair>& all() const { return m_flags; }
};

// 函數式：對現成資料做彙總，一行
int countEnabled(const std::vector<FlagPair>& flags) {
    return static_cast<int>(std::count_if(flags.begin(), flags.end(),
                                          [](const FlagPair& f) { return f.enabled; }));
}

int main() {
    std::cout << "=== C++ 多範式展示 ===" << std::endl;
    std::cout << std::endl;
    
    // 1. 程序式風格
    std::cout << "[程序式風格]" << std::endl;
    int sum = add(3, 5);
    std::cout << "add(3, 5) = " << sum << std::endl;
    std::cout << std::endl;
    
    // 2. 物件導向風格
    std::cout << "[物件導向風格]" << std::endl;
    Calculator calc;
    int product = calc.multiply(4, 6);
    std::cout << "calc.multiply(4, 6) = " << product << std::endl;
    std::cout << "calc.getLastResult() = " << calc.getLastResult() << std::endl;
    std::cout << std::endl;
    
    // 3. 泛型風格
    std::cout << "[泛型風格]" << std::endl;
    std::cout << "getMax(10, 20) = " << getMax(10, 20) << std::endl;
    std::cout << "getMax(3.14, 2.71) = " << getMax(3.14, 2.71) << std::endl;
    std::cout << std::endl;
    
    // 4. 函數式風格（使用 lambda）
    std::cout << "[函數式風格]" << std::endl;
    std::vector<int> numbers = {5, 2, 8, 1, 9, 3};
    
    // 使用 lambda 表達式排序
    std::sort(numbers.begin(), numbers.end(), [](int a, int b) {
        return a < b;  // 升序排列
    });
    
    std::cout << "排序後的數字: ";
    for (int n : numbers) {
        std::cout << n << " ";
    }
    std::cout << std::endl;
    

    std::cout << "\n[LeetCode 1512. Number of Good Pairs —— 三種範式]" << std::endl;
    std::vector<int> gp{1, 2, 3, 1, 1, 3};
    std::cout << "  輸入: {1,2,3,1,1,3}" << std::endl;
    std::cout << "  程序式(O(n^2))   = " << numIdenticalPairs_procedural(gp) << std::endl;
    std::cout << "  物件導向(O(n))   = " << numIdenticalPairs_oop(gp) << std::endl;
    std::cout << "  函數式(O(n))     = " << numIdenticalPairs_functional(gp) << std::endl;
    std::vector<int> gp2{1, 1, 1, 1};
    std::cout << "  {1,1,1,1} -> " << numIdenticalPairs_functional(gp2)
              << "  (C(4,2)=6)" << std::endl;
    std::vector<int> gp3{1, 2, 3};
    std::cout << "  {1,2,3}   -> " << numIdenticalPairs_functional(gp3)
              << "  (沒有重複)" << std::endl;

    std::cout << "\n[日常實務：功能開關設定解析 —— 依情境選範式]" << std::endl;
    FeatureFlags flags("dark_mode=on,telemetry=off,beta_ui=on,legacy=off");
    std::cout << "  dark_mode 啟用? " << std::boolalpha << flags.isEnabled("dark_mode") << std::endl;
    std::cout << "  telemetry 啟用? " << flags.isEnabled("telemetry") << std::endl;
    std::cout << "  未知開關 unknown_flag 啟用? " << flags.isEnabled("unknown_flag")
              << "  (安全預設為關閉)" << std::endl;
    std::cout << "  共解析出 " << flags.all().size() << " 項，其中啟用 "
              << countEnabled(flags.all()) << " 項" << std::endl;
    for (const FlagPair& f : flags.all()) {
        std::cout << "    " << f.key << " = " << (f.enabled ? "on" : "off") << std::endl;
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 1 課：C++ 的歷史與設計哲學1.cpp" -o lesson1

// === 預期輸出 ===
// === C++ 多範式展示 ===
// 
// [程序式風格]
// add(3, 5) = 8
// 
// [物件導向風格]
// calc.multiply(4, 6) = 24
// calc.getLastResult() = 24
// 
// [泛型風格]
// getMax(10, 20) = 20
// getMax(3.14, 2.71) = 3.14
// 
// [函數式風格]
// 排序後的數字: 1 2 3 5 8 9
// 
// [LeetCode 1512. Number of Good Pairs —— 三種範式]
//   輸入: {1,2,3,1,1,3}
//   程序式(O(n^2))   = 4
//   物件導向(O(n))   = 4
//   函數式(O(n))     = 4
//   {1,1,1,1} -> 6  (C(4,2)=6)
//   {1,2,3}   -> 0  (沒有重複)
// 
// [日常實務：功能開關設定解析 —— 依情境選範式]
//   dark_mode 啟用? true
//   telemetry 啟用? false
//   未知開關 unknown_flag 啟用? false  (安全預設為關閉)
//   共解析出 4 項，其中啟用 2 項
//     dark_mode = on
//     telemetry = off
//     beta_ui = on
//     legacy = off
