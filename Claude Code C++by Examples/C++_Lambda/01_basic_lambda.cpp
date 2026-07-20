// =============================================================================
//  01_basic_lambda.cpp  —  Lambda 表達式基礎
// =============================================================================
//  參考：
//    https://en.cppreference.com/w/cpp/language/lambda
//    https://cplusplus.com/reference/functional/
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 一、什麼是 lambda？                                        │
//  └────────────────────────────────────────────────────────────┘
//
//  Lambda 是 C++11 引入的「匿名函式物件」。一行程式同時定義一個 class
//  並產生它的實例：
//
//      [捕獲列表](參數列表) 修飾子 -> 回傳型別 { 函式本體 }
//      └─────┬─────┘└────┬───┘ └─┬─┘   └──┬───┘  └────┬────┘
//          capture     params  qualifiers  return    body
//
//  最小例子：
//      auto add = [](int a, int b) { return a + b; };
//      add(1, 2); // → 3
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 二、編譯器底下做了什麼？                                   │
//  └────────────────────────────────────────────────────────────┘
//
//  編譯器看到 lambda 時會「合成」一個 class（俗稱 closure type）：
//
//      auto add = [](int a, int b) { return a + b; };
//
//  約等價於：
//
//      class __lambda_at_line_XX {
//      public:
//          // operator() 才是真正的呼叫點
//          int operator()(int a, int b) const { return a + b; }
//      };
//      __lambda_at_line_XX add{};   // 一個編譯器自動取名的物件
//
//  關鍵觀察：
//   * lambda 的型別是「編譯器自動取名」的 unique 型別 → 兩個看起來一樣的
//     lambda 也是「不同型別」。所以實務上一律 `auto` 接，不要硬寫型別。
//   * `operator()` 預設是 `const` → 不能在 body 裡修改 by-value 捕獲的值。
//     若要解除 const，加 `mutable`（見 03_mutable_lambda.cpp）。
//   * 沒有捕獲（capture list 是空 `[]`）的 lambda 可以隱式轉成 function
//     pointer（見 07_function_pointer.cpp）。
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 三、為什麼 lambda 比手寫 functor 重要？                    │
//  └────────────────────────────────────────────────────────────┘
//
//  STL 演算法（sort、find_if、accumulate ...）需要傳入「可呼叫物件」做
//  比較或行為自訂。沒 lambda 時要這樣寫：
//
//      struct Greater { bool operator()(int a, int b) const { return a > b; } };
//      std::sort(v.begin(), v.end(), Greater{});
//
//  有 lambda 後一行解決：
//
//      std::sort(v.begin(), v.end(), [](int a, int b) { return a > b; });
//
//  Lambda 的核心價值：「把行為當參數傳」，而且就在使用點寫，不用跑去
//  別的地方定義一個 class。
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 四、本檔範例與 LeetCode                                    │
//  └────────────────────────────────────────────────────────────┘
//
//  * Demo 1：最小可運行的 lambda
//  * Demo 2：immediately-invoked lambda（IILE：宣告完馬上呼叫）
//  * Demo 3：lambda 當作 sort 比較器
//  * LeetCode 1768. Merge Strings Alternately（用 lambda 表達 zip 行為）
// =============================================================================

/*
補充筆記：basic_lambda
  - basic_lambda 會產生一個 closure 物件；capture list 決定它保存資料還是借用資料。
  - 值捕獲安全但可能拷貝，參考捕獲便宜但有生命週期風險。
  - lambda 若被存起來、丟到 thread 或包進 std::function，捕獲物件必須活得夠久。
  - basic_lambda 要從 closure object 理解：lambda 不是神祕語法，而是編譯器產生的匿名函式物件。
  - 捕獲 by value 是在建立 lambda 時複製，by reference 是保存別名；延後執行時 reference 捕獲最容易 dangling。
  - mutable 只讓 by value 捕獲的副本可修改，不會修改外部原變數。
  - generic lambda 的 auto 參數本質上是 function call operator template，錯誤可能在呼叫時才出現。
  - std::function 可保存不同 callable，但可能有型別抹除成本和配置成本；效能敏感處可優先用 template 接 callable。
  - lambda 放進 algorithm 時應讓 predicate 無副作用或副作用明確，否則演算法意圖會變難讀。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】lambda 的本質 / closure type
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. lambda 的本質是什麼？編譯器怎麼實作它？
//     答：lambda 是語法糖。編譯器為每個 lambda 運算式生成一個唯一、未命名的 class
//     （closure type）：每個捕獲成為一個 private 非靜態資料成員，函式本體成為
//     operator()（預設為 const）。lambda 運算式本身是該型別的一個 prvalue。
//     追問：兩個字面上完全一樣的 lambda 是同一個型別嗎？（不是，每次出現都產生
//     獨立型別，所以彼此不能賦值，也因此才需要 auto 或 std::function 來承接）
//
// 🔥 Q2. 請手寫一個等價於 lambda 的 functor class。
//     答：int x; double y; auto f = [x, &y](int a){ return x + y + a; }; 等價於一個
//     class：int x 成員（值捕獲的副本）、double& y 成員（參考捕獲）、
//     auto operator()(int a) const { return x + y + a; }。標準措辭是捕獲會
//     direct-initialize 對應成員，編譯器不必真的生出一個具名建構子。
//     追問：為什麼 operator() 預設是 const？（讓閉包在多次呼叫間不改變自身狀態；
//     加 mutable 就移除這個 const）
//
// Q3. IIFE（立即呼叫的 lambda）有什麼用？
//     答：const auto v = [&]{ ...多步驟計算...; return r; }(); 讓「要算好幾步才得到」
//     的變數仍能宣告為 const，同時把中間暫存變數限縮在 lambda 內、不污染外層命名。
//     編譯器通常能完全 inline，實務上零開銷。
//
// ⚠️ 陷阱. 可以寫 decltype(f1) g = f2; 把兩個「長得一模一樣」的 lambda 互相賦值嗎？
//     答：不行。兩者是不同型別，而且有捕獲的閉包型別沒有賦值運算子；C++20 前連無捕獲
//     lambda 都沒有預設建構子（C++20 起無捕獲 lambda 才可預設建構與賦值）。
//     為什麼會錯：多數人把 lambda 想成「一種值」，而它其實是「一個匿名型別的物件」，
//     型別身分由出現位置決定，不由簽名或內容決定。
// ═══════════════════════════════════════════════════════════════════════════

#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

int main() {
    // ─────────────────────────────────────────────────────────
    // Demo 1：最小 lambda
    // ─────────────────────────────────────────────────────────
    auto add = [](int a, int b) { return a + b; };
    std::cout << "[Demo1] add(3, 4) = " << add(3, 4) << "\n";

    // ─────────────────────────────────────────────────────────
    // Demo 2：IILE — 宣告完立即呼叫
    //   常用於：複雜初始化邏輯封裝在 const 變數初始化時
    // ─────────────────────────────────────────────────────────
    const int boxed = [] {
        int x = 0;
        for (int i = 1; i <= 10; ++i) x += i;
        return x;             // → 55
    }();                      // 結尾的 () 就是「立刻呼叫」
    std::cout << "[Demo2] sum(1..10) = " << boxed << "\n";

    // ─────────────────────────────────────────────────────────
    // Demo 3：lambda 當作 sort 比較器
    //   把 vector 從大到小排序
    // ─────────────────────────────────────────────────────────
    std::vector<int> v{5, 2, 9, 1, 7};
    std::sort(v.begin(), v.end(), [](int a, int b) { return a > b; });
    std::cout << "[Demo3] sorted desc:";
    for (int x : v) std::cout << ' ' << x;
    std::cout << '\n';

    // ─────────────────────────────────────────────────────────
    // LeetCode 1768. Merge Strings Alternately
    //   題意：兩個字串輪流取一個字元組合；某一邊用完就把另一邊剩下的接上。
    //   範例："abc" + "pqr" → "apbqcr"；"ab" + "pqrs" → "apbqrs"
    //
    //   為何放這裡？這是非常基本的「行為當參數」入門 — 我們把「取第 i 個
    //   字元」抽成一個 lambda，讓主迴圈專心處理 i 的推進。
    // ─────────────────────────────────────────────────────────
    auto mergeAlternately = [](const std::string& a, const std::string& b) {
        std::string out;
        out.reserve(a.size() + b.size());

        // safeAt：i 超出範圍時回傳空字元，這樣不必在主迴圈到處檢查邊界
        auto safeAt = [](const std::string& s, size_t i) -> char {
            return i < s.size() ? s[i] : '\0';
        };

        size_t n = std::max(a.size(), b.size());
        for (size_t i = 0; i < n; ++i) {
            char ca = safeAt(a, i);
            char cb = safeAt(b, i);
            if (ca) out.push_back(ca);
            if (cb) out.push_back(cb);
        }
        return out;
    };
    std::cout << "[LC1768] merge(\"abc\",\"pqr\") = "
              << mergeAlternately("abc", "pqr") << '\n';
    std::cout << "[LC1768] merge(\"ab\",\"pqrs\") = "
              << mergeAlternately("ab", "pqrs") << '\n';

    // ─────────────────────────────────────────────────────────
    // LeetCode 1672. Richest Customer Wealth
    //   題意：accounts[i] 是第 i 位顧客在各銀行的存款；回傳最高的「客戶總存款」。
    //   思路：對每一列用 lambda 加總，再取最大值；展示「行為當參數」最入門的寫法。
    // ─────────────────────────────────────────────────────────
    {
        std::vector<std::vector<int>> accounts{{1,2,3},{3,2,1},{1,5}};
        // rowSum：簡單的加總 lambda（無捕獲、IILE 友善）
        auto rowSum = [](const std::vector<int>& row) {
            int s = 0;
            for (int v : row) s += v;
            return s;
        };
        int richest = 0;
        for (const auto& row : accounts) {
            int s = rowSum(row);
            if (s > richest) richest = s;
        }
        std::cout << "[LC1672] maximum wealth = " << richest << '\n';
        // 預期：6（第 3 列 1+5 = 6 為最大）
    }

    // ─────────────────────────────────────────────────────────
    // 實用範例：把日常工作常見「重試 N 次直到成功」抽成 lambda
    //   把「要做的動作」當參數傳；helper 不需要知道做什麼，只負責重試節奏
    // ─────────────────────────────────────────────────────────
    {
        // 模擬一個「前兩次失敗、第三次成功」的動作
        int attempts = 0;
        auto flakyJob = [&attempts]() {
            ++attempts;
            return attempts >= 3;       // 回傳 true 代表成功
        };

        // 通用 retry：吃任何「無參數、回傳 bool」的 callable
        auto retry = [](auto job, int maxTry) {
            for (int i = 1; i <= maxTry; ++i) {
                if (job()) return i;    // 回報「第幾次成功」
            }
            return -1;                  // 全失敗
        };

        int okAt = retry(flakyJob, 5);
        std::cout << "[retry] succeeded at attempt #" << okAt << '\n';
    }

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：lambda 的型別到底是什麼？
    //    A：每個 lambda 都是「unique unnamed class type」。所以
    //
    //          auto a = [](int){};
    //          auto b = [](int){};   // 跟 a 看起來一樣
    //          // a 和 b 的型別「不同」、不能互相 assign。
    //
    //       要把不同 lambda 放同一容器，必須用 std::function 統一型別
    //       （見 06_std_function.cpp）。
    //
    //  Q2：為什麼 operator() 預設 const？
    //    A：因為 lambda 預設是「pure function」概念 — 同樣輸入給同樣輸出。
    //       要在 body 改捕獲值就得用 `mutable`。
    //
    //  Q3：return 型別什麼時候要寫？
    //    A：編譯器多半能自動推導；只有兩條路 return 的型別不一致，或要強
    //       制成某型別時才需要明寫 `-> T`。
    //
    return 0;
}

// 編譯: g++ -std=c++20 -Wall -Wextra 01_basic_lambda.cpp -o 01_basic_lambda

// === 預期輸出 ===
// [Demo1] add(3, 4) = 7
// [Demo2] sum(1..10) = 55
// [Demo3] sorted desc: 9 7 5 2 1
// [LC1768] merge("abc","pqr") = apbqcr
// [LC1768] merge("ab","pqrs") = apbqrs
// [LC1672] maximum wealth = 6
// [retry] succeeded at attempt #3
