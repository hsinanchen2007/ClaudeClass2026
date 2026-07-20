// =============================================================================
//  06_ctad.cpp  —  Class Template Argument Deduction (C++17)
// =============================================================================
//  參考：https://en.cppreference.com/w/cpp/language/class_template_argument_deduction
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 一、是什麼？                                               │
//  └────────────────────────────────────────────────────────────┘
//
//  C++17 之前：建構 class template 必須指定 template 參數：
//
//      std::pair<int, std::string> p{1, "hi"};
//      std::vector<int> v{1, 2, 3};
//      std::tuple<int, double, std::string> t{1, 2.5, "x"};
//
//  通常會用 helper：
//      auto p = std::make_pair(1, std::string{"hi"});
//      auto t = std::make_tuple(1, 2.5, std::string{"x"});
//
//  C++17 起編譯器能「從建構子參數推導」 — 不必再寫 template arg：
//
//      std::pair p{1, std::string{"hi"}};        // → std::pair<int, std::string>
//      std::vector v{1, 2, 3};                    // → std::vector<int>
//      std::tuple t{1, 2.5, std::string{"x"}};    // → std::tuple<int, double, std::string>
//
//  helper 函式 (make_pair, make_tuple) 在 C++17+ 多數情況可以不必用。
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 二、deduction guide                                        │
//  └────────────────────────────────────────────────────────────┘
//
//  自訂 class 想支援 CTAD，多數情況「編譯器自動推導 ctor」就夠；複雜情況
//  可以寫 deduction guide：
//
//      template <class T>
//      struct Pack {
//          T value;
//          template <class U> Pack(U u) : value(u) {}
//      };
//      template <class U> Pack(U) -> Pack<U>;     // 顯式 deduction guide
//
//      Pack p{42};                  // → Pack<int>
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 三、本檔示範                                               │
//  └────────────────────────────────────────────────────────────┘
//
//   * Demo 1：標準容器都支援 CTAD
//   * Demo 2：自訂 class + deduction guide
// =============================================================================

/*
補充筆記：ctad
  - ctad 是現代 C++ 語法或標準庫特性；學習時要把「少寫字」和「語意更精確」分開看。
  - auto 讓型別由初始化式推導，但會丟掉 top-level const/reference；需要保留引用語意時要寫 auto&、const auto& 或 decltype(auto)。
  - brace initialization 能減少未初始化與 narrowing，但遇到 initializer_list overload 可能選到不同建構子。
  - constexpr、static_assert、if constexpr 把部分錯誤和計算提前到編譯期，能讓 template 和常數邏輯更清楚。
  - 屬性如 [[nodiscard]]、[[maybe_unused]]、[[fallthrough]] 是對編譯器和讀者的意圖標記，不應拿來掩蓋設計問題。
  - string_view、optional、variant、structured binding 等特性改善介面表達力，但也帶來生命週期或狀態檢查責任。
  - CTAD 讓 class template 可從建構參數推導模板參數，例如 std::pair p{1,2.0}。
  - 推導不一定符合你想要的型別，公開 API 或教學中有時明寫模板參數更清楚。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】CTAD（Class Template Argument Deduction）
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. CTAD 是什麼？和 make_pair / make_tuple 的關係？
//     答：std::pair p{1, 2.0}; 編譯器由建構子引數推出 pair<int,double>，不必再寫
//         make_pair。編譯器先由所有建構子與明確寫出的推導指引產生一組虛擬函式，
//         再走一般的重載決議。
//     追問：make_* 系列全過時了嗎？（大體上是，但 make_unique / make_shared 不可
//           取代——它們負責的是例外安全與單次配置，不是型別推導）
//
// ⚠️ 陷阱. CTAD 有哪些「推導出乎意料」的情況？
//     答：① std::vector v{vec};（vec 是 vector<int>）推成 copy，即 vector<int>，
//            而非 vector<vector<int>>——這是特殊的 copy 優先規則。
//         ② std::pair p{"a", "b"}; 推成 pair<const char*, const char*>，
//            不是 std::string；要 std::pair p{"a"s, "b"s} 或自訂推導指引。
//         ③ C++17 不支援 aggregate 的 CTAD（需自寫推導指引，C++20 才支援）。
//         ④ C++17 的 CTAD 不適用於別名模板（C++20 放寬）。
//         ⑤ 只有在完全沒寫模板引數時才啟動，連 std::pair<> 都不行。
//     為什麼會錯：大家把 CTAD 想成「編譯器會猜我要的型別」，但它實際上只是拿建構
//         子跑重載決議，字面量該退化成指標就退化，不會幫你升級成 std::string。
// ═══════════════════════════════════════════════════════════════════════════

#include <array>
#include <iostream>
#include <map>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

// 自訂 class — 編譯器能自動從 ctor 推導
template <class T>
struct Pack {
    T value;
    Pack(T v) : value(v) {}
};

// 顯式 deduction guide（多數情況自動就夠，這個 demo 是教學示意）
// 如果 ctor 是 template<class U> 而成員型別不同，就需要自寫 guide
template <class T>
struct Wrapper {
    T value;
    template <class U>
    Wrapper(U&& u) : value(std::forward<U>(u)) {}
};
template <class U> Wrapper(U&&) -> Wrapper<std::decay_t<U>>;

int main() {
    // ─────────────────────────────────────────────────────────
    // Demo 1：標準容器
    // ─────────────────────────────────────────────────────────
    std::pair p{1, std::string{"hi"}};         // pair<int, string>
    std::vector v{10, 20, 30};                  // vector<int>
    std::array  a{1, 2, 3, 4};                  // array<int, 4>
    std::tuple  t{1, 2.5, std::string{"x"}};    // tuple<int, double, string>

    std::cout << "[Demo1] pair: " << p.first << ", " << p.second << '\n';
    std::cout << "[Demo1] vec size=" << v.size() << '\n';
    std::cout << "[Demo1] array size=" << a.size() << '\n';
    std::cout << "[Demo1] tuple<0>=" << std::get<0>(t)
              << " <2>=" << std::get<2>(t) << '\n';

    // map 也行（從 initializer_list 推 K, V）
    std::map m{std::pair{std::string{"alice"}, 30},
               std::pair{std::string{"bob"},   25}};
    std::cout << "[Demo1] map size=" << m.size() << '\n';

    // ─────────────────────────────────────────────────────────
    // Demo 2：自訂類別
    // ─────────────────────────────────────────────────────────
    Pack p1{42};                       // Pack<int>
    Pack p2{std::string{"hello"}};     // Pack<string>
    std::cout << "[Demo2] p1.value=" << p1.value
              << " p2.value=" << p2.value << '\n';

    Wrapper w1{42};                    // Wrapper<int>
    Wrapper w2{3.14};                  // Wrapper<double>
    std::cout << "[Demo2] w1=" << w1.value << " w2=" << w2.value << '\n';

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：什麼時候要寫 deduction guide？
    //    A：(a) ctor 是 template<class U>，成員型別不能直接從 U 推
    //       (b) 你想推導出「跟引數不一樣」的型別 (例如「char[N] → string」)
    //       (c) 想精確控制推導結果（避免 array decay）
    //
    //  Q2：CTAD 跟 auto 衝突嗎？
    //    A：不衝突，互補。auto 用於「左邊變數型別不寫」；CTAD 用於「右邊
    //       建構子型別不寫」：
    //         auto v = std::vector<int>{1, 2};   // CTAD 不必：右邊已寫
    //         std::vector v = {1, 2};             // CTAD：右邊省 <int>
    //
    //  Q3：CTAD 跟 make_pair / make_tuple 比較？
    //    A：CTAD 多數情況可取代 make_*，且更簡潔。但 make_pair / make_tuple
    //       對「decay」(array → pointer 等) 有特定 helper 行為，少數場合
    //       還是保留。新程式碼預設用 CTAD，怪怪的再 fallback 到 make_*。
    //
    // ─────────────────────────────────────────────────────────
    // LeetCode 1480. Running Sum of 1d Array — 用 CTAD 建立 vector
    //   題意：給 nums，回 prefix sum 陣列。
    //   為什麼放這？CTAD 讓 std::vector v{1,2,3,4} 直接寫，
    //                測試輸入準備變得超簡潔。
    // ─────────────────────────────────────────────────────────
    auto runningSum = [](std::vector<int> nums) {
        for (std::size_t i = 1; i < nums.size(); ++i) nums[i] += nums[i - 1];
        return nums;
    };
    std::vector input{1, 2, 3, 4};                  // CTAD：vector<int>
    auto out = runningSum(input);
    std::cout << "[LC1480] prefix:";
    for (int x : out) std::cout << ' ' << x;
    std::cout << '\n';

    // ─────────────────────────────────────────────────────────
    // 實用範例：用 CTAD 建立「name -> data」對照表
    //   工作上常見：測試 fixture、設定表初始化
    // ─────────────────────────────────────────────────────────
    std::pair host{std::string{"localhost"}, 8080};    // pair<string, int>
    std::pair endpoint{std::string{"api.example.com"}, 443};
    std::vector endpoints{host, endpoint};              // vector<pair<string,int>>
    for (const auto& [h, port] : endpoints) {
        std::cout << "[Demo3] endpoint: " << h << ":" << port << '\n';
    }

    return 0;
}
