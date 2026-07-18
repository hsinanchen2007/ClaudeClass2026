// =============================================================================
//  02_decltype_auto.cpp  —  decltype(auto) (C++14)
// =============================================================================
//  參考：https://en.cppreference.com/w/cpp/language/auto
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 一、為什麼有 decltype(auto)？                              │
//  └────────────────────────────────────────────────────────────┘
//
//  auto 推導會「捨棄 reference / top-level const」（值語意），但有時候你
//  需要「精確保留 reference」 — 例如「轉發某個 lvalue 給呼叫者」。
//
//  例：
//
//      template <class C, class K>
//      auto get(C& c, const K& k) {
//          return c[k];           // 永遠 by value 回傳！
//      }
//
//  即使 c[k] 本來是「lvalue ref」(可以被改)，auto 推導後變成「值」 — 你
//  把改寫能力丟掉了。
//
//  decltype(auto) 解決：「auto 寫起來方便，又跟 decltype 一樣保留精準型別」：
//
//      template <class C, class K>
//      decltype(auto) get(C& c, const K& k) {
//          return c[k];           // 保留 ref
//      }
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 二、推導規則                                               │
//  └────────────────────────────────────────────────────────────┘
//
//      auto x = expr;             // 像「值」，不保留 ref/const
//      decltype(auto) x = expr;   // 跟 decltype(expr) 一樣 — 保留 ref/const
//
//  也用在「函式 return type 位置」：
//
//      decltype(auto) f() { return ...; }
//
//  關鍵：return 的「運算式形式」會影響推導：
//   * return some_var;        → 值
//   * return (some_var);       → 加 () → lvalue → reference！
//
//  常見錯誤：return (local) 會回傳「local 變數的 ref」 — UB。
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 三、本檔示範                                               │
//  └────────────────────────────────────────────────────────────┘
//
//   * Demo 1：auto vs decltype(auto) 的取出值差異
//   * Demo 2：用 decltype(auto) 寫「轉發容器存取」的 wrapper
//   * Demo 3：return (x) 帶括號的陷阱
// =============================================================================

/*
補充筆記：decltype_auto
  - decltype_auto 是現代 C++ 語法或標準庫特性；學習時要把「少寫字」和「語意更精確」分開看。
  - auto 讓型別由初始化式推導，但會丟掉 top-level const/reference；需要保留引用語意時要寫 auto&、const auto& 或 decltype(auto)。
  - brace initialization 能減少未初始化與 narrowing，但遇到 initializer_list overload 可能選到不同建構子。
  - constexpr、static_assert、if constexpr 把部分錯誤和計算提前到編譯期，能讓 template 和常數邏輯更清楚。
  - 屬性如 [[nodiscard]]、[[maybe_unused]]、[[fallthrough]] 是對編譯器和讀者的意圖標記，不應拿來掩蓋設計問題。
  - string_view、optional、variant、structured binding 等特性改善介面表達力，但也帶來生命週期或狀態檢查責任。
  - decltype(auto) 會保留 reference 和 value category，適合完美轉送回傳值。
  - return (x); 搭配 decltype(auto) 可能回傳 reference；括號會改變結果，必須刻意使用。
*/
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

// auto 版 — 永遠回傳值（即使 c[k] 是 lvalue ref）
template <class C, class K>
auto getByVal(C& c, const K& k) {
    return c[k];
}

// decltype(auto) 版 — 完整保留 c[k] 的型別（含 reference）
template <class C, class K>
decltype(auto) getByRef(C& c, const K& k) {
    return c[k];
}

int main() {
    // ─────────────────────────────────────────────────────────
    // Demo 1：簡單對比
    // ─────────────────────────────────────────────────────────
    int x = 10;
    int& xr = x;

    auto             a = xr;          // a 是 int（去掉 &）
    decltype(auto)   b = xr;          // b 是 int& —精確保留

    a = 99;                            // a 是「自己的 int」
    (void)a;                           // 抑制 set-but-not-used
    std::cout << "[Demo1] after a=99, x=" << x << " (expect 10)\n";

    b = 99;                            // 改 b 等於改 x
    std::cout << "[Demo1] after b=99, x=" << x << " (expect 99)\n";

    // ─────────────────────────────────────────────────────────
    // Demo 2：對 map 寫 setter wrapper
    // ─────────────────────────────────────────────────────────
    std::map<std::string, int> m{{"alice", 30}};

    // 用 by-value getter — 改不到 map
    auto v = getByVal(m, "alice");
    v = 999;
    (void)v;                         // 抑制 set-but-not-used warning
    std::cout << "[Demo2] after byVal change, alice=" << m["alice"]
              << " (expect 30 — change lost)\n";

    // 用 by-ref getter — 真的改 map
    decltype(auto) r = getByRef(m, "alice");
    r = 999;
    std::cout << "[Demo2] after byRef change, alice=" << m["alice"]
              << " (expect 999)\n";

    // ─────────────────────────────────────────────────────────
    // Demo 3：return (x) 帶括號的陷阱
    // ─────────────────────────────────────────────────────────
    auto safeReturn = []() -> decltype(auto) {
        int local = 42;
        return local;       // 回傳 int（局部變數 by value）
    };
    std::cout << "[Demo3] safeReturn() = " << safeReturn() << '\n';

    // 錯誤示範（保留為註解）：
    //   auto badReturn = []() -> decltype(auto) {
    //       int local = 42;
    //       return (local);    // ❌ 回傳 int& — 而 local 立刻死亡 → UB
    //   };

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：什麼時候要用 decltype(auto)？
    //    A：寫「轉發層」(perfect forwarding wrapper) 時想原封保留型別 —
    //       例如 STL 風格的容器存取 wrapper、字典查找介面。
    //
    //  Q2：return (x) 跟 return x; 差很大嗎？
    //    A：「decltype(auto) 推導下」差很大：
    //         return x;     → decltype(x)   通常是值
    //         return (x);   → decltype((x)) = lvalue → ref
    //       對 auto return type 沒差（auto 都剝掉 ref）。陷阱專屬於
    //       decltype(auto)。
    //
    //  Q3：可以混用 decltype(auto) 跟普通 auto 嗎？
    //    A：可以 — 它們在「同一個函式不同位置」獨立判斷。但建議「整個函
    //       式統一」，避免讀者搞混。
    //
    // ─────────────────────────────────────────────────────────
    // 實用範例 1：把 STL 容器存取 wrap 成「無痛轉發」
    //   工作上常見：寫 wrapper class 包住 vector，存取要保留 ref 語意
    // ─────────────────────────────────────────────────────────
    class IntStore {
    public:
        explicit IntStore(std::vector<int> v) : data_(std::move(v)) {}
        // 用 decltype(auto)：non-const 版回 int&、const 版回 const int&
        decltype(auto) at(std::size_t i)       { return data_[i]; }
        decltype(auto) at(std::size_t i) const { return data_[i]; }
        std::size_t size() const { return data_.size(); }
    private:
        std::vector<int> data_;
    };
    IntStore store{{10, 20, 30}};
    store.at(0) = 99;                      // ✅ at 回 int&，能寫入
    std::cout << "[Demo4] store.at(0) = " << store.at(0) << '\n';

    // ─────────────────────────────────────────────────────────
    // 實用範例 2：用 decltype(auto) 寫「對 map 找 key、回值的 ref」函式
    //   工作上常見：配置中心 lookup，存在就回 ref（可改），不存在 throw
    //   注意：lambda 沒寫 trailing return 時走 auto 推導 → 拷貝；要 ref 必須
    //          顯式寫 -> decltype(auto)，且 body return 為 lvalue 表達式。
    // ─────────────────────────────────────────────────────────
    auto lookup = [](std::map<std::string, int>& m,
                     const std::string& key) -> decltype(auto) {
        auto it = m.find(key);
        if (it == m.end()) throw std::runtime_error("key not found");
        return (it->second);               // ⚠️ 括號很重要 — 強制 lvalue → int&
    };
    std::map<std::string, int> cfg{{"timeout", 30}};
    lookup(cfg, "timeout") = 60;           // 透過 ref 直接改 map
    std::cout << "[Demo5] cfg[timeout] after lookup-assign = "
              << cfg["timeout"] << '\n';

    return 0;
}
