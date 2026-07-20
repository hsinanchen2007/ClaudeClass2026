// =============================================================================
//  第 2.7 章 範例 10  —  陷阱：重載函式名無法被完美轉發
// =============================================================================
//
// 【主題資訊 Information】
//   標頭檔：<iostream>、<utility>（std::forward）、<string>
//   （若改用 std::invoke 統一呼叫語法，則需 <functional>，C++17 起提供）
//   問題形式：
//       void process(int);  void process(double);      // 兩個多載
//       template<class T> void wrapper(T&&);
//       wrapper(process);                              // ✗ 編譯錯誤
//   兩個解法：static_cast 指定簽名（C++11）／lambda 包裝（C++11，推薦）。
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼「函式名」推導不出型別】
//   在 C++ 裡，process 這個名字本身不是一個值，而是「一組多載的集合」
//   （overload set）。要把它變成一個具體的函式指標，必須有一個「目標型別」
//   來做選擇——這個過程叫做多載決議下的位址取用。
//       void (*p)(int) = process;   // ✓ 有目標型別 void(*)(int)，選得出來
//       auto p = process;           // ✗ auto 沒有目標型別，無從選起
//   模板參數推導正是後者的情況：T 就是待求的未知數，
//   編譯器不可能「先假設答案再回頭驗證」，所以直接推導失敗。
//
// 【2. 這不是完美轉發的缺陷，是語言的基本規則】
//   同樣的錯誤在完全沒有轉發的地方也會出現，例如
//       std::vector<???> v;  v.push_back(process);
//   或把多載函式名傳給任何模板參數。理解成「多載集合需要目標型別才能塌縮
//   成單一函式」，而不是「forward 有 bug」，才不會在錯的地方找解法。
//
// 【3. 兩種解法的取捨】
//   解法一 static_cast<void(*)(int)>(process)
//     優點：不需要額外的可呼叫物件，零額外抽象。
//     缺點：必須手寫完整的函式指標型別。process 的簽名一旦改變
//           （多一個參數、回傳型別改了），這串型別字串就要同步修改，
//           而且忘記改時，編譯器可能選到另一個多載而不報錯。
//   解法二 [](int x){ process(x); }
//     優點：呼叫端只寫「怎麼呼叫」，選哪個多載交給正常的多載決議；
//           簽名微調時通常不必動這行。
//     缺點：多一個閉包型別（實務上幾乎零成本，會被內聯掉）。
//   C++14 起可寫成泛型 lambda [](auto&&... a){ process(std::forward<decltype(a)>(a)...); }，
//   一行同時涵蓋所有多載，這是現代寫法的首選。
//
// 【概念補充 Concept Deep Dive】
//   (A) 為什麼 static_cast 選得出來？因為 static_cast 的目標型別
//       void(*)(int) 提供了「唯一正確答案」的判準，多載決議在這個
//       目標型別下只有 process(int) 匹配，於是塌縮成功。
//   (B) 樣板函式名也一樣：把一個函式模板名傳給另一個模板，同樣推不出來，
//       必須寫 f<int> 明確實例化，或用 lambda 包起來。
//   (C) 這個陷阱在標準函式庫裡很常見：std::thread(process, 1)、
//       std::async(process, 1)、std::transform(..., process) 全都會踩到。
//       解法都一樣：包一層 lambda。
//
// 【注意事項 Pay Attention】
//   1. 錯誤訊息通常是 "no matching function" 或 "unresolved overloaded
//      function type"，看起來像是 wrapper 有問題，其實問題在引數。
//   2. static_cast 的型別必須「完全吻合」，包括回傳型別與 const/noexcept
//      修飾，否則不會匹配。
//   3. 泛型 lambda 包裝時記得 forward，否則又退回「一律左值」的老問題。
//   4. 只有一個 process 時不會出錯——所以這個 bug 常常是「別人後來加了
//      一個多載」才突然冒出來的。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】重載函式名的轉發
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. wrapper(process) 為什麼編譯不過？process 明明存在。
//     答：process 是「一組多載的集合」而不是單一的值。要塌縮成具體的
//         函式指標必須有目標型別可依循，而模板參數 T 正是待求的未知數，
//         編譯器無從選擇，推導直接失敗。
//     追問：那 void (*p)(int) = process; 為什麼可以？
//         → 因為左邊提供了明確目標型別，多載決議有判準，能唯一選出。
//
// 🔥 Q2. static_cast 與 lambda 兩種解法，實務上該選哪個？
//     答：多數情況選 lambda。static_cast 要求手寫完整函式指標型別，
//         被呼叫函式的簽名一改就得同步維護，忘了改還可能默默選到別的多載。
//         lambda 只描述「怎麼呼叫」，選擇交回給正常的多載決議。
//         C++14 泛型 lambda 搭配 forward 可一次涵蓋所有多載。
//
// ⚠️ 陷阱. 這段程式碼昨天還好好的，今天同事加了一個 process 多載就編不過了——
//          為什麼？
//     答：只有一個 process 時，函式名可以無歧義地退化成函式指標，
//         模板推導成功。一旦出現第二個多載，名字就變成多載集合，
//         推導立刻失敗。錯誤訊息還會指向 wrapper 的呼叫處，
//         讓人誤以為是模板寫錯了。
//     為什麼會錯：大家把函式名想成「一個變數」，
//         但它其實是一個名字查詢的結果，可能對應多個實體；
//         「能不能當值用」取決於上下文有沒有提供選擇的依據。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <utility>
#include <string>

void process(int x)    { std::cout << "  int: " << x << "\n"; }
void process(double x) { std::cout << "  double: " << x << "\n"; }

// 這個 wrapper 真的會呼叫它收到的東西，讓兩種解法的效果看得見
template<typename F, typename... Args>
void wrapper(F&& f, Args&&... args) {
    std::forward<F>(f)(std::forward<Args>(args)...);
}

// -----------------------------------------------------------------------------
// 【日常實務範例】把「格式化輸出」註冊進事件處理表
//   情境：日誌系統允許使用者註冊處理函式。專案裡 formatValue 依型別多載，
//         直接把函式名交給註冊介面（模板）就會踩到本檔的陷阱。
//         實務上一律用 lambda 包一層——這也是為什麼你在真實程式碼裡
//         很少看到裸的函式名被傳給泛型介面。
// -----------------------------------------------------------------------------
void formatValue(int v)                { std::cout << "    [int]    " << v << "\n"; }
void formatValue(double v)             { std::cout << "    [double] " << v << "\n"; }
void formatValue(const std::string& v) { std::cout << "    [string] " << v << "\n"; }

template<typename Handler>
void runHandler(Handler&& h) {
    std::forward<Handler>(h)();
}

int main() {
    // wrapper(process);
    // 錯誤！process 是重載函式，編譯器不知道要選哪一個
    // T 無法推導

    std::cout << "=== 解法 1：static_cast 明確指定簽名 ===\n";
    wrapper(static_cast<void(*)(int)>(process), 42);
    wrapper(static_cast<void(*)(double)>(process), 3.5);

    std::cout << "\n=== 解法 2：用 Lambda 包裝（推薦）===\n";
    wrapper([](int x) { process(x); }, 42);
    wrapper([](double x) { process(x); }, 3.5);

    std::cout << "\n=== 解法 2 進階：泛型 lambda 一次涵蓋所有多載（C++14）===\n";
    auto anyProcess = [](auto&& v) { process(std::forward<decltype(v)>(v)); };
    wrapper(anyProcess, 7);
    wrapper(anyProcess, 2.75);

    std::cout << "\n=== 日常實務：註冊日誌格式化處理器 ===\n";
    // 不能寫 runHandler(formatValue)——多載集合推導不出型別
    runHandler([] { formatValue(1001); });
    runHandler([] { formatValue(99.5); });
    runHandler([] { formatValue(std::string("service started")); });

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 2.7 章：完美轉發 (Perfect Forwarding) — 泛型程式設計的關鍵技術10.cpp" -o overload_fwd

// 註：本檔未附 LeetCode 範例。這是一個語言層的推導限制，
//     在演算法題中不會構成解題核心，硬套一題只會失真。

// === 預期輸出 ===
// === 解法 1：static_cast 明確指定簽名 ===
//   int: 42
//   double: 3.5
//
// === 解法 2：用 Lambda 包裝（推薦）===
//   int: 42
//   double: 3.5
//
// === 解法 2 進階：泛型 lambda 一次涵蓋所有多載（C++14）===
//   int: 7
//   double: 2.75
//
// === 日常實務：註冊日誌格式化處理器 ===
//     [int]    1001
//     [double] 99.5
//     [string] service started
