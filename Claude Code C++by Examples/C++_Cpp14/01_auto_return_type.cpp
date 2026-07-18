// =============================================================================
//  01_auto_return_type.cpp  —  函式 return type 用 auto 自動推導 (C++14)
// =============================================================================
//  參考：https://en.cppreference.com/w/cpp/language/function
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 一、C++11 的限制                                           │
//  └────────────────────────────────────────────────────────────┘
//
//  C++11 雖然有 lambda 自動推導 return type、但「一般函式」仍然要明寫：
//
//      auto add(int a, int b) -> int { return a + b; }
//      // 或：
//      int add(int a, int b)         { return a + b; }
//
//  C++14 起一般函式也能省略 return type，只寫 auto：
//
//      auto add(int a, int b) { return a + b; }
//      //   ^^^^                        ↑
//      //   推導為 int             return 表達式決定型別
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 二、推導規則                                               │
//  └────────────────────────────────────────────────────────────┘
//
//   * 跟「auto x = expr;」一樣 — 拷貝值、捨棄 reference 與 top-level const
//   * 多條 return statement 必須推導出「同一型別」，否則編譯錯
//   * 函式定義必須「先看到」才能呼叫（必須有 body 推導才知道）
//
//  例：
//      auto bad(bool b) {
//          if (b) return 1;       // int
//          else   return 1.0;     // double  ❌ 型別衝突
//      }
//      // 解法：double half(bool b) { ... } 或顯式 trailing return
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 三、適合 / 不適合的場合                                   │
//  └────────────────────────────────────────────────────────────┘
//
//  適合：
//   * 短的 inline 函式 — return type 一目瞭然
//   * 真的依賴 template 參數的型別
//
//  不適合：
//   * 大型 .h / .cpp 拆分 API — 讀者打開 .h 想立刻知道 return type
//   * 跨模組 API — 明寫 return type 是文件
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 四、本檔示範                                               │
//  └────────────────────────────────────────────────────────────┘
//
//   * Demo 1：基本 auto return
//   * Demo 2：跟 template 結合
//   * Demo 3：兩條路 return 必須同型別
// =============================================================================

/*
補充筆記：auto_return_type
  - auto_return_type 是現代 C++ 語法或標準庫特性；學習時要把「少寫字」和「語意更精確」分開看。
  - auto 讓型別由初始化式推導，但會丟掉 top-level const/reference；需要保留引用語意時要寫 auto&、const auto& 或 decltype(auto)。
  - brace initialization 能減少未初始化與 narrowing，但遇到 initializer_list overload 可能選到不同建構子。
  - constexpr、static_assert、if constexpr 把部分錯誤和計算提前到編譯期，能讓 template 和常數邏輯更清楚。
  - 屬性如 [[nodiscard]]、[[maybe_unused]]、[[fallthrough]] 是對編譯器和讀者的意圖標記，不應拿來掩蓋設計問題。
  - string_view、optional、variant、structured binding 等特性改善介面表達力，但也帶來生命週期或狀態檢查責任。
  - C++14 auto return type 由 return statement 推導；多個 return 必須推導出一致型別。
  - 公開 API 若回傳型別是介面承諾，過度使用 auto 可能讓呼叫端難以看出型別。
*/
#include <iostream>
#include <string>
#include <type_traits>
#include <vector>

// 基本 — 比明寫 int 短
auto sum(int a, int b) { return a + b; }

// template — 不必再寫 -> decltype(a + b)
template <class T, class U>
auto multiply(T a, U b) { return a * b; }

// 多 return 必須同型別 — 用三元或統一型別
auto absoluteVal(double x) {
    return x < 0 ? -x : x;       // 都是 double
}

// 回傳容器 — 編譯器推導出 std::vector<int>
auto makeVec(int n) {
    std::vector<int> v;
    v.reserve(n);
    for (int i = 0; i < n; ++i) v.push_back(i * i);
    return v;
}

int main() {
    auto a = sum(1, 2);
    auto b = multiply(3, 4.5);              // double
    auto c = multiply(2, 3);                // int
    (void)c;

    static_assert(std::is_same<decltype(a), int>::value, "");
    static_assert(std::is_same<decltype(b), double>::value, "");
    std::cout << "[Demo1] sum(1,2)="       << a
              << " multiply(3,4.5)="       << b << '\n';

    auto v = makeVec(5);
    std::cout << "[Demo2] vec:";
    for (int x : v) std::cout << ' ' << x;
    std::cout << '\n';

    std::cout << "[Demo3] absoluteVal(-7) = " << absoluteVal(-7) << '\n';

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：auto return type 跟 trailing return 哪個好？
    //    A：能省就 auto；要明寫型別（API 文件、依賴 decltype）就 trailing。
    //
    //  Q2：「forward declare」auto 函式可以嗎？
    //    A：不行 — 沒有 body 就無法推導，會編譯錯。要前向宣告請寫明確的
    //       trailing return / 傳統 return type。
    //
    //  Q3：recursive auto 函式？
    //    A：要小心 — 內部呼叫自己時 return type 還沒定。慣例：把第一個
    //       return 寫在最前面，編譯器就能定下來：
    //         auto f(int n) {
    //             if (n <= 0) return 0;       // 第一個 return 定型別 = int
    //             return f(n - 1) + n;
    //         }
    //
    // ─────────────────────────────────────────────────────────
    // LeetCode 1672. Richest Customer Wealth — 用 auto return 型別
    //   題意：accounts[i][j] 第 i 客戶在 j 銀行的存款，求最有錢客戶總財富。
    //   為什麼放這？示範 auto return type 在 lambda 與一般 helper 都能用。
    // ─────────────────────────────────────────────────────────
    auto rowWealth = [](const std::vector<int>& row) {
        int s = 0;
        for (int v3 : row) s += v3;
        return s;                       // auto 推導為 int
    };
    auto maximumWealth = [&](const std::vector<std::vector<int>>& accs) {
        int mx = 0;
        for (const auto& row : accs) {
            int w = rowWealth(row);
            if (w > mx) mx = w;
        }
        return mx;
    };
    std::vector<std::vector<int>> accs{{1, 2, 3}, {3, 2, 1}, {7, 7, 7}};
    std::cout << "[LC1672] max wealth = " << maximumWealth(accs) << '\n';

    // ─────────────────────────────────────────────────────────
    // 實用範例：寫 makeRange 工廠 — return type 是「容器」auto 推導即可
    //   工作上常見：寫工廠函式回容器，不必明寫一長串 std::vector<...>
    // ─────────────────────────────────────────────────────────
    auto makeRange = [](int from, int to) {
        std::vector<int> out;
        out.reserve(static_cast<std::size_t>(to - from));
        for (int i = from; i < to; ++i) out.push_back(i);
        return out;                     // 編譯器推導為 vector<int>
    };
    auto r5 = makeRange(10, 15);
    std::cout << "[Demo4] makeRange(10,15) =";
    for (int x : r5) std::cout << ' ' << x;
    std::cout << '\n';

    return 0;
}
