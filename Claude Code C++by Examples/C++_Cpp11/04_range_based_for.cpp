// =============================================================================
//  04_range_based_for.cpp  —  range-based for 迴圈 (C++11)
// =============================================================================
//  參考：https://en.cppreference.com/w/cpp/language/range-for
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 一、語法                                                   │
//  └────────────────────────────────────────────────────────────┘
//
//      for (declaration : range)
//          body;
//
//  例：
//      for (int x : v)            // 拷貝每個 element
//      for (auto& x : v)          // 取參考 — 可改寫
//      for (const auto& x : v)    // 取 const ref — 不改、避免拷貝
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 二、編譯器實際展開                                         │
//  └────────────────────────────────────────────────────────────┘
//
//      for (auto x : v) { body; }
//
//  約等於：
//
//      auto&& __range = v;
//      auto __begin = std::begin(__range);
//      auto __end   = std::end(__range);
//      for (; __begin != __end; ++__begin) {
//          auto x = *__begin;
//          body;
//      }
//
//  關鍵：用 std::begin / std::end，所以對自訂容器只要提供 begin()/end()，
//  range-for 就能用。
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 三、三種寫法的選擇                                         │
//  └────────────────────────────────────────────────────────────┘
//
//   * 「我要改容器內元素」              → for (auto& x : v)
//   * 「我只看不改、且型別便宜（int 等）」→ for (auto x : v)
//   * 「我只看不改、但型別貴（vector 等）」→ for (const auto& x : v)
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 四、本檔示範                                               │
//  └────────────────────────────────────────────────────────────┘
//
//   * Demo 1：三種寫法各示範一次
//   * Demo 2：對自訂 class 提供 begin/end → range-for 也能用
//   * LeetCode 1480. Running Sum of 1d Array
// =============================================================================

/*
補充筆記：range_based_for
  - range_based_for 是現代 C++ 語法或標準庫特性；學習時要把「少寫字」和「語意更精確」分開看。
  - auto 讓型別由初始化式推導，但會丟掉 top-level const/reference；需要保留引用語意時要寫 auto&、const auto& 或 decltype(auto)。
  - brace initialization 能減少未初始化與 narrowing，但遇到 initializer_list overload 可能選到不同建構子。
  - constexpr、static_assert、if constexpr 把部分錯誤和計算提前到編譯期，能讓 template 和常數邏輯更清楚。
  - 屬性如 [[nodiscard]]、[[maybe_unused]]、[[fallthrough]] 是對編譯器和讀者的意圖標記，不應拿來掩蓋設計問題。
  - string_view、optional、variant、structured binding 等特性改善介面表達力，但也帶來生命週期或狀態檢查責任。
  - range-based for 會呼叫 begin/end；容器在迴圈中被修改時要小心 iterator 失效。
  - for (auto x : v) 會複製元素，修改 x 不會改容器；要修改元素應寫 auto&。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】range-based for
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. range-based for 實際展開成什麼？
//     答：大致是 `auto&& __r = range;` 取得 `__b = begin(__r)`、`__e = end(__r)`，
//         再 `for (; __b != __e; ++__b) { 宣告 = *__b; body }`。所以自訂型別
//         只要提供 begin()/end() 就能被 range-for 走訪（見本檔 CountUp）。
//     追問：begin 與 end 可以是不同型別嗎？（C++11/14 要求兩者同型別；
//         C++17 起才放寬允許 sentinel，這是很常被追打的版本分界）
//
// 🔥 Q2. `auto x`、`auto& x`、`const auto& x` 三種寫法怎麼選？
//     答：`auto x` 每輪都拷貝一份元素，改它不影響容器；`auto& x` 取參考、可就地
//         修改；`const auto& x` 唯讀且免拷貝。元素便宜（int）用第一種無妨，元素
//         貴（string、vector）一律用 const auto&。
//     追問：那 `std::vector<bool>` 為什麼不能用 `auto&`？（operator[] 回傳的是
//         proxy 暫時物件，非 const 左值參考綁不上去，本機 g++ 實測即編譯錯）
//
// ⚠️ 陷阱. `for (auto x : getObj().items())` 安全嗎？（items() 回傳成員容器的參考）
//     答：不安全，是懸垂存取。被 `auto&& __r` 延長壽命的，只有「__r 直接綁定的
//         那個臨時物件」；這裡 __r 綁到的是 items() 回傳的參考，而 getObj() 產生
//         的臨時物件在該完整運算式結束時就被銷毀，迴圈是在對已死物件的成員迭代。
//         本機以 -fsanitize=address 實測，確實報 stack-use-after-scope。
//     為什麼會錯：大家記得「range-for 會幫我延長臨時物件的壽命」，卻沒注意到被
//         延長的只有 __r 綁的那一個。`for (auto x : getVector())` 是安全的（__r
//         直接綁到那個臨時 vector）。保險寫法：先 `auto obj = getObj();`，
//         把物件實際留在具名區域變數裡，再迭代 `obj.items()`。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <string>
#include <vector>

// 自訂一個「能被 range-for 走」的容器：只需 begin / end
struct CountUp {
    int n;
    struct Iter {
        int i;
        int operator*() const { return i; }
        Iter& operator++() { ++i; return *this; }
        bool operator!=(const Iter& o) const { return i != o.i; }
    };
    Iter begin() const { return {0}; }
    Iter end()   const { return {n}; }
};

int main() {
    // ─────────────────────────────────────────────────────────
    // Demo 1：三種寫法
    // ─────────────────────────────────────────────────────────
    std::vector<int> v{1, 2, 3, 4, 5};

    // (1) 改寫 — 用 reference
    for (auto& x : v) x *= 2;
    std::cout << "[Demo1] doubled:";
    for (auto x : v) std::cout << ' ' << x;
    std::cout << '\n';

    // (2) const ref — 大型物件唯讀
    std::vector<std::string> ss{"alice", "bob", "charlie"};
    for (const auto& s : ss) std::cout << "[Demo1]   name=" << s << '\n';

    // ─────────────────────────────────────────────────────────
    // Demo 2：自訂容器
    // ─────────────────────────────────────────────────────────
    std::cout << "[Demo2] CountUp{5}:";
    for (int x : CountUp{5}) std::cout << ' ' << x;
    std::cout << '\n';

    // ─────────────────────────────────────────────────────────
    // LeetCode 1480. Running Sum of 1d Array
    //   題意：給 nums，回傳 prefix sum 陣列
    //   為何放這？示範 range-for 累積、與標準寫法的乾淨對照
    // ─────────────────────────────────────────────────────────
    std::vector<int> nums{1, 2, 3, 4};
    std::vector<int> prefix;
    prefix.reserve(nums.size());
    int sum = 0;
    for (int x : nums) {       // range-for 比 for(int i=0; i<nums.size(); ++i) 簡潔
        sum += x;
        prefix.push_back(sum);
    }
    std::cout << "[LC1480] prefix =";
    for (int x : prefix) std::cout << ' ' << x;
    std::cout << '\n';

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：range-for 中可以拿到「index」嗎？
    //    A：本身沒提供。要 index 就用傳統 for 或自己維護一個 i：
    //         int i = 0;
    //         for (auto& x : v) { /* 用 i */ ++i; }
    //       C++23 std::views::enumerate 有提供 index。
    //
    //  Q2：在 range-for 中改容器（push/erase）安全嗎？
    //    A：通常不安全 — iterator 可能失效（vector push_back 重配置時）。
    //       要修改大小用傳統 for 或 erase-remove idiom。
    //
    //  Q3：C++17 / C++20 對 range-for 有改進嗎？
    //    A：C++17 允許 begin / end 是不同型別（更廣的 range concept）；
    //       C++20 加入「init statement」：for (auto i = 0; auto x : v) {...}
    //
    // ─────────────────────────────────────────────────────────
    // LeetCode 1672. Richest Customer Wealth
    //   題意：accounts[i][j] = 第 i 客戶在第 j 銀行的存款，求最有錢客戶總財富。
    //   為什麼放這？示範「巢狀 range-for」處理 2D 矩陣最乾淨的寫法。
    // ─────────────────────────────────────────────────────────
    std::vector<std::vector<int>> accounts{{1, 2, 3}, {3, 2, 1}, {4, 5, 6}};
    int richest = 0;
    for (const auto& row : accounts) {       // 外層 const ref，避免拷貝 vector
        int wealth = 0;
        for (int money : row) wealth += money;
        if (wealth > richest) richest = wealth;
    }
    std::cout << "[LC1672] richest = " << richest << '\n';

    // ─────────────────────────────────────────────────────────
    // 實用範例：把整批字串改大寫
    //   工作上常見：對 vector<string> 做就地 transform
    // ─────────────────────────────────────────────────────────
    std::vector<std::string> tags{"hello", "World", "C++"};
    for (auto& tag : tags) {                  // 取 ref → 可改
        for (auto& ch : tag) {                // 巢狀 ref → 改 char
            if (ch >= 'a' && ch <= 'z') ch = static_cast<char>(ch - 'a' + 'A');
        }
    }
    std::cout << "[Demo4] upper:";
    for (const auto& tag : tags) std::cout << ' ' << tag;
    std::cout << '\n';

    return 0;
}
